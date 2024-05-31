#!/bin/bash

# 本脚本默认会将x86机器上所有识别到的昆仑卡的firmware升级到 1.0016.0021,
# 将飞腾(Phytium)机器上所有识别到的昆仑卡的firmware升级到 1.0015.0022
#
# 如果不需要升级中间位版本，请注释掉 BOOTLOADER=*** 这一行
# 如果不需要升级第三位版本，请注释掉 MAINFW=*** 这一行
#
# 使用方法：
# 1. 重启机器，并且保证重启后没有加载过 xpu_driver
#    `lsmod | grep xpu_driver` 没有输出
# 2. 设置XRE_ROOT系统变量，指向XRE产出的根目录
# 3. 使用ROOT权限执行本脚本
# 4. 全部更新完毕后，脚本会打印所有设备的新固件版本号，请人工确认一下
# 5. 没问题后再次重启使新固件生效

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
XRE_ROOT=${XRE_ROOT:-${SCRIPT_DIR}/..}

if [[ ! -d ${XRE_ROOT} ]]; then
    echo "Cannot find folder '${XRE_ROOT}', please set \$XRE_ROOT correctly"
    exit 1
fi

IS_PHYTIUM=0
BOOTLOADER_PATH=${XRE_ROOT}/firmware/flash_init_bootloader.online.x.16.x
MAINFW_PATH=${XRE_ROOT}/firmware/main.online.x.x.21
if [ $(cat /proc/cpuinfo | grep Phytium | wc -l) -gt 0 ];then
    # 飞腾(Phytium)机器
    IS_PHYTIUM=1
    BOOTLOADER_PATH=${XRE_ROOT}/firmware/flash_init_bootloader.online.x.15.x
    MAINFW_PATH=${XRE_ROOT}/firmware/main.online.x.x.22
fi

OTA=${XRE_ROOT}/tools/ota
BURNER=${XRE_ROOT}/firmware/flash_burner.online
# 如果不需要升级中间位版本，请注释掉下面这一行
BOOTLOADER=${BOOTLOADER_PATH}
# 如果不需要升级第三位版本，请注释掉下面这一行
MAINFW=${MAINFW_PATH}

if [[ ! -x ${OTA} ]]; then
    echo "ota '${OTA}' not found or not execute permission, please set \$XRE_ROOT correctly"
    exit 1
fi

if [[ ! -f ${BURNER} ]]; then
    echo "burner '${BURNER}' not found, please set \$XRE_ROOT correctly"
    exit 1
fi

args=" -f ${BURNER} "

if [[ -n ${BOOTLOADER} ]]; then
    if [[ ! -f ${BOOTLOADER} ]]; then
        echo "bootloader '${BOOTLOADER}' not found, please set \$XRE_ROOT correctly"
        exit 1
    fi
    args="${args} -b ${BOOTLOADER} "
fi

if [[ -n ${MAINFW} ]]; then
    if [[ ! -f ${MAINFW} ]]; then
        echo "main firmware '${MAINFW}' not found, please set \$XRE_ROOT correctly"
        exit 1
    fi
    args="${args} -m ${MAINFW} "
fi

for addr in $(lspci -D -d 1d22: | awk '{print $1}' | tr '\n' ' '); do
    echo "##### Enable ${addr} ..."
    echo 1 > /sys/bus/pci/devices/${addr}/enable
    echo "Done"

    echo "##### cat /sys/bus/pci/devices/${addr}/enable"
    cat /sys/bus/pci/devices/${addr}/enable
done

for addr in $(lspci -D -d 1d22: | awk '{print $1}' | tr '\n' ' '); do
    echo "##### ${OTA} -s ${addr} ${args}"
    ${OTA} -s ${addr} ${args}
    if [[ $? -eq 0 ]]; then
        echo "Finish"
    else
        echo "Fail"
    fi
done

echo "##### Please check new firmware version"
SMI=${XRE_ROOT}/bin/xpu_smi
for addr in $(lspci -D -d 1d22: | awk '{print $1}' | awk '{match($0,"0000:");print substr($0,RSTART+5)}' | tr '\n' ' '); do
    echo "${addr}:"
    ${OTA} -s ${addr} -f ${BURNER} | grep "flash version" | awk '{print $3}' | tr ',' ' '
done
