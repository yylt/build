#!/bin/bash

set -e

# 准备必要的文件
# mkdir /var/lib/dpkg
# touch /var/lib/dpkg/status
# mkdir /var/lib/dpkg/updates/
# mkdir -p /var/lib/dpkg/info/
# touch /var/lib/dpkg/info/format-new
# mkdir -p /var/cache/apt/archives/partial
# mkdir -p /var/log/apt/
# mkdir /var/lib/dpkg/alternatives
# touch /var/lib/dpkg/available


# 安装 kernel deb
dpkg -i /tmp/linux*deb 

# 安装 mlnx
/tmp/MLNX_OFED_LINUX-5.8*/mlnxofedinstall -k 5.15.63 --add-kernel-support --distro ubuntu22.04

# 更新内核模块依赖
depmod -a 5.15.63
