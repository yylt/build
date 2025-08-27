package controller

import (
	"context"
	"ecsnode/pkg/util"
	"fmt"
	"reflect"
	"strings"
	"sync"
	"time"

	corev1 "k8s.io/api/core/v1"
	v1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/apimachinery/pkg/types"
	"k8s.io/apimachinery/pkg/util/sets"
	"k8s.io/klog/v2"
	ctrl "sigs.k8s.io/controller-runtime"

	ctrclient "sigs.k8s.io/controller-runtime/pkg/client"
	"sigs.k8s.io/controller-runtime/pkg/controller/controllerutil"
	"sigs.k8s.io/controller-runtime/pkg/manager"
	"sigs.k8s.io/controller-runtime/pkg/predicate"
)

type ConfigMapConfig struct {
	Namespace string `json:"namespace,omitempty" yaml:"namespace,omitempty"`
	Name      string `json:"name" yaml:"name"`
}

func (c *ConfigMapConfig) Valid() error {
	if len(c.Name) == 0 || len(c.Namespace) == 0 {
		return fmt.Errorf("name and namespace must not be empty")
	}
	return nil
}

type ConfigMapSync struct {
	config *ConfigMapConfig

	mu   sync.RWMutex
	data map[string]sets.Set[string]

	nodesub *util.Subscriber

	client ctrclient.Client

	triggerch chan struct{}
}

func NewConfigMapSync(config *ConfigMapConfig, ps *util.PubSub, mgr manager.Manager) *ConfigMapSync {

	nodesub, err := ps.Subscribe(EcsNodeTopic, "configmap")
	if err != nil {
		klog.Fatal(err)
	}

	ss := &ConfigMapSync{

		config:    config,
		data:      make(map[string]sets.Set[string]),
		nodesub:   nodesub,
		client:    mgr.GetClient(),
		triggerch: make(chan struct{}, 10),
	}

	ctrl.NewControllerManagedBy(mgr).
		For(&corev1.ConfigMap{}).
		WithEventFilter(predicate.NewPredicateFuncs(func(object ctrclient.Object) bool {
			res, ok := object.(*corev1.ConfigMap)
			if !ok {
				return false
			}
			if res.Name != ss.config.Name || res.Namespace != ss.config.Namespace {
				return false
			}
			return true
		})).
		Complete(ss)
	return ss
}

func (s *ConfigMapSync) NeedLeaderElection() bool { return true }

func (s *ConfigMapSync) Start(ctx context.Context) error {
	go func() {
		for {
			select {
			case <-ctx.Done():
				close(s.triggerch)
				klog.Infof("controller had stop: %v", ctx.Err())
				return
			case msg, ok := <-s.nodesub.GetMessage():
				if !ok {
					klog.Errorf("node subscribe had close")
					return
				}
				origin, ok := msg.Data.(map[string]sets.Set[string])
				if !ok {
					klog.Errorf("node subscribe data type is: %v, except: %v", reflect.TypeOf(msg.Data), "map[string]sets.Set[string]")
					break
				}
				klog.Infof("configmap controller receive event topic: %s", msg.Topic)
				var (
					changed []string
				)

				s.mu.Lock()
				for k, v := range origin {
					oldv, ok := s.data[k]
					if !ok || !v.Equal(oldv) {
						changed = append(changed, k)
						s.data[k] = v.Clone()
					}
				}
				// remove not exist host
				if len(s.data) > len(origin) {
					for k := range s.data {
						if _, ok := origin[k]; !ok {
							changed = append(changed, k)
							delete(s.data, k)
						}
					}
				}
				s.mu.Unlock()
				if len(changed) > 0 {
					klog.Infof("host changed: %v, will trigger reconcile", changed)
					s.triggerch <- struct{}{}
				}
			case _, ok := <-s.triggerch:
				if !ok {
					return
				}
				err := s.syncConfigmap(ctx)

				if err != nil {
					klog.Errorf("configmap sync failed: %v, will retry", err)
					go func() {
						<-time.After(RequeuDelay)
						s.triggerch <- struct{}{}
					}()
				}
				klog.Infof("configmap(%s/%s) sync success", s.config.Namespace, s.config.Name)
			}
		}
	}()
	return nil
}

func (s *ConfigMapSync) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
	var (
		err     error
		cm      = &corev1.ConfigMap{}
		trigger = false
		nsname  = types.NamespacedName{Name: s.config.Name, Namespace: s.config.Namespace}
	)
	if len(s.data) == 0 {
		klog.Warningf("ecsnode not sync yet, configmap(%s) will not sync", nsname)
		return ctrl.Result{}, nil
	}
	klog.V(2).Infof("start configmap reconcile: %v", req.NamespacedName)
	defer func() {
		if trigger {
			klog.Infof("configmap(%s) changed, will trigger reconcile", ctrclient.ObjectKeyFromObject(cm))
			s.triggerch <- struct{}{}
		}
	}()
	if err = s.client.Get(ctx, nsname, cm); err != nil {
		klog.Errorf("get configmap %s failed: %v", nsname, err)
		trigger = true
		return ctrl.Result{}, nil
	}
	if cm.DeletionTimestamp != nil {
		trigger = true
		return ctrl.Result{}, nil
	}
	var (
		current = getIpSetFromConfigmap(cm)
	)

	s.mu.RLock()
	defer s.mu.RUnlock()
	if !deepEqualSets(current, s.data) {
		trigger = true
	}

	return ctrl.Result{}, nil
}

// syncconfigmap creates or updates configmap
func (s *ConfigMapSync) syncConfigmap(ctx context.Context) error {
	var (
		err         error
		desiredInfo = &corev1.ConfigMap{ObjectMeta: v1.ObjectMeta{
			Name:      s.config.Name,
			Namespace: s.config.Namespace,
		}}
	)
	result, err := controllerutil.CreateOrUpdate(ctx, s.client, desiredInfo, func() error {
		s.mu.RLock()
		defer s.mu.RUnlock()

		if !deepEqualSets(getIpSetFromConfigmap(desiredInfo), s.data) {
			desiredInfo.Data = map[string]string{}
			for k, v := range s.data {
				desiredInfo.Data[k] = strings.Join(v.UnsortedList(), ",")
			}
		}
		return nil
	})
	if err != nil {
		klog.Errorf("create or patch configmap failed: %v", err)
		return err
	}
	klog.Infof("create or patch configmap: %s, result: %v", ctrclient.ObjectKeyFromObject(desiredInfo), result)

	return nil
}

func getIpSetFromConfigmap(cm *corev1.ConfigMap) map[string]sets.Set[string] {
	var (
		current = make(map[string]sets.Set[string])
	)
	for k, v := range cm.Data {
		vlist := strings.Split(v, ",")
		ipset := sets.New[string]()
		for _, ip := range vlist {
			ipset.Insert(strings.TrimSpace(ip))
		}
		current[k] = ipset
	}
	return current
}

func deepEqualSets(s1, s2 map[string]sets.Set[string]) bool {

	if len(s1) != len(s2) {
		return false
	}

	if len(s1) == 0 && len(s2) == 0 {
		return true
	}

	for key, set1 := range s1 {
		set2, exists := s2[key]
		if !exists {
			return false
		}

		if !set1.Equal(set2) {
			return false
		}
	}

	for key := range s2 {
		if _, exists := s1[key]; !exists {
			return false
		}
	}

	return true
}
