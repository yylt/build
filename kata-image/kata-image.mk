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

CONTAINERD_SHIM_NAME ?= containerd-shim-ecr-v2
ECR_RUNTIME ?= ecr-runtime
CONFIG_FILE_NAME ?= configuration.toml
SOURCE_DIR ?= ../src/runtime
AGENT_DIR ?= ../src/agent
REGISTRY_NAME ?= ghcr.io
IMAGE_NAME ?= $(REGISTRY_NAME)/yylt/amd64-ecr-deploy

LAST_COMMIT_ID ?= $(shell git rev-parse HEAD)
IMAGE_TAG ?= v$(shell cat ../VERSION)-$(LAST_COMMIT_ID)

ifeq ($(shell arch),aarch64)
	# aarch64 环境重新命名 IMAGE_NAME 结构
	IMAGE_NAME = $(REGISTRY_NAME)/yylt/arm64-ecr-deploy
endif




all: generate-config build-image build-kernel ecr-runtime containerd-shim-v2 docker-push

containerd-shim-v2:
ifeq ($(shell arch),x86_64)
	make -C $(SOURCE_DIR) containerd-shim-v2
	mv $(SOURCE_DIR)/containerd-shim-kata-v2 $(CONTAINERD_SHIM_NAME)
endif


ifeq ($(shell arch),aarch64)
	make -C $(SOURCE_DIR) containerd-shim-v2; \
	mv $(SOURCE_DIR)/containerd-shim-kata-v2 $(CONTAINERD_SHIM_NAME)
endif

ecr-runtime:
ifeq ($(shell arch),x86_64)
	make -C $(SOURCE_DIR) runtime
	mv $(SOURCE_DIR)/kata-runtime $(ECR_RUNTIME)
endif

ifeq ($(shell arch),aarch64)
	make -C $(SOURCE_DIR) runtime
	mv $(SOURCE_DIR)/kata-runtime $(ECR_RUNTIME)
endif

# 根据架构不同生成安全容器配置文件
generate-config:
ifeq ($(shell arch),x86_64)
	# make -C $(SOURCE_DIR) clean
	SKIP_GO_VERSION_CHECK=y make -C $(SOURCE_DIR) config/configuration-qemu.toml \
	QEMUPATH="/usr/libexec/qemu-kvm" \
	KERNELPATH="/usr/share/ecr/vmlinuz.container" \
	IMAGEPATH="/usr/share/ecr/ecr-containers.img" \
	QEMUVALIDHYPERVISORPATHS='[\"/usr/libexec/qemu-kvm\"]' \
	CPUFEATURES="pmu=off,vmx=off" \
	DEFMEMSZ="2048" \
	DEFVIRTIOFSDAEMON="/usr/libexec/virtiofsd" \
	DEFVALIDVIRTIOFSDAEMONPATHS='[\"/usr/libexec/virtiofsd\"]' \
	DEFSANDBOXCGROUPONLY="true" \
	DEFENABLEANNOTATIONS='[\"default_vcpus\",\"guest_hook_path\",\"kernel\",\"image\",\"enable_sriov\",\"scsi_scan_mod\",\"hotplug_vfio_on_root_bus\",\"pcie_root_port\",\"sandbox_cgroup_only\",\"read_only_rootfs\"]'
	mv $(SOURCE_DIR)/config/configuration-qemu.toml configuration.toml
endif

ifeq ($(shell arch),aarch64)
	# make -C $(SOURCE_DIR) clean
	SKIP_GO_VERSION_CHECK=y make -C $(SOURCE_DIR) config/configuration-qemu.toml \
	QEMUPATH="/usr/libexec/qemu-kvm" \
	KERNELPATH="/usr/share/ecr/vmlinuz.container" \
	IMAGEPATH="/usr/share/ecr/ecr-containers.img" \
	QEMUVALIDHYPERVISORPATHS='[\"/usr/libexec/qemu-kvm\"]' \
	CPUFEATURES="pmu=off" \
	DEFMEMSZ="2048" \
	DEFVIRTIOFSDAEMON="/usr/libexec/virtiofsd" \
	DEFVALIDVIRTIOFSDAEMONPATHS='[\"/usr/libexec/virtiofsd\"]' \
	DEFSANDBOXCGROUPONLY="true" \
	DEFAULTPFLASHES='[\"/usr/share/ecr/ecr-flash0.img\", \"/usr/share/ecr/ecr-flash1.img\"]' \
	DEFAULTDISABLEIMAGENVDIMM="true" \
	DEFENABLEANNOTATIONS='[\"default_vcpus\",\"kernel\",\"image\",\"scsi_scan_mod\",\"hotplug_vfio_on_root_bus\",\"pcie_root_port\",\"sandbox_cgroup_only\",\"read_only_rootfs\"]'
	mv $(SOURCE_DIR)/config/configuration-qemu.toml configuration.toml
endif

package:
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
		glibc-tools \
		libseccomp-dev \
		curl wget sudo \
		bc

build-kernel: package

ifeq ($(shell arch),x86_64)
	cd ../tools/packaging/kernel; \
	sudo -E ./build-kernel.sh -v 5.15.63 -g nvidia -f -d setup; \
	sudo -E ./build-kernel.sh -v 5.15.63 -g nvidia -f -d build; \
	sudo -E ./build-kernel.sh -v 5.15.63 -g nvidia -f -d install; \
	sudo -E cp /usr/share/kata-containers/vmlinux-5.15.63-96-nvidia-gpu ../../../ecr_deploy/vmlinuz.container
endif

