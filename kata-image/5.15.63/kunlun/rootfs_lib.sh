# kernel must 5.15.63 version 
kunlun_build() {
	local rootfs_dir=$1

	local script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

	rootfstmp="${rootfs_dir}/tmp"
	modfs="${rootfs_dir}/lib/modules/5.15.63/extra"
	rootfslib64="${rootfs_dir}/usr/local/xpu/lib64"
	rulesdir="${rootfs_dir}/etc/udev/rules.d/"

	mkdir -p "${rootfstmp}" "${rootfslib64}" "${modfs}" "${rulesdir}"
	
	
	# 拷贝 deb
	cp  $script_dir/xpu-container-toolkit/libxpu-container0_0.1.0_arm64.deb "$rootfstmp"
	cp  $script_dir/xpu-container-toolkit/libxpu-container-tools_0.1.0_arm64.deb "$rootfstmp"
	cp  $script_dir/xpu-container-toolkit/xpu-container-toolkit_0.1.0~rc.1-1_arm64.deb "$rootfstmp"

	# 拷贝 so,ko,rule
	cp ${script_dir}/xre_linux-headers-5.15.63-arm64_4.0.25/so/libxpuml.so* "${rootfslib64}"
	cp ${script_dir}/xre_linux-headers-5.15.63-arm64_4.0.25/kunlun_module/kunlun-5.15.63.ko  "${modfs}/kunlun.ko"
	cp ${script_dir}/xre_linux-headers-5.15.63-arm64_4.0.25//udev_rules/99-kunlun.rules "${rulesdir}"

	# 拷贝 install.sh
	cp "$script_dir/install.sh" "$rootfstmp"

	chroot "${rootfs_dir}" /usr/bin/bash "/tmp/install.sh"

	# 清理
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
suite=jammy
packages=$PACKAGES $EXTRA_PKGS 
EOF
	multistrap -a "$DEB_ARCH" -d "$rootfs_dir" -f "$multistrap_conf"
	kunlun_build "$rootfs_dir"
	rm -rf "$rootfs_dir/var/run"
	ln -s /run "$rootfs_dir/var/run"
	cp --remove-destination /etc/resolv.conf "$rootfs_dir/etc"

	# Reduce image size and memory footprint by removing unnecessary files and directories.
	rm -rf $rootfs_dir/usr/share/{bash-completion,bug,doc,info,lintian,locale,man,menu,misc,pixmaps,terminfo,zsh}
}
