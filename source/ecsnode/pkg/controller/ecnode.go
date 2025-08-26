package controller

import (
	"context"
	ecsnodev1 "ecsnode/pkg/apis/v1"
	"ecsnode/pkg/util"
	"fmt"
	"strings"
	"sync"

	apierrors "k8s.io/apimachinery/pkg/api/errors"
	"k8s.io/apimachinery/pkg/util/sets"
	"k8s.io/klog/v2"
	ctrl "sigs.k8s.io/controller-runtime"
	ctrclient "sigs.k8s.io/controller-runtime/pkg/client"
	"sigs.k8s.io/controller-runtime/pkg/manager"
)

type EcsNodeConfig struct {
	Interfaces []string `json:"interfaces" yaml:"interfaces"`
	MasterIf   string   `json:"masterif" yaml:"masterif"`

	// EcsNode Namespace:
	Namespace string `json:"namespace,omitempty" yaml:"namespace,omitempty"`
}

func (c *EcsNodeConfig) Valid() error {
	if len(c.Interfaces) == 0 || len(c.MasterIf) == 0 {
		return fmt.Errorf("interfaces or masterif must not be empty")
	}
	if c.Namespace == "" {
		return fmt.Errorf("namespace must not be empty")
	}
	return nil
}

type EcsNode struct {
	mu      sync.RWMutex
	hostips map[string]sets.Set[string]

	interfaces map[string]struct{}

	ps *util.PubSub

	done    chan struct{}
	trigger chan struct{}

	config *EcsNodeConfig
	client ctrclient.Client
}

func NewEcsNode(config *EcsNodeConfig, ps *util.PubSub, mgr manager.Manager) *EcsNode {
	var ifs = make(map[string]struct{})
	for i := range config.Interfaces {
		ifs[config.Interfaces[i]] = struct{}{}
	}

	ecs := &EcsNode{
		client:     mgr.GetClient(),
		hostips:    make(map[string]sets.Set[string]),
		config:     config,
		ps:         ps,
		done:       make(chan struct{}),
		trigger:    make(chan struct{}, 10),
		interfaces: ifs,
	}
	ctrl.NewControllerManagedBy(mgr).
		For(&ecsnodev1.ECSNode{}).
		Complete(ecs)

	return ecs
}
func (h *EcsNode) HadDone() <-chan struct{} {
	return h.done
}

func (h *EcsNode) onceInit(ctx context.Context) (bool, error) {
	var (
		nodes = ecsnodev1.ECSNodeList{}
	)
	select {
	case <-h.done:
		return false, nil
	default:
	}
	err := h.client.List(ctx, &nodes)
	if err != nil {
		klog.Warningf("init ecsnode failed, list failed: %v", err)
		return false, err
	}

	for _, node := range nodes.Items {
		mip, ipset := h.getIpmap(&node)
		if mip == "" {
			klog.Warningf("node %s not found master ip", node.Spec.Hostname)
			continue
		}
		h.hostips[mip] = ipset
	}
	klog.Infof("init ecsnode success, nodes: %v", len(nodes.Items))
	close(h.done)
	return true, nil
}

func (h *EcsNode) NeedLeaderElection() bool { return true }

func (h *EcsNode) Start(ctx context.Context) error {
	go func() {
		for {
			select {
			case <-ctx.Done():
				close(h.trigger)
				klog.Infof("controller had stop: %v", ctx.Err())
				return

			case _, ok := <-h.trigger:
				if !ok {
					return
				}
				klog.Infof("publish ecsnode event start")
				h.mu.RLock()
				h.ps.Publish(EcsNodeTopic, h.hostips)
				h.mu.RUnlock()
				klog.Infof("publish ecsnode event done")
			}
		}
	}()
	return nil
}

// first reconcile should publish event
// other would compare then publish
func (h *EcsNode) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
	var (
		node          = ecsnodev1.ECSNode{}
		err           error
		shouldpublish bool
	)
	defer func() {
		if shouldpublish {
			klog.Infof("ecsnode had changed, publish event")
			h.trigger <- struct{}{}
		}
	}()
	klog.V(2).Infof("start ecsnode reconcile: %v", req.NamespacedName)

	shouldpublish, err = h.onceInit(ctx)
	if err != nil {
		klog.Errorf("init ecsnode failed, err: %v", err)
		return ctrl.Result{RequeueAfter: RequeuDelay}, err
	}
	if shouldpublish {
		return ctrl.Result{}, nil
	}

	err = h.client.Get(ctx, req.NamespacedName, &node)

	mip, newips := h.getIpmap(&node)
	if err != nil {
		klog.Errorf("get ecsnode %s(%s) failed, err: %v", req.Name, node.Spec.Hostname, err)
		if apierrors.IsNotFound(err) {
			klog.Infof("ecsnode %s not found, but not matter", req.Name)
		}
		return ctrl.Result{}, nil
	}
	if mip == "" {
		klog.Errorf("get ecsnode %s failed, masterip not found", node.Spec.Hostname)
		return ctrl.Result{}, nil
	}
	if node.DeletionTimestamp != nil {
		klog.Infof("ecsnode %s is deleting", node.Spec.Hostname)
		delete(h.hostips, mip)
		shouldpublish = true
		return ctrl.Result{}, nil
	}
	h.mu.RLock()
	defer h.mu.RUnlock()
	oldips, ok := h.hostips[mip]
	if !ok || !newips.Equal(oldips) {
		klog.Infof("ecsnode %s had changed, old: %v, new: %v", node.Spec.Hostname, oldips, newips)
		shouldpublish = true
		h.hostips[mip] = newips
	}
	return ctrl.Result{}, nil
}

func (h *EcsNode) getIpmap(node *ecsnodev1.ECSNode) (string, sets.Set[string]) {
	if node == nil {
		return "", nil
	}
	var (
		ips    = sets.New[string]()
		master string
	)

	for _, ep := range node.Spec.Endpoints {
		if ep == nil {
			continue
		}

		ipnet := strings.Split(ep.Ipnet, "/")
		ipstr := strings.TrimSpace(ipnet[0])
		if len(ipstr) == 0 {
			continue
		}
		if _, ok := h.interfaces[ep.Name]; !ok {
			continue
		}
		if ep.Name == h.config.MasterIf {
			master = ipstr
		}
		ips.Insert(ipstr)
	}
	klog.V(2).Infof("node: %s, status: %s, ip: %v", node.Spec.Hostname, node.Spec.Status, ips)
	return master, ips
}