ifeq ($(shell arch),aarch64)
	cd ../tools/packaging/kernel; \
	sudo -E echo "" >> configs/fragments/arm64/base.conf; \
	sudo -E echo "CONFIG_NFS_FS=y" >> configs/fragments/arm64/base.conf; \
	sudo -E echo "CONFIG_NFS_V4=y" >> configs/fragments/arm64/base.conf; \
	sudo -E echo "CONFIG_NFS_V3=y" >> configs/fragments/arm64/base.conf; \
	sudo -E echo "CONFIG_NFS_V4_1=y" >> configs/fragments/arm64/base.conf; \
	sudo -E echo "CONFIG_NFS_COMMON=y" >> configs/fragments/arm64/base.conf; \
	sudo -E echo "CONFIG_NFSD=y" >> configs/fragments/arm64/base.conf; \
	sudo -E echo "CONFIG_NFSD_V3=y" >> configs/fragments/arm64/base.conf; \
	sudo -E echo "CONFIG_NFSD_V4=y" >> configs/fragments/arm64/base.conf; \
	sudo -E echo "" >> configs/fragments/arm64/base.conf; \
	sudo -E ./build-kernel.sh -v 5.15.63 -f -d setup; \
	sudo -E ./build-kernel.sh -v 5.15.63 -f -d build; \
	sudo -E ./build-kernel.sh -v 5.15.63 -f -d install; \
	sudo -E cp /usr/share/kata-containers/vmlinuz.container ../../../ecr_deploy/vmlinuz.container
endif


build-image: agent
ifeq ($(shell arch),x86_64)
	sudo -E apt install -y qemu-utils multistrap; \
	dir=$(PWD); \
	cd ../tools/osbuilder; \
	sed -i '27d' rootfs-builder/ubuntu/Dockerfile.in; \
	export USE_DOCKER=true; \
	export LIBC=gnu; \
	export EXTRA_PKGS="chrony coreutils gcc make curl gnupg  apt tar kmod nfs-common pkg-config libc-dev libc6-dev pciutils bridge-utils iproute2 iputils-ping iputils-arping"; \
	export ROOTFS_DIR="$${dir}/../tools/osbuilder/rootfs-builder/rootfs"; \
	export AGENT_SOURCE_BIN="$${dir}/agent/target/x86_64-unknown-linux-gnu/release/kata-agent"; \
	cd rootfs-builder; \
	sudo -E ./rootfs.sh ubuntu; \
	cd ..; \
	mkdir -p rootfs-builder/rootfs/etc/systemd/system; \
	sudo -E cp ../../src/agent/kata-agent.service rootfs-builder/rootfs/etc/systemd/system/; \
	sudo -E cp ../../src/agent/kata-containers.target rootfs-builder/rootfs/etc/systemd/system/; \
	sudo -E ./image-builder/image_builder.sh rootfs-builder/rootfs; \
	sudo -E cp kata-containers.img ../../ecr_deploy/ecr-containers.img
endif

ifeq ($(shell arch),aarch64) 
	sudo -E apt install -y qemu-utils multistrap; \
	dir=$(PWD); \
	cd ../tools/osbuilder; \
	sed -i '27d' rootfs-builder/ubuntu/Dockerfile.in; \
	export USE_DOCKER=true; \
	export LIBC=gnu; \
	export EXTRA_PKGS="chrony coreutils gcc make curl gnupg  apt tar nfs-common kmod pkg-config libc-dev libc6-dev pciutils bridge-utils iproute2 iputils-ping iputils-arping"; \
	export ROOTFS_DIR="$${dir}/../tools/osbuilder/rootfs-builder/rootfs"; \
	export AGENT_SOURCE_BIN="$${dir}/agent/target/aarch64-unknown-linux-gnu/release/kata-agent"; \
	sudo -E ./rootfs-builder/rootfs.sh  ubuntu; \
	sudo -E mkdir -p rootfs-builder/rootfs/etc/systemd/system; \
	sudo -E cp ../../src/agent/kata-agent.service rootfs-builder/rootfs/etc/systemd/system/; \
	sudo -E cp ../../src/agent/kata-containers.target rootfs-builder/rootfs/etc/systemd/system/; \
	sudo -E ./image-builder/image_builder.sh rootfs-builder/rootfs; \
	sudo -E cp kata-containers.img ../../ecr_deploy/ecr-containers.img
endif


agent: package
ifeq ($(shell arch),x86_64)
	# make -C $(AGENT_DIR) clean
	rustup target add x86_64-unknown-linux-musl;
	LIBC=gnu make -C $(AGENT_DIR) kata-agent
	make -C $(AGENT_DIR) kata-agent.service
endif

ifeq ($(shell arch),aarch64)
	# 注意：需要提前 clone kata-containers/tests 项目
	# ../ci/install_musl.sh; \
	# ../ci/install_rust.sh; \
	# export PATH=$$PATH:"$$HOME/.cargo/bin"; \
	# rustup target add aarch64-unknown-linux-gnu; \
	# rustup component add rustfmt clippy; \
	LIBC=gnu make -C $(AGENT_DIR) SECCOMP=no kata-agent; \
	make -C $(AGENT_DIR) kata-agent.service
endif

docker-image:
	sudo -E docker build -t $(IMAGE_NAME):$(IMAGE_TAG) -f Dockerfile .

docker-push: docker-image
	sudo -E docker push $(IMAGE_NAME):$(IMAGE_TAG)

clean:
	rm -f $(CONTAINERD_SHIM_NAME) $(ECR_RUNTIME) $(CONFIG_FILE_NAME) ecr-containers.img vmlinuz.container