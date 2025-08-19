package main

import (
	"context"
	"crypto/tls"
	"etcdcluster/util"
	"fmt"
	"net"
	"net/http"
	"os"
	"os/exec"
	"os/signal"
	"path/filepath"
	"regexp"
	"strconv"
	"strings"
	"sync"
	"syscall"
	"time"

	etcdcli "go.etcd.io/etcd/client/v3"
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
)

var (
	Version, BuildTime string

	logger  *zap.Logger
	ETCDBIN string
)

type EnvVars map[string]string

func init() {
	var err error
	config := zap.Config{
		Level:       zap.NewAtomicLevelAt(zap.InfoLevel),
		Development: false,
		Encoding:    "console",
		EncoderConfig: zapcore.EncoderConfig{
			// Keys can be anything except the empty string.
			TimeKey:        "T",
			LevelKey:       "L",
			CallerKey:      "C",
			FunctionKey:    zapcore.OmitKey,
			MessageKey:     "M",
			EncodeDuration: zapcore.StringDurationEncoder,
			EncodeCaller:   zapcore.ShortCallerEncoder,
			LineEnding:     zapcore.DefaultLineEnding,
			EncodeLevel:    zapcore.CapitalLevelEncoder,
			EncodeTime:     zapcore.RFC3339TimeEncoder,
		},
		OutputPaths:      []string{"stderr"},
		ErrorOutputPaths: []string{"stderr"},
	}
	logger, err = config.Build()
	if err != nil {
		fmt.Printf("Failed to initialize logger: %v\n", err)
		os.Exit(1)
	}
}

func main() {
	defer logger.Sync()
	var (
		err error
	)
	logger.Info("Starting etcd-cluster",
		zap.String("version", Version),
		zap.String("build-time", BuildTime))
	// 1. Check required environment variables
	if !checkRequiredEnvVars() {
		os.Exit(1)
	}
	ETCDBIN, err = exec.LookPath("etcd")
	if err != nil {
		logger.Fatal("etcd binary not found")
	}
	// Parse POD_NAME
	prefix, myIndex, err := parsePodName(os.Getenv("POD_NAME"))
	if err != nil {
		logger.Error("Failed to parse POD_NAME", zap.Error(err))
		os.Exit(1)
	}

	myIPs, err := readIPFile(fmt.Sprintf("%s.%s.%s.svc.cluster.local",
		os.Getenv("POD_NAME"), os.Getenv("SERVICE_NAME"), os.Getenv("POD_NAMESPACE")))
	if err != nil || len(myIPs) == 0 {
		logger.Fatal("Failed to read IP file for current pod", zap.Error(err))
	}
	// Get alive endpoints
	for true {
		aliveEndpoints := getAliveEndpoints(prefix, util.MustAtoi(os.Getenv("MAX")), myIndex)
		switch len(aliveEndpoints) {
		case 0:
			if myIndex != 0 {
				logger.Warn("Failed to get alive endpoints, retrying...",
					zap.String("pod_name", os.Getenv("POD_NAME")))
				break
			}
			cmd, err := initializeNewCluster(myIPs)
			if err != nil {
				logger.Error("Failed to initialize new cluster", zap.Error(err))
				break
			}
			waitExit(cmd)
		default:
			client, err := getCurrentClient(aliveEndpoints)
			if err != nil {
				logger.Error("Failed to get current client", zap.Error(err))
				break
			}
			cmd, err := joinExistingCluster(client, myIPs)
			if err == nil {
				waitExit(cmd)
			} else {
				logger.Error("Failed to join existing cluster", zap.Error(err))
			}
		}
		time.Sleep(2 * time.Second)
	}
}

