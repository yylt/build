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
		package \
		clean


CONTAINERD_SHIM_NAME ?= ecr_deploy/containerd-shim-ecr-v2
ECR_RUNTIME ?= ecr_deploy/ecr-runtime
CONFIG_FILE_NAME ?= ecr_deploy/configuration.toml
SOURCE_DIR ?= src/runtime
AGENT_DIR ?= src/agent
REGISTRY_NAME ?= ghcr.io

LAST_COMMIT_ID ?= $(shell git rev-parse HEAD)
IMAGE_TAG ?= v$(shell cat ./VERSION)-$(LAST_COMMIT_ID)

TARGET ?= "" # kunlun, nvidia, mellanox or ecr

ifeq ($(shell arch),aarch64)
	IMAGE_NAME ?= $(REGISTRY_NAME)/arm64v8/ecr-deploy
else
	IMAGE_NAME ?= $(REGISTRY_NAME)/captain/ecr-deploy
endif


all: generate-config build-kernel build-image ecr-runtime containerd-shim-v2 docker-push 

.PHONY: target
target: 
	chmod +x ../kata-image/build.sh
	sudo -E ../kata-image/build.sh -k "5.15.63" -s "$(PWD)" -t "$(TARGET)"

containerd-shim-v2:
	make -C $(SOURCE_DIR) containerd-shim-v2; \
	mv $(SOURCE_DIR)/containerd-shim-kata-v2 $(CONTAINERD_SHIM_NAME)

ecr-runtime:
	make -C $(SOURCE_DIR) runtime
	mv $(SOURCE_DIR)/kata-runtime $(ECR_RUNTIME)


# 根据架构不同生成安全容器配置文件
generate-config:
ifeq ($(shell arch),x86_64)
	# make -C $(SOURCE_DIR) clean
	make -C $(SOURCE_DIR) config/configuration-qemu.toml \
	SKIP_GO_VERSION_CHECK=y \
	QEMUPATH="/usr/libexec/qemu-kvm" \
	KERNELPATH="/usr/share/ecr/vmlinuz.container" \
	IMAGEPATH="/usr/share/ecr/ecr-containers.img" \
	QEMUVALIDHYPERVISORPATHS='[\"/usr/libexec/qemu-kvm\"]' \
	CPUFEATURES="pmu=off,vmx=off" \
	DEFMEMSZ="2048" \
	MACHINETYPE="q35" \
	DEFVIRTIOFSDAEMON="/usr/libexec/virtiofsd" \
	DEFVALIDVIRTIOFSDAEMONPATHS='[\"/usr/libexec/virtiofsd\"]' \
	DEFSANDBOXCGROUPONLY="true" \
	DEFAULTPFLASHES='[]' \
	IOMMU='true' \
	IOMMUPLATFORM='true' \
	DEFENABLEANNOTATIONS='[\"default_vcpus\",\"guest_hook_path\",\"kernel\",\"kernel_params\",\"image\",\"enable_sriov\",\"scsi_scan_mod\",\"enable_iommu\",\"hotplug_vfio_on_root_bus\",\"pcie_root_port\",\"sandbox_cgroup_only\",\"read_only_rootfs\"]'
	mv $(SOURCE_DIR)/config/configuration-qemu.toml $(CONFIG_FILE_NAME)
endif

