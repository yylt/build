# Copyright (c) 2024 yylt.
#
# SPDX-License-Identifier: Apache-2.0

# notice: kernel must 5.15.63 version 

melx_build() {
	local rootfs_dir=$1

	local script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

	rootfstmp="${rootfs_dir}/tmp"

	mkdir -p "${rootfstmp}" 

	cp "$script_dir/install.sh" "$rootfstmp"


	# 准备文件,注意 MLNX文件和deb 需提前放到本目录
	tar xvf $script_dir/MLNX*-x86_64.tgz -C "$rootfstmp"
	cp $script_dir/linux*.deb "$rootfstmp"

	mount -t proc -o ro none $rootfs_dir/proc
	chroot "${rootfs_dir}" /usr/bin/bash "/tmp/install.sh"
	umount ${rootfs_dir}/proc

	rm -rf "${rootfstmp}"
}

build_rootfs() {
	local rootfs_dir=$1
	local multistrap_conf=multistrap.conf

	# For simplicity's sake, use multistrap for foreign and native bootstraps.
	cat > "$multistrap_conf" << EOF
[General]
cleanup=true
aptsources=Ubuntu
bootstrap=Ubuntu

[Ubuntu]
source=$REPO_URL
keyring=ubuntu-keyring
components=main restricted universe multiverse
suite=jammy jammy-updates
packages=$PACKAGES $EXTRA_PKGS 
EOF

	multistrap -a "$DEB_ARCH" -d "$rootfs_dir" -f "$multistrap_conf"
	
	melx_build "$rootfs_dir"
	rm -rf "$rootfs_dir/var/run"
	ln -s /run "$rootfs_dir/var/run"
	cp --remove-destination /etc/resolv.conf "$rootfs_dir/etc"

	# Reduce image size and memory footprint by removing unnecessary files and directories.
	rm -rf $rootfs_dir/usr/share/{bash-completion,bug,doc,info,lintian,locale,man,menu,misc,pixmaps,terminfo,zsh}
}