func checkRequiredEnvVars() bool {
	requiredVars := []string{
		"SERVICE_NAME", "MAX", "NODEIP_DIR",
		"POD_NAME", "POD_NAMESPACE",
		"ETCDCTL_CACERT", "ETCDCTL_CERT", "ETCDCTL_KEY", "ETCD_DATA_DIR",
		"CLIENT_PORT", "PEER_PORT",
	}

	missingVars := []string{}
	for _, varName := range requiredVars {
		if os.Getenv(varName) == "" {
			missingVars = append(missingVars, varName)
		}
	}

	if len(missingVars) > 0 {
		logger.Error("Missing required environment variables",
			zap.Strings("missingVars", missingVars))
		return false
	}
	logger.Info("Starting cluster initialization",
		zap.String("SERVICE_NAME", os.Getenv("SERVICE_NAME")),
		zap.String("MAX", os.Getenv("MAX")),
		zap.String("POD_NAME", os.Getenv("POD_NAME")),
		zap.String("POD_NAMESPACE", os.Getenv("POD_NAMESPACE")),
		zap.String("ETCD_DATA_DIR", os.Getenv("ETCD_DATA_DIR")),
		zap.String("NODEIP_DIR", os.Getenv("NODEIP_DIR")),
		zap.String("CLIENT_PORT", os.Getenv("CLIENT_PORT")),
		zap.String("PEER_PORT", os.Getenv("PEER_PORT")),
	)
	return true
}

func parsePodName(podName string) (string, int, error) {
	re := regexp.MustCompile(`^(.*)-(\d+)$`)
	matches := re.FindStringSubmatch(podName)
	if len(matches) != 3 {
		return "", 0, fmt.Errorf("invalid POD_NAME format: %s", podName)
	}

	prefix := matches[1]
	index, err := strconv.Atoi(matches[2])
	if err != nil {
		return "", 0, fmt.Errorf("invalid index in POD_NAME: %s", podName)
	}

	logger.Info("Parsed POD_NAME", zap.String("prefix", prefix), zap.Int("index", index))
	return prefix, index, nil
}

func readIPFile(domain string) ([]string, error) {
	ips, err := net.LookupHost(domain)
	if err != nil {
		logger.Error("Failed to resolve domain", zap.String("domain", domain), zap.Error(err))
		return nil, err
	}

	if len(ips) == 0 {
		return nil, fmt.Errorf("no IPs found for domain %s", domain)
	}

	resolvedIP := ips[0]
	logger.Info("Resolved domain", zap.String("domain", domain), zap.String("ip", resolvedIP))

	ipFile := filepath.Join(os.Getenv("NODEIP_DIR"), resolvedIP)
	if !util.FileExists(ipFile) {
		logger.Warn("IP file not found, using resolved IP directly",
			zap.String("ipFile", ipFile), zap.String("domain", domain))
		return []string{resolvedIP}, nil
	}

	content, err := os.ReadFile(ipFile)
	if err != nil {
		logger.Error("Failed to read IP file", zap.String("domain", domain), zap.Error(err))
		return nil, err
	}

	ipList := strings.Split(strings.TrimSpace(string(content)), ",")
	logger.Info("Read IPs from file", zap.String("domain", domain), zap.Strings("ips", ipList))
	return ipList, nil
}

func checkReadyz(ipport string, tlscfg *tls.Config) bool {
	url := fmt.Sprintf("https://%s/readyz", ipport)

	// Setup HTTPS client
	client := &http.Client{
		Transport: &http.Transport{
			TLSClientConfig: tlscfg,
		},
		Timeout: 2 * time.Second,
	}

	resp, err := client.Get(url)
	if err != nil {
		logger.Error("Failed to check etcd health", zap.String("url", url))
		return false
	}
	defer resp.Body.Close()

	if resp.StatusCode == http.StatusOK {
		logger.Info("Etcd node is ready", zap.String("ipport", ipport))
		return true
	}

	logger.Warn("Etcd node is not ready",
		zap.String("ipport", ipport),
		zap.Int("statusCode", resp.StatusCode))
	return false
}

func peerEndponits(endpoints map[string][]string, port string, withname bool, withscheme bool) []string {
	var addresses []string
	for k, iplist := range endpoints {
		for _, ip := range iplist {
			if withscheme {
				ip = fmt.Sprintf("https://%s", ip)
			}
			if withname {
				addresses = append(addresses, fmt.Sprintf("%s=%s:%s", k, ip, port))
			} else {
				addresses = append(addresses, fmt.Sprintf("%s:%s", ip, port))
			}
		}
	}
	return addresses
}