ifeq ($(shell arch),aarch64)
	# make -C $(SOURCE_DIR) clean
	make -C $(SOURCE_DIR) config/configuration-qemu.toml \
	SKIP_GO_VERSION_CHECK=y \
	QEMUPATH="/opt/ecr/bin/qemu-system-aarch64" \
	KERNELPATH="/usr/share/ecr/vmlinuz.container" \
	IMAGEPATH="/usr/share/ecr/ecr-containers.img" \
	QEMUVALIDHYPERVISORPATHS='[\"/usr/libexec/qemu-kvm\", \"/opt/ecr/bin/qemu-system-aarch64\"]' \
	CPUFEATURES="pmu=off" \
	DEFMEMSZ="2048" \
	MACHINETYPE="virt" \
	DEFVIRTIOFSDAEMON="/usr/libexec/virtiofsd" \
	DEFVALIDVIRTIOFSDAEMONPATHS='[\"/usr/libexec/virtiofsd\"]' \
	DEFSANDBOXCGROUPONLY="true" \
	DEFAULTPFLASHES='[\"/usr/share/ecr/ecr-flash0.img\", \"/usr/share/ecr/ecr-flash1.img\"]' \
	DEFAULTDISABLEIMAGENVDIMM="true" \
	IOMMU='false' \
	IOMMUPLATFORM='false' \
	DEFENABLEANNOTATIONS='[\"default_vcpus\",\"guest_hook_path\",\"kernel\",\"kernel_params\",\"image\",\"scsi_scan_mod\",\"enable_iommu\",\"enable_sriov\",\"hotplug_vfio_on_root_bus\",\"pcie_root_port\",\"sandbox_cgroup_only\",\"read_only_rootfs\"]'
	mv $(SOURCE_DIR)/config/configuration-qemu.toml $(CONFIG_FILE_NAME)
endif


package:
	sudo -E apt update 
	sudo -E apt install -y \
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
		libfl-dev \
		musl-tools \
		libseccomp-dev \
		curl wget sudo \
		bc
	sudo -E mkdir -p ~/.cargo/
	sudo -E cp ../kata-image/cargo-config.toml ~/.cargo/config
	
build-kernel: package
ifeq ($(shell arch),aarch64)
	cp -a tools/packaging/kernel/patches/5.15.x/arm-experimental/* tools/packaging/kernel/patches/5.15.x/; \
	rm tools/packaging/kernel/patches/5.15.x/no_patch* -f
endif
	cp ../kata-image/5.15.63/configs/* tools/packaging/kernel/configs/fragments/common/
	cd ./tools/packaging/kernel; \
	sudo -E ./build-kernel.sh -v 5.15.63 -f -d setup; \
	sudo -E ./build-kernel.sh -v 5.15.63 -f -d install; \
	sudo -E cp /usr/share/kata-containers/vmlinuz.container  ../../../ecr_deploy/vmlinuz.container

build-image: agent
	dir=$(PWD); \
	arch=$(shell uname -m); \
	cd ./tools/osbuilder; \
	sed -i '27,29d' rootfs-builder/ubuntu/Dockerfile.in; \
	export USE_DOCKER=true; \
	export SECCOMP=no; \
	export LIBC=gnu; \
	export DEBUG=true; \
	export EXTRA_PKGS="gcc make curl gnupg coreutils apt tar kmod pkg-config libc-dev libc6-dev pciutils coreutils curl tar nfs-common pciutils bridge-utils iproute2 iputils-ping iputils-arping"; \
	export ROOTFS_DIR="$${dir}/tools/osbuilder/rootfs-builder/rootfs"; \
	export AGENT_SOURCE_BIN="$${dir}/src/agent/target/$$arch-unknown-linux-gnu/release/kata-agent"; \
	sudo -E ./rootfs-builder/rootfs.sh  ubuntu; \
	sudo -E mkdir -p rootfs-builder/rootfs/etc/systemd/system; \
	sudo -E cp ../../src/agent/kata-agent.service rootfs-builder/rootfs/etc/systemd/system/; \
	sudo -E cp ../../src/agent/kata-containers.target rootfs-builder/rootfs/etc/systemd/system/; \
	sudo -E ./image-builder/image_builder.sh $${dir}/tools/osbuilder/rootfs-builder/rootfs; \
	sudo -E cp kata-containers.img ../../ecr_deploy/ecr-containers.img


agent: package
	LIBC=gnu DEBUG=true SECCOMP=no make -C $(AGENT_DIR) kata-agent; \
	make -C $(AGENT_DIR) kata-agent.service

docker-image:
	cd ecr_deploy; \
	sudo -E docker build -t $(IMAGE_NAME):$(IMAGE_TAG) -f Dockerfile .

docker-push: docker-image
	sudo -E docker push $(IMAGE_NAME):$(IMAGE_TAG)

clean:
	rm -f $(CONTAINERD_SHIM_NAME) $(ECR_RUNTIME) $(CONFIG_FILE_NAME) ecr-containers.img vmlinuz.container
