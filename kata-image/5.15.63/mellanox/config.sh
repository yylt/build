# Copyright (c) 2024 Yylt.
#
# SPDX-License-Identifier: Apache-2.0

OS_NAME=ubuntu
OS_VERSION=jammy

PACKAGES="apt apt-utils bash bison build-essential bzip2 ca-certificates chrony coreutils \
curl dpkg dkms dialog ethtool flex gcc iptables kmod libc-dev libc6-dev make pciutils tar \
libelf-dev libltdl-dev debhelper swig python3 pkg-config dh-python chrpath quilt make graphviz \
lsof autoconf automake dpatch python3-distutils build-essential autotools-dev dh-autoreconf"

# PACKAGES="apt-utils ca-certificates autoconf automake autotools-dev bash bison build-essential bzip2 chrony chrpath debhelper dh-autoreconf dh-python \
# dpatch ethtool flex gcc gfortran graphviz iptables kmod libelf-dev libfuse2 libgfortran5 libltdl-dev libnl-3-200 libnl-3-dev \
# libnl-route-3-200 libnl-route-3-dev libnuma1 libusb-1.0-0 lsof m4 make pciutils perl perl-base perl-modules pkg-config \
# python3 python3-distutils quilt swig tcl tk udev dh-python quilt libelf-dev make dpatch pciutils gcc graphviz chrpath kmod \
# lsof ethtool autoconf perl debhelper python3 pkg-config dh-autoreconf swig coreutils bzip2 libltdl-dev build-essential m4 grep autotools-dev automake python3-distutils procps"

[ "$AGENT_INIT" = no ] && PACKAGES+=" init"

REPO_URL=http://ports.ubuntu.com

case "$ARCH" in
	aarch64) DEB_ARCH=arm64; REPO_URL=https://mirrors.tuna.tsinghua.edu.cn/ubuntu-ports ;;
	ppc64le) DEB_ARCH=ppc64el;;
	s390x) DEB_ARCH="$ARCH";;
	x86_64) DEB_ARCH=amd64; 
		#REPO_URL=http://archive.ubuntu.com/ubuntu 
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
