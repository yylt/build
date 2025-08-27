package controller

import (
	"context"
	"ecsnode/pkg/util"
	"fmt"
	"reflect"
	"sync"

	corev1 "k8s.io/api/core/v1"
	apierrors "k8s.io/apimachinery/pkg/api/errors"
	"k8s.io/apimachinery/pkg/labels"
	"k8s.io/apimachinery/pkg/util/sets"
	"k8s.io/klog/v2"
	ctrl "sigs.k8s.io/controller-runtime"
	ctrclient "sigs.k8s.io/controller-runtime/pkg/client"
	"sigs.k8s.io/controller-runtime/pkg/manager"
	"sigs.k8s.io/controller-runtime/pkg/predicate"
)

type PodConfig struct {
	Lables map[string]string `json:"labels,omitempty" yaml:"labels,omitempty"`

	// pod namespace
	Namespace string `json:"namespace,omitempty" yaml:"namespace,omitempty"`
}

func (c *PodConfig) Valid() error {
	if len(c.Namespace) == 0 {
		return fmt.Errorf("namespace must not be empty")
	}
	return nil
}

type StatIp struct {
	Ready  bool
	NodeIp string
	HostIp sets.Set[string]
}

func (s *StatIp) String() string {
	return fmt.Sprintf("ready=%v, hostIp=%v", s.Ready, s.NodeIp)
}

type PodCtrl struct {
	config *PodConfig

	mu      sync.RWMutex
	podInfo map[string]*StatIp

	hostmu     sync.RWMutex
	hostIpDict map[string]sets.Set[string]

	done chan struct{}

	ps *util.PubSub

	ecssub *util.Subscriber
	client ctrclient.Client
}

func NewPod(config *PodConfig, ps *util.PubSub, mgr manager.Manager) *PodCtrl {
	ecssub, err := ps.Subscribe(EcsNodeTopic, "pod")
	if err != nil {
		klog.Fatalf("subscribe ecsnode topic failed: %v", err)
	}
	newctl := &PodCtrl{
		config:     config,
		podInfo:    make(map[string]*StatIp),
		hostIpDict: make(map[string]sets.Set[string]),
		done:       make(chan struct{}),
		ps:         ps,
		ecssub:     ecssub,
		client:     mgr.GetClient(),
	}

	ctrl.NewControllerManagedBy(mgr).
		For(&corev1.Pod{}).
		WithEventFilter(predicate.NewPredicateFuncs(func(object ctrclient.Object) bool {
			pod, ok := object.(*corev1.Pod)
			if !ok {
				return false
			}
			// namespace filter
			if newctl.config.Namespace != "" && pod.Namespace != newctl.config.Namespace {
				return false
			}
			// labels filter
			if newctl.config.Lables != nil {
				if len(pod.Labels) == 0 {
					return false
				}
				for k, v := range newctl.config.Lables {
					if pod.Labels[k] != v {
						return false
					}
				}
			}
			return true
		})).
		Complete(newctl)

	return newctl
}
func (h *PodCtrl) NeedLeaderElection() bool { return true }

func (h *PodCtrl) Start(ctx context.Context) error {
	go func() {
		for {
			select {
			case <-ctx.Done():
				klog.Infof("pod subscribe controller exit: %v", ctx.Err())
				return
			case msg, ok := <-h.ecssub.GetMessage():
				if !ok {
					klog.Errorf("pod subscribe had close")
					return
				}
				origin, ok := msg.Data.(map[string]sets.Set[string])
				if !ok {
					klog.Errorf("pod subscribe data type is: %v, except: %v", reflect.TypeOf(msg.Data), "map[string]sets.Set[string]")
					break
				}
				klog.Infof("pod controller receive event topic: %s", msg.Topic)
				var (
					changed []string
				)

				h.hostmu.Lock()
				// add or update
				for k, v := range origin {
					oldv, ok := h.hostIpDict[k]
					if !ok || !v.Equal(oldv) {
						changed = append(changed, k)
						h.hostIpDict[k] = v.Clone()
					}
				}
				// remove not exist host
				if len(h.hostIpDict) > len(origin) {
					for k := range h.hostIpDict {
						if _, ok := origin[k]; !ok {
							changed = append(changed, k)
							delete(h.hostIpDict, k)
						}
					}
				}
				h.hostmu.Unlock()
				if len(changed) > 0 {
					klog.Infof("pod controller, host changed: %s", changed)
				}
			}
		}
	}()
	return nil
}

func (h *PodCtrl) publishEvent() {
	h.ps.Publish(PodTopic, nil)
}

func (h *PodCtrl) HadDone() <-chan struct{} {
	return h.done
}

