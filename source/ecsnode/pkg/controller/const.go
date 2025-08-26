package controller

import "time"

const (
	EcsNodeTopic = "/ecsnode"
	PodTopic     = "/pod"

	RequeuDelay = 5 * time.Second
)

const (
	ServiceAnnoNotReadyKey = "servicesync.io/publish_notready"
)
