#!/bin/bash

set -ex

readonly bk_script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly bk_script_name="$(basename "${BASH_SOURCE[0]}")"


build_target="" # 支持 kunlun，mellanox，nvidia 和 空
default_target="kata"

kernel_version="5.15.63" # 默认 5.15.63

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
    if [ -d "$patch_dir" ]; then
        die "kata patch path should exist"
    fi 
    if [ -d "$config_dir" ]; then
        die "kata config path should exist"
    fi 

    info "kata src path ${kata_src_path}"
    info "kata patch path ${patch_dir}"
    info "kata config path ${config_dir}"
}

kernel() {
    # 拷贝 config 
    if [ -d "${bk_script_dir}/${kernel_version}/configs" ]; then
        find "${bk_script_dir}/${kernel_version}/configs" -type f -name "common*" -exec cp {} "${config_dir}/fragments/common/" \;
    fi 

    # 架构相关 patch
    case "$(uname -m)" in
		aarch64) 
          if [ -d "${patch_dir}/arm-experimental" ]; then  
            cp -f ${patch_dir}/arm-experimental/* ${patch_dir}/
            rm -f ${patch_dir}/no_patches.txt
          fi
        ;;
		*) echo "nothing" ;;
	esac

    # 目标相关 patch
    if [ -d "${bk_script_dir}/${kernel_version}/${build_target}/patchs" ]; then
        find "${bk_script_dir}/${kernel_version}/${build_target}/patchs" -type f -exec cp {} "${patch_dir}/" \;
    fi

    # 编译脚本
    local script="${kata_src_path}/tools/packaging/kernel/build_kernel.sh"

    # gpu 相关配置在 sriov.conf 中，不再需要
    "$script" -v $kernel_version -f -d setup
    "$script" -v $kernel_version -f -d install
    image="vmlinuz.container"

    cp -L "/usr/share/kata-containers/${image}" "${bk_script_dir}/${arch_target}/${build_target}-vmlinuz.container"

    # 准备 deb 
    case "$build_target" in
		mellanox|nvidia)
          echo "build deb, build target is: ${build_target}"
          make deb-pkg -C ${kata_src_path}/kata-linux-${kernel_version}-96
        ;;
		*) echo "no build deb" ;;
	esac
}


image() {
    # 准备 rootfs 
    if [[ $build_target != $default_target ]]; then
        cp -af ${bk_script_dir}/${kernel_version}/${build_target} ${kata_src_path}/tools/osbuilder/rootfs-builder/
    fi 

    # 准备 env
	export USE_DOCKER=true
	export LIBC=gnu
	export DEBUG=true
	export ROOTFS_DIR="${kata_src_path}/tools/osbuilder/rootfs-builder/rootfs"
	export AGENT_SOURCE_BIN="${kata_src_path}/src/agent/target/$(uname -m)-unknown-linux-gnu/release/kata-agent"

    case "$build_target" in
        kunlun)
            export EXTRA_PKGS="coreutils apt tar nfs-common pciutils bridge-utils iproute2 iputils-ping iputils-arping"
            target="kunlun"
        ;;
		mellanox)
            export EXTRA_PKGS="chrony coreutils gcc make curl gnupg apt tar nfs-common kmod pkg-config libc-dev libc6-dev pciutils bridge-utils iproute2 iputils-ping iputils-arping"
            target="mellanox"
        ;;
        nvidia)
            target="nvidia"
        ;;
		*) 
            export EXTRA_PKGS="coreutils apt tar nfs-common pciutils bridge-utils iproute2 iputils-ping iputils-arping"
            # 删除错误或不必要
            
	        sed -i '27d' ${kata_src_path}/tools/osbuilder/rootfs-builder/ubuntu/Dockerfile.in
            sed -i '29d' ${kata_src_path}/tools/osbuilder/rootfs-builder/ubuntu/Dockerfile.in

            target="ubuntu"
        ;;
	esac
    ${kata_src_path}/tools/osbuilder/rootfs-builder/rootfs.sh  $target
	mkdir -p ${kata_src_path}/tools/osbuilder/rootfs-builder/rootfs/etc/systemd/system
	cp ${kata_src_path}/src/agent/kata-agent.service ${kata_src_path}/tools/osbuilder/rootfs-builder/rootfs/etc/systemd/system/
	cp ${kata_src_path}/src/agent/kata-containers.target ${kata_src_path}/tools/osbuilder/rootfs-builder/rootfs/etc/systemd/system/
	${kata_src_path}/tools/osbuilder/image-builder/image_builder.sh ${kata_src_path}/tools/osbuilder/rootfs-builder/rootfs

	cp ${kata_src_path}/tools/osbuilder/kata-containers.img ${bk_script_dir}/$target-containers.img
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

    if [[ "$build_target" == "" ]];then
        build_target=$default_target
    fi

	major_version=$(echo "${kernel_version}" | cut -d. -f1)

    if [[ $kata_src_path == "" ]]; then
        die "kata src path should not be null"
    fi

    # patchers 目录，拷贝 patch
    patch_dir="${kata_src_path}/tools/packaging/kernel/patches/${major_version}.x"
    

    # configs 目录
    config_dir="${kata_src_path}/tools/packaging/kernel/configs"

    # 校验
    check_valid

    info "build target is ${build_target}"
    kernel && image
}

main $@
