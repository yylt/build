#!/bin/bash
#
# 本脚本为 kunlun runtime 的卸载脚本
# 1. 用户通过 install_rt.sh 脚本安装的昆仑驱动可以通过该脚本进行卸载, 需要 root 权限，流程如下：
#    a. 卸载驱动，执行 rmmod kunlun
#    b. 执行该脚本，./uninstall_rt.sh
# 2. 脚本行为详细描述如下：
#    a. 删除驱动文件和对应的 rules，解除了系统重启后驱动自动安装的流程
#    b. 删除通过 install_rt.sh 脚本安装在系统 /usr/local/xpu 下的动态链接库和二进制工具文件

echo "Removing KUNLUN Driver..."

# in case of deprecated-xpu-driver pieces remained
rm -rf /lib/modules/$(uname -r)/extra/xpu_driver*.ko
rm -rf /etc/udev/rules.d/99-xpu_driver.rules

rm -rf /lib/modules/$(uname -r)/extra/kunlun*.ko
cd /lib/modules/$(uname -r)/; depmod
rm -rf /etc/udev/rules.d/99-kunlun.rules

echo "Removing KUNLUN RT..."

if [[ -h /usr/local/xpu ]]; then
    link=$(readlink -f /usr/local/xpu)
    rm -rf ${link}
    rm -rf /usr/local/xpu
    rm -rf /usr/local/bin/xpu_smi
elif [[ -d /usr/local/xpu ]]; then
    rm -rf /usr/local/xpu
    rm -rf /usr/local/bin/xpu_smi
fi
