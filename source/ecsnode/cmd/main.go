package main

import (
	"context"
	apisv1 "ecsnode/pkg/apis/v1"
	"ecsnode/pkg/controller"
	"ecsnode/pkg/util"
	"flag"
	"os"
	"os/signal"
	"syscall"

	"k8s.io/apimachinery/pkg/runtime"
	utilruntime "k8s.io/apimachinery/pkg/util/runtime"
	clientgoscheme "k8s.io/client-go/kubernetes/scheme"
	"k8s.io/klog/v2"
	ctrl "sigs.k8s.io/controller-runtime"
	metricserver "sigs.k8s.io/controller-runtime/pkg/metrics/server"
)

var (
	scheme = runtime.NewScheme()
	nsenv  = "NAMESPACE"
)

func init() {
	utilruntime.Must(clientgoscheme.AddToScheme(scheme))
	utilruntime.Must(apisv1.AddToScheme(scheme))
}

func main() {
	config := flag.String("config", "config.yaml", "配置文件")

	ctrl.RegisterFlags(flag.CommandLine)
	klog.InitFlags(flag.CommandLine)

	flag.Parse()
	cfg, err := LoadFromYaml(*config)
	if err != nil {
		klog.Fatalf("load config error: %v", err)
	}
	defaultns := os.Getenv(nsenv)
	if defaultns == "" {
		klog.Fatalf("Missing required environment variables: %v", nsenv)
	}
	err = ApplyDefault(cfg, defaultns)
	if err != nil {
		klog.Fatalf("apply default config error: %v", err)
	}
	klog.Infof("config: %v", cfg)
	klog.Infof("Starting ecsnode version: %s build-time: %s", Version, BuildTime)

	ctx := SetupSignalHandler()

	// controller runtime
	restconfig := ctrl.GetConfigOrDie()
	ctrl.SetLogger(klog.NewKlogr())
	mgr, err := ctrl.NewManager(restconfig, ctrl.Options{
		Scheme:                  scheme,
		LeaderElection:          true,
		LeaderElectionID:        "89x1.ecsnode.leader",
		LeaderElectionNamespace: defaultns,
		Metrics:                 metricserver.Options{BindAddress: "0"},
		HealthProbeBindAddress:  "0",
	})
	if err != nil {
		klog.Fatalf("initialize manager failed: %v", err)
	}
	pubsub := util.NewPubSub()
	ecsctl := controller.NewEcsNode(&cfg.EcsNode, pubsub, mgr)
	cmctl := controller.NewConfigMapSync(&cfg.Configmap, pubsub, mgr)
	poctl := controller.NewPod(&cfg.PodConfig, pubsub, mgr)
	svcctl := controller.NewServiceSync(&cfg.ServiceConfig, poctl.ListPodHostIp, pubsub, mgr)

	mgr.Add(ecsctl)
	mgr.Add(cmctl)
	mgr.Add(poctl)
	mgr.Add(svcctl)

	// start manager

	go func() {
		if err := mgr.Start(ctx); err != nil {
			klog.Fatalf("running manager failed: %v", err)
		}
	}()

	<-ctx.Done()
}

func SetupSignalHandler() context.Context {
	ctx, cancel := context.WithCancel(context.Background())

	c := make(chan os.Signal, 2)
	signal.Notify(c, []os.Signal{os.Interrupt, syscall.SIGTERM}...)
	go func() {
		<-c
		cancel()
	}()

	return ctx
}
