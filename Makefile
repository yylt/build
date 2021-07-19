
ARCH ?= 
PROJECT ?=

all: check

k8s: check
	KUBE_BUILD_PLATFORMS=linux/${ARCH} make WHAT=cmd/hyperkube

check:
	cd ${PROJECT}
