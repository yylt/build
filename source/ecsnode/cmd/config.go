package main

import (
	"ecsnode/pkg/controller"
	"fmt"
	"os"

	"gopkg.in/yaml.v3"
)

type Config struct {
	Configmap     controller.ConfigMapConfig `json:"configmap" yaml:"configmap"`
	PodConfig     controller.PodConfig       `json:"pod" yaml:"pod"`
	EcsNode       controller.EcsNodeConfig   `json:"ecsnode" yaml:"ecsnode"`
	ServiceConfig controller.ServiceConfig   `json:"service" yaml:"service"`
}

var (
	Version, BuildTime string
)

// LoadConfigmap reads data from file-path
func LoadFromYaml(fp string) (*Config, error) {
	var (
		cfg = &Config{}
	)
	configmapBytes, err := os.ReadFile(fp)
	if nil != err {
		return nil, fmt.Errorf("failed to read config file %s, error: %v", fp, err)
	}

	err = yaml.Unmarshal(configmapBytes, &cfg)
	if nil != err {
		return nil, fmt.Errorf("failed to parse configmap, error: %v", err)
	}

	return cfg, nil
}

func ApplyDefault(newcfg *Config, defaultns string) error {

	if newcfg == nil {
		return fmt.Errorf("config is nil")
	}
	if newcfg.Configmap.Namespace == "" {
		newcfg.Configmap.Namespace = defaultns
	}
	if newcfg.PodConfig.Namespace == "" {
		newcfg.PodConfig.Namespace = defaultns
	}
	if newcfg.ServiceConfig.Namespace == "" {
		newcfg.ServiceConfig.Namespace = defaultns
	}
	if newcfg.EcsNode.Namespace == "" {
		newcfg.EcsNode.Namespace = defaultns
	}
	switch {
	case newcfg.EcsNode.Valid() != nil:
		return fmt.Errorf("invalid ecnsnode config: %v, failed: %v", newcfg.EcsNode, newcfg.EcsNode.Valid())
	case newcfg.Configmap.Valid() != nil:
		return fmt.Errorf("invalid configmap config: %v, failed: %v", newcfg.Configmap, newcfg.Configmap.Valid())
	case newcfg.PodConfig.Valid() != nil:
		return fmt.Errorf("invalid pod config: %v, failed: %v", newcfg.PodConfig, newcfg.PodConfig.Valid())
	case newcfg.ServiceConfig.Valid() != nil:
		return fmt.Errorf("invalid service config: %v, failed: %v", newcfg.ServiceConfig, newcfg.ServiceConfig.Valid())
	}
	return nil
}
