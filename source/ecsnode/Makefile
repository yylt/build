
BINARY_NAME := ecsnode


.PHONY: all
all: build

CONTROLLER_TOOLS_VERSION ?= v0.16.5
GOOS ?= $(shell go env GOOS)
GOARCH ?= $(shell go env GOARCH)

PLATFORMS :=linux/amd64 linux/arm64 


VERSION := $(shell git describe --tags --always || echo "dev")
BUILD_TIME := $(shell date -u '+%Y-%m-%d_%H:%M:%S')


GCFLAGS := -trimpath -m -l -dwarf=false
LDFLAGS := -s -w -X main.Version=$(VERSION) -X main.BuildTime=$(BUILD_TIME)


.PHONY: build-all
build-all:
	@for platform in $(PLATFORMS); do \
		GOOS=$${platform%/*} \
		GOARCH=$${platform#*/} \
		$(MAKE) build; \
	done


.PHONY: build
build: deps
	@echo "Building for $(GOOS)/$(GOARCH)..."
	@mkdir -p bin/$(GOOS)/$(GOARCH)
	@CGO_ENABLED=0 GOOS=$(GOOS) GOARCH=$(GOARCH) \
		go build \
		-gcflags="$(GCFLAGS)" \
		-ldflags="$(LDFLAGS)" \
		-o bin/$(GOOS)/$(GOARCH)/$(BINARY_NAME) \
		./cmd
	@echo "Build completed: bin/$(GOOS)/$(GOARCH)/$(BINARY_NAME)"

.PHONY: generate
generate:  controller-gen ## Generate code containing DeepCopy, DeepCopyInto, and DeepCopyObject method implementations.
	controller-gen object:headerFile="hack/boilerplate.go.txt" paths="./..."


.PHONY: clean
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf bin/
	@echo "Clean completed"

.PHONY: controller-gen
controller-gen:
	@which controller-gen && controller-gen --version | grep -q $(CONTROLLER_TOOLS_VERSION) || \
	@go install sigs.k8s.io/controller-tools/cmd/controller-gen@$(CONTROLLER_TOOLS_VERSION)

.PHONY: test
test:
	@go test -v -race ./...


.PHONY: deps
deps:
	@go mod download