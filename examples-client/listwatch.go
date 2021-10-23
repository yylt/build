package examplesclient

import (
	"context"

	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/apimachinery/pkg/fields"
	"k8s.io/apimachinery/pkg/labels"
	"k8s.io/apimachinery/pkg/runtime"
	"k8s.io/apimachinery/pkg/selection"
	"k8s.io/apimachinery/pkg/watch"

	"k8s.io/client-go/kubernetes"
	"k8s.io/client-go/tools/cache"
)

func GetListWatcher(lw *ListWatch) cache.ListerWatcher {
	return &cache.ListWatch{
		ListFunc:  lw.list,
		WatchFunc: lw.watch,
	}
}

type ListWatch struct {
	client   *kubernetes.Clientset
	labels   labels.Selector
	ns       string
	obj      runtime.Object
	fslector fields.Selector
}

func NewLW() *ListWatch {
	return &ListWatch{}
}

func (lw *ListWatch) WithClient(client *kubernetes.Clientset) *ListWatch {
	lw.client = client
	return lw
}

func (lw *ListWatch) WithFieldSelector(fieldSelector fields.Selector) *ListWatch {
	lw.fslector = fieldSelector
	return lw
}

func (lw *ListWatch) WithLabels(las map[string]string) *ListWatch {
	if las == nil {
		return lw
	}
	selector := labels.NewSelector()
	for k, v := range las {
		req, err := labels.NewRequirement(k, selection.Equals, []string{v})
		if err != nil {
			panic(err)
		}
		selector.Add(*req)
	}
	lw.labels = selector
	return lw
}

func (lw *ListWatch) WithNamespace(ns string) *ListWatch {
	lw.ns = ns
	return lw
}

func (lw *ListWatch) WithObject(obj runtime.Object) *ListWatch {
	lw.obj = obj
	return lw
}

func (lw *ListWatch) list(options metav1.ListOptions) (runtime.Object, error) {
	if lw.fslector != nil {
		options.FieldSelector = lw.fslector.String()
	}
	if lw.labels != nil {
		options.LabelSelector = lw.labels.String()
	}
	request := lw.client.RESTClient().Get()
	request.VersionedParams(&options, metav1.ParameterCodec)
	if lw.ns != "" {
		request.Namespace(lw.ns)
	}

	return request.Do(context.TODO()).Get()
}

func (lw *ListWatch) watch(options metav1.ListOptions) (watch.Interface, error) {
	if lw.fslector != nil {
		options.FieldSelector = lw.fslector.String()
	}
	if lw.labels != nil {
		options.LabelSelector = lw.labels.String()
	}
	request := lw.client.RESTClient().Get()
	request.VersionedParams(&options, metav1.ParameterCodec)
	if lw.ns != "" {
		request.Namespace(lw.ns)
	}

	return request.Watch(context.TODO())
}
