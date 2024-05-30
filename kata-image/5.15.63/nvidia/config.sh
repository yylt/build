# Copyright (c) 2024 Yylt.
#
# SPDX-License-Identifier: Apache-2.0

OS_NAME=ubuntu
OS_VERSION=jammy

PACKAGES="apt apt-utils bash bison build-essential bzip2 ca-certificates chrony coreutils \
curl dpkg dkms dialog ethtool flex gcc iptables kmod libc-dev libc6-dev make pciutils pkg-config tar"

[ "$AGENT_INIT" = no ] && PACKAGES+=" init"

REPO_URL=http://ports.ubuntu.com

case "$ARCH" in
	aarch64) DEB_ARCH=arm64; REPO_URL=https://mirrors.tuna.tsinghua.edu.cn/ubuntu-ports ;;
	ppc64le) DEB_ARCH=ppc64el;;
	s390x) DEB_ARCH="$ARCH";;
	x86_64) DEB_ARCH=amd64; 
		# REPO_URL=http://archive.ubuntu.com/ubuntu 
		REPO_URL=https://mirrors.tuna.tsinghua.edu.cn/ubuntu
	;;
	*) die "$ARCH not supported"
esac

if [ "$(uname -m)" != "$ARCH" ]; then
	case "$ARCH" in
		ppc64le) cc_arch=powerpc64le;;
		x86_64) cc_arch=x86-64;;
		*) cc_arch="$ARCH"
	esac
	export CC="$cc_arch-linux-gnu-gcc"
fi
