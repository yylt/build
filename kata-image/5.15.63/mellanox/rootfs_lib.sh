# kernel must 5.15.63 version 

melx_build() {
	local rootfs_dir=$1

	local script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"


	rootfstmp="${rootfs_dir}/tmp"

	mkdir -p "${rootfstmp}" 

	# 拷贝 install.sh
	cp "$script_dir/install.sh" "$rootfstmp"

	make dep-pkg
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
source=https://mirrors.tuna.tsinghua.edu.cn/ubuntu-ports/
keyring=ubuntu-keyring
components=main restricted universe multiverse
suite=jammy
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
