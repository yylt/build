.PHONY: containerd-shim-v2 \
		ecr-runtime \
		kernel \
		image \
		generate-config \
		build-kernel \
		build-image \
		docker-image \
		docker-push \
		agent \
		all \
		clean

CONTAINERD_SHIM_NAME = "containerd-shim-ecr-v2"
ECR_RUNTIME = "ecr-runtime"
CONFIG_FILE_NAME = "configuration.toml"
SOURCE_DIR = "./src/runtime"
AGENT_DIR = "./src/agent"
REGISTRY_NAME = "registry.yylt.tk:41072"
IMAGE_NAME = $(REGISTRY_NAME)/captain/ecr-deploy
LAST_COMMIT_ID = $(shell git rev-parse HEAD)
IMAGE_TAG = $(shell arch)-$(LAST_COMMIT_ID)

all: generate-config containerd-shim-v2 ecr-runtime build-kernel build-image docker-push

containerd-shim-v2:
	./ci/install_yq.sh
	make -C $(SOURCE_DIR) containerd-shim-v2
	mv $(SOURCE_DIR)/containerd-shim-kata-v2 $(CONTAINERD_SHIM_NAME)


ecr-runtime:
	make -C $(SOURCE_DIR) runtime
	mv $(SOURCE_DIR)/kata-runtime $(ECR_RUNTIME)

# 根据架构不同生成安全容器配置文件
generate-config:
ifeq ($(shell arch),x86_64)
	# make -C $(SOURCE_DIR) clean
	SKIP_GO_VERSION_CHECK=y make -C $(SOURCE_DIR) cli/config/configuration-qemu.toml \
	QEMUPATH="/usr/libexec/qemu-kvm" \
	KERNELPATH="/usr/share/ecr/vmlinuz.container" \
	IMAGEPATH="/usr/share/ecr/ecr-containers.img" \
	QEMUVALIDHYPERVISORPATHS='[\"/usr/libexec/qemu-kvm\"]' \
	CPUFEATURES="pmu=off,vmx=off" \
	DEFMEMSZ="2048" \
	DEFVIRTIOFSDAEMON="/usr/libexec/virtiofsd" \
	DEFVALIDVIRTIOFSDAEMONPATHS='[\"/usr/libexec/virtiofsd\"]' \
	DEFSANDBOXCGROUPONLY="true"
	mv $(SOURCE_DIR)/cli/config/configuration-qemu.toml configuration.toml
endif

ifeq ($(shell arch),aarch64)
	# make -C $(SOURCE_DIR) clean
	SKIP_GO_VERSION_CHECK=y make -C $(SOURCE_DIR) cli/config/configuration-qemu.toml \
	QEMUPATH="/usr/libexec/qemu-kvm" \
	KERNELPATH="/usr/share/ecr/vmlinuz.container" \
	IMAGEPATH="/usr/share/ecr/ecr-containers.img" \
	QEMUVALIDHYPERVISORPATHS='[\"/usr/libexec/qemu-kvm\"]' \
	CPUFEATURES="pmu=off,vmx=off" \
	DEFMEMSZ="2048" \
	DEFVIRTIOFSDAEMON="/usr/libexec/virtiofsd" \
	DEFVALIDVIRTIOFSDAEMONPATHS='[\"/usr/libexec/virtiofsd\"]' \
	DEFSANDBOXCGROUPONLY="true"
	mv $(SOURCE_DIR)/cli/config/configuration-qemu.toml configuration.toml
endif

build-kernel:
	sudo -E apt-get install -y \
		gcc \
		make \
		libncurses5-dev \
		openssl \
		libssl-dev \
		build-essential \
		pkg-config \
		libc6-dev \
		bison \
		flex \
		libelf-dev \
		bc
ifeq ($(shell arch),x86_64)
	cd ./tools/packaging/kernel; \
	sudo -E ./build-kernel.sh -v 5.10.25 -g nvidia -f -d setup; \
	sudo -E ./build-kernel.sh -v 5.10.25 -g nvidia -f -d build; \
	sudo -E ./build-kernel.sh -v 5.10.25 -g nvidia -f -d install; \
	cd ../../..; \
	sudo -E cp /usr/share/kata-containers/vmlinuz-nvidia-gpu.container ./ecr_deploy/vmlinuz.container
endif

ifeq ($(shell arch),aarch64)
	cd ./tools/packaging/kernel; \
	sudo -E ./build-kernel.sh -v 5.10.25 -f -d setup; \
	sudo -E ./build-kernel.sh -v 5.10.25 -f -d build; \
	sudo -E ./build-kernel.sh -v 5.10.25 -f -d install; \
	cd ../../..; \
	sudo -E cp /usr/share/kata-containers/vmlinuz.container ./ecr_deploy/vmlinuz.container
endif


build-image: agent
	cp ./src/agent/target/*/release/kata-agent /tmp/; \
	export IMAGE_REGISTRY="docker.io --network host"; \
	export AGENT_SOURCE_BIN="/tmp/kata-agent"; \
	export USE_DOCKER=true; \
	export DEBUG=true; \
	cd ./tools/osbuilder/rootfs-builder; \
	sudo -E ./rootfs.sh centos; \
	cd ..; \
	sudo -E cp ../../src/agent/kata-agent.service rootfs-builder/rootfs-Centos/usr/lib/systemd/system/; \
	sudo -E cp ../../src/agent/kata-containers.target rootfs-builder/rootfs-Centos/usr/lib/systemd/system/; \
	sudo -E ./image-builder/image_builder.sh rootfs-builder/rootfs-Centos; \
	sudo -E cp kata-containers.img ../../ecr_deploy/ecr-containers.img


agent:
ifeq ($(shell arch),aarch64)
	./ci/install_musl.sh
endif
	make -C $(AGENT_DIR) -e PATH=$$PATH:/usr/local/musl/bin kata-agent
	make -C $(AGENT_DIR) -e PATH=$$PATH:/usr/local/musl/bin kata-agent.service


docker-image:
ifeq ($(shell arch),x86_64)
	IMAGE_NAME=$(REGISTRY_NAME)/captain/ecr-deploy
endif
ifeq ($(shell arch),aarch64)
	IMAGE_NAME=$(REGISTRY_NAME)/arm64v8/ecr-deploy
endif	
	docker build -t $(IMAGE_NAME):$(IMAGE_TAG) -f ./ecr_deploy/Dockerfile ./ecr_deploy

docker-push: docker-image
ifeq ($(shell arch),x86_64)
	IMAGE_NAME=$(REGISTRY_NAME)/captain/ecr-deploy
endif
ifeq ($(shell arch),aarch64)
	IMAGE_NAME=$(REGISTRY_NAME)/arm64v8/ecr-deploy
endif	
	docker push $(IMAGE_NAME):$(IMAGE_TAG)


clean:
	rm -f $(CONTAINERD_SHIM_NAME) $(ECR_RUNTIME) $(CONFIG_FILE_NAME) ecr-containers.img vmlinuz.container