func getAliveEndpoints(prefix string, maxNodes, myIndex int) map[string][]string {
	aliveEndpoints := make(map[string][]string)
	var (
		wg          sync.WaitGroup
		mu          sync.Mutex
		client_port = os.Getenv("CLIENT_PORT")
		podnames    []string
	)
	// Load client cert
	cert, err := tls.LoadX509KeyPair(os.Getenv("ETCDCTL_CERT"), os.Getenv("ETCDCTL_KEY"))
	if err != nil {
		logger.Fatal("Failed to load key pair", zap.Error(err))
	}
	tlscfg := &tls.Config{
		Certificates:       []tls.Certificate{cert},
		InsecureSkipVerify: true,
	}
	wg.Add(maxNodes - 1)
	for i := 0; i < maxNodes; i++ {
		if i == myIndex {
			continue
		}

		podName := fmt.Sprintf("%s-%d", prefix, i)
		domain := fmt.Sprintf("%s.%s.%s.svc.cluster.local",
			podName, os.Getenv("SERVICE_NAME"), os.Getenv("POD_NAMESPACE"))

		ips, err := readIPFile(domain)
		if err != nil || len(ips) == 0 {
			continue
		}
		go func(iplist []string, podname string) {
			defer wg.Done()
			var ready bool
			// Check if node is healthy
			for _, ip := range iplist {
				ipport := net.JoinHostPort(ip, client_port)
				if checkReadyz(ipport, tlscfg) {
					aliveEndpoints[podName] = iplist
					logger.Info("Found alive endpoint",
						zap.String("pod", podName))
					ready = true
					break
				}
			}
			if ready {
				mu.Lock()
				defer mu.Unlock()
				aliveEndpoints[podname] = iplist
				podnames = append(podnames, podname)
			}
		}(ips, podName)
	}
	wg.Wait()
	logger.Info("Total alive endpoints found", zap.Strings("all", podnames))
	return aliveEndpoints
}

func getCurrentClient(aliveEndpoints map[string][]string) (*etcdcli.Client, error) {
	if len(aliveEndpoints) == 0 {
		return nil, fmt.Errorf("no alive endpoints found")
	}
	var endpoints []string
	for _, iplist := range aliveEndpoints {
		for _, ip := range iplist {
			endpoints = append(endpoints, net.JoinHostPort(ip, os.Getenv("CLIENT_PORT")))
		}
	}
	// Load client cert
	cert, err := tls.LoadX509KeyPair(os.Getenv("ETCDCTL_CERT"), os.Getenv("ETCDCTL_KEY"))
	if err != nil {
		return nil, fmt.Errorf("failed to load key pair: %v", err)
	}

	cliconfig := etcdcli.Config{
		Endpoints:   endpoints,
		DialTimeout: 2 * time.Second,
		TLS: &tls.Config{
			InsecureSkipVerify: true,
			Certificates:       []tls.Certificate{cert},
		},
	}
	client, err := etcdcli.New(cliconfig)
	if err != nil {
		return nil, fmt.Errorf("failed to create etcd client: %v", err)
	}
	return client, nil
}