// not thread safe
// should only be called once, and depencies had been ready
func (h *PodCtrl) syncOnce(ctx context.Context) (bool, error) {
	select {
	case <-h.done:
		return false, nil
	default:
	}
	hostHadSync := func() bool {
		h.hostmu.RLock()
		defer h.hostmu.RUnlock()
		return len(h.hostIpDict) > 0
	}()

	if !hostHadSync {
		return false, fmt.Errorf("ecsnode not sync yet")
	}
	var (
		pods = corev1.PodList{}
	)
	err := h.client.List(ctx, &pods, &ctrclient.ListOptions{
		Namespace:     h.config.Namespace,
		LabelSelector: labels.SelectorFromSet(h.config.Lables)},
	)
	if err != nil {
		return false, fmt.Errorf("init sync failed, list pod error: %v", err)
	}
	for _, pod := range pods.Items {
		h.podInfo[pod.Name] = h.stat(&pod)
	}
	klog.Infof("init pod success, pods: %v", len(h.podInfo))
	close(h.done)
	return true, nil
}

// list hostip which pod is running on
func (h *PodCtrl) ListPodHostIp(ctx context.Context, publish_notready bool) ([]string, error) {
	var (
		hostips []string
	)
	select {
	case <-h.done:
	default:
		return nil, fmt.Errorf("pod controller is not synconce Done")
	}

	h.mu.RLock()
	defer h.mu.RUnlock()
	for _, stat := range h.podInfo {
		if publish_notready || stat.Ready {
			hostips = append(hostips, stat.HostIp.UnsortedList()...)
		}
	}
	return hostips, nil
}

func (h *PodCtrl) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
	var (
		pod           = &corev1.Pod{}
		err           error
		shouldpublish = false
	)
	defer func() {
		if shouldpublish {
			h.publishEvent()
		}
	}()
	shouldpublish, err = h.syncOnce(ctx)
	if err != nil {
		klog.Errorf("sync pod failed, err: %v, requeue after: %s", err, RequeuDelay)
		return ctrl.Result{RequeueAfter: RequeuDelay}, nil
	}
	klog.V(2).Infof("start pod reconcile: %v", req.NamespacedName)
	err = h.client.Get(ctx, req.NamespacedName, pod)
	h.mu.Lock()
	defer h.mu.Unlock()
	oldstat, ok := h.podInfo[pod.Name]
	if err != nil {
		klog.Errorf("get pod %s failed, err: %v", req.NamespacedName, err)
		if apierrors.IsNotFound(err) && ok {
			delete(h.podInfo, pod.Name)
			shouldpublish = true
		}
		return ctrl.Result{}, nil
	}
	if pod.DeletionTimestamp != nil && ok {
		klog.Infof("pod %s is deleting", req.NamespacedName)
		delete(h.podInfo, req.Name)
		shouldpublish = true
		return ctrl.Result{}, nil
	}
	if changed, newstat := h.hadChanged(oldstat, pod); changed {
		klog.Infof("pod(%s) state changed, old: %v, new: %v", req.NamespacedName, oldstat, newstat)
		h.podInfo[pod.Name] = newstat
		shouldpublish = true
	}
	return ctrl.Result{}, nil
}

func (h *PodCtrl) hadChanged(oldstat *StatIp, pod *corev1.Pod) (bool, *StatIp) {
	newstat := h.stat(pod)
	if oldstat == nil {
		if newstat.HostIp == nil || newstat.HostIp.Len() == 0 {
			return false, newstat
		}
		return true, newstat
	}
	if oldstat.Ready != isReady(pod) || !oldstat.HostIp.Equal(newstat.HostIp) {
		return true, newstat
	}
	return false, newstat
}

func (h *PodCtrl) stat(pod *corev1.Pod) *StatIp {
	var (
		ready  = isReady(pod)
		ipset  = sets.New[string]()
		poname = ctrclient.ObjectKeyFromObject(pod)
	)

	if pod.Status.HostIP != "" {
		h.hostmu.RLock()
		hostips, ok := h.hostIpDict[pod.Status.HostIP]
		if !ok {
			klog.Warningf("pod(%s) hostIp(%s) not found from hostIpDict", poname, pod.Status.HostIP)
		} else {
			ipset = hostips.Clone()
		}
		h.hostmu.RUnlock()
	}

	stat := &StatIp{
		NodeIp: pod.Status.HostIP,
		Ready:  ready,
		HostIp: ipset,
	}
	klog.Infof("pod(%s) stat: %s", poname, stat)
	return stat
}

func isReady(po *corev1.Pod) bool {
	if po.Status.Phase != corev1.PodRunning {
		return false
	}
	for _, cs := range po.Status.InitContainerStatuses {
		if !cs.Ready {
			return false
		}
	}
	for _, cs := range po.Status.ContainerStatuses {
		if !cs.Ready {
			return false
		}
	}
	return true
}
