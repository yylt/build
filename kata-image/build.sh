#!/bin/bash

set -ex

readonly bk_script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"


build_target="" # 支持 kunlun，mellanox，nvidia, ecr

kernel_version="5.15.63" # 默认 5.15.63

mlnx_download_url=$MLNX_X64_URL # from env
nvidia_download_url=$NVIDIA_X64_URL # from env

kata_src_path=""
patch_dir=""
config_dir=""

arch_target="$(uname -m)"

die()
{
	error "$*"
	exit 1
}

info()
{
	local msg="$*"
	echo "INFO: ${msg}"
}

check_valid() {
    if [[ $kata_src_path == "" ]]; then
        die "kata src path should not be null"
    fi 
    if [ ! -d "$patch_dir" ]; then
        die "kata patch path should exist"
    fi 
    if [ ! -d "$config_dir" ]; then
        die "kata config path should exist"
    fi 
    case "$build_target" in
		kunlun|mellanox|nvidia|ecr) 
            info "build target is ${build_target}"
        ;;
		*) 
            die "target must be one of 'kunlun|mellanox|nvidia|ecr' " 
        ;;
	esac

    info "kata src path ${kata_src_path}"
    info "kata patch path ${patch_dir}"
    info "kata config path ${config_dir}"
}

kernel() {
    # 已存在，则什么都不做
    if [ -f ${bk_script_dir}/${arch_target}-vmlinuz.container ]; then
        info "${bk_script_dir}/${arch_target}-vmlinuz.container exist, skip build image"
        return 
    fi

    build_deb=1

    # 拷贝 config 
    if [ -d "${bk_script_dir}/${kernel_version}/configs" ]; then
        find "${bk_script_dir}/${kernel_version}/configs" -type f -name "common*" -exec cp {} "${config_dir}/fragments/common/" \;
    fi 

    # 架构相关
    case "$arch_target" in
		aarch64) 
            if [ -d "${patch_dir}/arm-experimental" ]; then  
                cp -f ${patch_dir}/arm-experimental/* ${patch_dir}/
                rm -f ${patch_dir}/no_patches.txt
            fi

            ## kunlun相关 patchs, 当前有问题
            # if [ -d "${bk_script_dir}/${kernel_version}/patchs" ]; then
            #     find "${bk_script_dir}/${kernel_version}/patchs" -type f -exec cp {} "${patch_dir}/" \;
            # fi

            # arm64 当前不需要 deb
            build_deb=0
        ;;
	esac

    # 编译脚本
    local script="${kata_src_path}/tools/packaging/kernel/build-kernel.sh"

    # gpu 相关配置在 sriov.conf 中，不再需要
    "$script" -v $kernel_version -f -d setup
    "$script" -v $kernel_version -f -d install
    image="vmlinuz.container"

    cp -L "/usr/share/kata-containers/${image}" "${bk_script_dir}/${arch_target}-vmlinuz.container"

    # 准备 deb 
    if [[ $build_deb == 1 ]]; then
        apt-get install -y kmod cpio
        make deb-pkg -C ${kata_src_path}/kata-linux-${kernel_version}-96
    fi 
}


image() {
    # 已存在，则什么都不做
    if [ -f ${bk_script_dir}/$target-containers.img ]; then
        info "${bk_script_dir}/$target-containers.img exist, skip build image"
        return 
    fi

    # 准备 rootfs 
    cp -af ${bk_script_dir}/${kernel_version}/${build_target} ${kata_src_path}/tools/osbuilder/rootfs-builder/
    
    # 准备 env
	export USE_DOCKER=true
	export LIBC=gnu
	export DEBUG=true
    export SECCOMP=no
	export ROOTFS_DIR="${kata_src_path}/tools/osbuilder/rootfs-builder/rootfs"
	export AGENT_SOURCE_BIN="${kata_src_path}/src/agent/target/$(uname -m)-unknown-linux-gnu/release/kata-agent"
    export EXTRA_PKGS="bash curl nfs-common pciutils bridge-utils iproute2 iputils-ping iputils-arping"
    target=${build_target}
    case "$build_target" in
		mellanox)
            if [[ $mlnx_download_url == "" ]];then
                die "url for mlnx is null"
            fi
            filename=$(basename "$mlnx_download_url")
            curl -L ${mlnx_download_url} -o ${kata_src_path}/tools/osbuilder/rootfs-builder/mellanox/$filename
            cp -af ${kata_src_path}/linux*deb ${kata_src_path}/tools/osbuilder/rootfs-builder/mellanox/
        ;;
        nvidia)
            # 拷贝 linux header 
            cp -af ${kata_src_path}/linux*deb ${kata_src_path}/tools/osbuilder/rootfs-builder/nvidia/
            # 下载 nvidia driver
            if [[ $nvidia_download_url == "" ]];then
                die "url for nvidia is null"
            fi
            filename=$(basename "$nvidia_download_url")
            curl -L ${nvidia_download_url} -o ${kata_src_path}/tools/osbuilder/rootfs-builder/nvidia/$filename
            
            # 下载 mlnx driver
            if [[ $mlnx_download_url == "" ]];then
                die "url for mlnx is null"
            fi
            filename=$(basename "$mlnx_download_url")
            curl -L ${mlnx_download_url} -o ${kata_src_path}/tools/osbuilder/rootfs-builder/nvidia/$filename
        ;;
	esac

    ${kata_src_path}/tools/osbuilder/rootfs-builder/rootfs.sh  $target
    case "$build_target" in
		mellanox)
            sed -i "/ExecStart=/i ExecStartPre=-\/etc\/init.d\/openibd start" ${kata_src_path}/src/agent/kata-agent.service
        ;;
	esac

	mkdir -p ${kata_src_path}/tools/osbuilder/rootfs-builder/rootfs/etc/systemd/system
	cp ${kata_src_path}/src/agent/kata-agent.service ${kata_src_path}/tools/osbuilder/rootfs-builder/rootfs/etc/systemd/system/
	cp ${kata_src_path}/src/agent/kata-containers.target ${kata_src_path}/tools/osbuilder/rootfs-builder/rootfs/etc/systemd/system/
	${kata_src_path}/tools/osbuilder/image-builder/image_builder.sh ${kata_src_path}/tools/osbuilder/rootfs-builder/rootfs

	cp ${kata_src_path}/kata-containers.img ${bk_script_dir}/$target-containers.img
}


main() {
    local cmd=""
	while getopts "t:k:s:c:" opt; do	
		case "$opt" in
			t)
				build_target="${OPTARG}"
				;;
			k)
				kernel_version="${OPTARG}"
				;;
			s)
				kata_src_path="${OPTARG}"
				;;
			c)
				cmd="${OPTARG}"
				;;
		esac
	done

	major_version=$(echo "${kernel_version}" | cut -d. -f1,2)

    if [[ $kata_src_path == "" ]]; then
        die "kata src path should not be null"
    fi

    # patchers 目录，拷贝 patch
    patch_dir="${kata_src_path}/tools/packaging/kernel/patches/${major_version}.x"
    

    # configs 目录
    config_dir="${kata_src_path}/tools/packaging/kernel/configs"

    # 校验
    check_valid

    kernel && image
}

main $@

# debug
ls $bk_script_dir