func joinExistingCluster(client *etcdcli.Client, myIPs []string) (*exec.Cmd, error) {
	var (
		cluster, mypeers []string
		addResp          *etcdcli.MemberAddResponse

		myMemberID uint64
		podname    = os.Getenv("POD_NAME")
		peerport   = os.Getenv("PEER_PORT")

		bgct        = context.Background()
		ctx, cancle = context.WithTimeout(bgct, 3*time.Second)
	)
	defer cancle()
	resp, err := client.MemberList(etcdcli.WithRequireLeader(ctx))
	if err != nil {
		return nil, err
	}

	for _, member := range resp.Members {
		if member.Name == podname {
			myMemberID = member.ID
			if util.DirExists(filepath.Join(os.Getenv("ETCD_DATA_DIR"), "member")) {
				if member.GetIsLearner() {
					_, err = client.MemberPromote(ctx, myMemberID)
					if err != nil {
						return nil, err
					}
				}
				logger.Info("memberdir exists, and include current pod, start etcd")
				return startEtcd()
			}
		}
	}
	// must remove member, because memberdir not exists, but member exist
	if myMemberID != 0 {
		logger.Info("memberdir not exist, must remove member", zap.Uint64("id", myMemberID))
		_, err = client.MemberRemove(ctx, myMemberID)
		if err != nil {
			return nil, err
		}
	}
	os.RemoveAll(filepath.Join(os.Getenv("ETCD_DATA_DIR"), "member"))
	os.Setenv("ETCD_INITIAL_CLUSTER_STATE", "existing")
	for _, ip := range myIPs {
		mypeers = append(mypeers, fmt.Sprintf("https://%s:%s", ip, peerport))
	}
	ctx2, cancle2 := context.WithTimeout(bgct, 3*time.Second)
	defer cancle2()
	if len(resp.Members) != 1 {
		logger.Info("add member then start etcd")
		addResp, err = client.MemberAdd(ctx2, mypeers)
	} else {
		logger.Info("only one endpoint, start etcd as learner then prompt")
		addResp, err = client.MemberAddAsLearner(ctx2, mypeers)
	}
	if err != nil {
		return nil, err
	}
	for _, member := range addResp.Members {
		for _, url := range member.PeerURLs {
			if member.Name == "" {
				member.Name = podname
			}
			cluster = append(cluster, fmt.Sprintf("%s=%s", member.Name, url))
		}
	}
	os.Setenv("ETCD_INITIAL_CLUSTER", strings.Join(cluster, ","))
	cmd, err := startEtcd()
	if err != nil {
		return nil, err
	}
	if len(resp.Members) == 1 {
		count := 1
		for count < 10 {
			_, err = client.MemberPromote(bgct, addResp.Member.ID)
			if err == nil {
				logger.Info("promote member success", zap.String("member", podname))
				break
			}
			time.Sleep(1 * time.Second)
			count++
		}
	}
	if err != nil {
		err = cmd.Process.Signal(syscall.SIGTERM)
		return nil, err
	}
	return cmd, nil
}

func initializeNewCluster(ips []string) (*exec.Cmd, error) {
	logger.Info("Initializing new etcd cluster")
	os.Setenv("ETCD_INITIAL_CLUSTER_STATE", "new")
	cluster := peerEndponits(
		map[string][]string{os.Getenv("POD_NAME"): ips},
		os.Getenv("PEER_PORT"),
		true, true)

	os.Setenv("ETCD_INITIAL_CLUSTER", strings.Join(cluster, ","))
	os.RemoveAll(filepath.Join(os.Getenv("ETCD_DATA_DIR"), "member"))
	logger.Info("Starting etcd with new cluster configuration")
	return startEtcd()
}

func startEtcd() (*exec.Cmd, error) {
	cmd := exec.Command(ETCDBIN)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	cmd.Env = os.Environ()
	if err := cmd.Start(); err != nil {
		return nil, err
	}
	return cmd, nil
}

func waitExit(cmd *exec.Cmd) {
	signalChan := make(chan os.Signal, 1)
	signal.Notify(signalChan, syscall.SIGINT, syscall.SIGTERM)

	processExitChan := make(chan bool, 1)

	go func() {
		err := cmd.Wait()
		if err != nil {
			logger.Info("进程退出并出现错误：", zap.Error(err))
		} else {
			logger.Info("进程成功退出。")
		}
		processExitChan <- true
	}()

	logger.Info("主进程正在等待信号或子进程退出...")
	select {
	case s := <-signalChan:
		fmt.Printf("\n主进程收到信号：%v，准备退出。\n", s)
		if err := cmd.Process.Kill(); err != nil {
			logger.Fatal("无法杀死子进程", zap.Error(err))
		}
		logger.Info("已杀死子进程。")
	case <-processExitChan:
		logger.Info("子进程已退出，主进程也准备退出。")
	}
	logger.Info("主进程退出。")
	os.Exit(0)
}
