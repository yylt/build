package main

import (
	"flag"
	"os"
	"path/filepath"

	"k8s.io/client-go/kubernetes"
	restclient "k8s.io/client-go/rest"
	"k8s.io/client-go/tools/clientcmd"
)

func MustGetConfig() *restclient.Config {
	var (
		inCluster  bool
		kubeconfig *string
		config     *restclient.Config
		err        error
	)
	home := os.Getenv("HOME")
	if home != "" {
		kubeconfig = flag.String("kubeconfig", filepath.Join(home, ".kube", "config"), "(optional) absolute path to the kubeconfig file")
	} else {
		kubeconfig = flag.String("kubeconfig", "", "absolute path to the kubeconfig file")
	}
	flag.Parse()
	if *kubeconfig != "" {
		_, err := os.Stat(*kubeconfig)
		if err != nil {
			inCluster = true
		}
	}

	if inCluster {
		config, err = restclient.InClusterConfig()
		if err != nil {
			panic(err.Error())
		}
	} else {
		// use the current context in kubeconfig
		config, err = clientcmd.BuildConfigFromFlags("", *kubeconfig)
		if err != nil {
			panic(err.Error())
		}
	}
	return config

}

func main() {
	config := MustGetConfig()

	// create the clientset
	clientset, err := kubernetes.NewForConfig(config)
	if err != nil {
		panic(err.Error())
	}
}
