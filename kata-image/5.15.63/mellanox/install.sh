#!/bin/bash

set -x


# 安装 kernel deb
dpkg -i /tmp/linux*deb 

# 安装 mlnx
/tmp/MLNX_OFED_LINUX-5.8*/mlnxofedinstall -k 5.15.63 --add-kernel-support --distro ubuntu22.04

# 更新内核模块依赖
depmod -a 5.15.63
