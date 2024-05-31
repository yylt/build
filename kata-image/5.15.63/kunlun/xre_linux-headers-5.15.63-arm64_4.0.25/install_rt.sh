#!/bin/bash
#
# 本脚本为 kunlun runtime 的安装脚本，会将 runtime 安装到系统里去。
# 行为详细描述如下：
# 1. 将 kunlun-$(uname -r).ko 放到 /lib/modules/$(uname -r)/extra 目录下，并更新 modules 信息
#    a. 执行完这一步，并不会真正加载安装 kunlun 驱动，只是将 kunlun 驱动注册到了系统里
#    b. 安装后，可以在任何目录下通过 `modprobe kunlun` 安装驱动
#    c. 安装后，重启系统会自动加载 kunlun 驱动
# 2. 全部安装完成后，执行 `modprobe kunlun` 加载驱动
# 3. 将 XRE 所产出的库文件、头文件、工具安装到 /usr/local/xpu目录下
#    注意，安装后并不会修改 PATH 系统变量，如若想直接执行 runtime 中的工具，
#    仍需执行命令 `export PATH=/usr/local/xpu/bin:$PATH`
# 4. 本脚本预期存储目录为 $RT_OUT/，如果环境变量 $RT_OUT 未指定，则会认为相对地址
#    ./ 为 $RT_OUT，用户执行该脚本时可指定 $RT_OUT:
#    RT_OUT=<path_to_dir_runtime_output> ./install_rt.sh
#
# 本安装脚本假定 $RT_OUT 目录结构如下
#RT_OUT
#├── bin
#│   ├── xpu_modprobe
#│   └── xpu_smi
#├── Changelog
#├── doc
#│   ├── 01_xre-introdution.md
#│   ├── 02_xre-user-guide.md
#│   ├── 03_xre-programming-guide.md
#│   ├── 04_xre-api-reference.md
#│   └── 05_xre-faq.md
#├── include
#│   └── xpu
#│       ├── defs.h
#│       ├── runtime_ex.h
#│       ├── runtime.h
#│       ├── version.h
#│       └── xpuml.h
#├── install_rt.sh
#├── kunlun_module
#│   └── kunlun-*.ko
#├── lib
#│   ├── libpcida.a
#│   ├── libxpuml.a
#│   ├── libxpurt.a
#│   ├── libxpurt-debug.a
#│   ├── libxpurt-debug-ut.a
#│   └── libxpurt-ut.a
#├── script
#│   ├── bcm-plugin.py
#│   └── ota_upgrade_all.sh
#├── so
#│   ├── libpcida.so
#│   ├── libxpuml.so -> libxpuml.so.1
#│   ├── libxpuml.so.1 -> libxpuml.so.*
#│   ├── libxpuml.so.*
#│   ├── libxpurt-debug.so -> libxpurt-debug.so.1
#│   ├── libxpurt-debug.so.1 -> libxpurt-debug.so.*
#│   ├── libxpurt-debug.so.*
#│   ├── libxpurt.so -> libxpurt.so.1
#│   ├── libxpurt.so.1 -> libxpurt.so.*
#│   └── libxpurt.so.*
#├── test
#│   ├── scan_mem_linear
#│   ├── test_exception
#│   ├── test_exception_unsafe
#│   └── test_memcpy
#├── tools
#│   ├── br
#│   ├── bw
#│   ├── dump_mem
#│   ├── kl_fota
#│   ├── kunlun1
#│   │   ├── cdnn_diag_kl1
#│   │   ├── crash_recovery_mcu
#│   │   ├── freq
#│   │   ├── hbm_debug_tool
#│   │   ├── intc
#│   │   ├── mcu_rr
#│   │   ├── mcu_rw
#│   │   ├── mcu_status
#│   │   ├── ota
#│   │   ├── otp_rw
#│   │   ├── soft_rst
#│   │   └── xdinfo
#│   ├── kunlun2
#│   │   ├── mcu_util
#│   │   └── test_memcpy_peer
#│   ├── rr
#│   ├── run_sse_tc
#│   ├── rw
#│   ├── soft_reset
#│   ├── test_dma
#│   └── test_launch
#├── udev_rules
#│   └── 99-kunlun.rules
#├── uninstall_rt.sh
#└── version

set -ex

__DIR__=$(cd $(dirname ${BASH_SOURCE[0]}) && pwd -P)

if [[ -n "${RTOUT}" ]]; then
    rtout=${RTOUT}
else
    rtout=$(readlink -f ${__DIR__})
fi

version=$(cat ${rtout}/version | grep 'version:' | sed "s/version:\(.*\)/\1/g")

######################################
## Install Driver
######################################
kofile=${rtout}/kunlun_module/kunlun-$(uname -r).ko
if [[ ! -f ${kofile} ]]; then
    echo "Cannot find kunlun driver file ${kofile}"
    exit 1
fi

kernel_modules_dir=/lib/modules/$(uname -r)/
if [[ ! -d ${kernel_modules_dir} ]]; then
    echo "System's modules directory (${kernel_modules_dir}) does not exists"
    exit 1
fi

ko_install_dir=${kernel_modules_dir}/extra
mkdir -p ${ko_install_dir}

# in case of deprecated-xpu-driver pieces remained
if [[ -f ${ko_install_dir}/xpu_driver*.ko ]]; then
    rm -rf ${ko_install_dir}/xpu_driver*.ko
fi
if [[ -f /etc/udev/rules.d/99-xpu_driver.rules ]]; then
    rm /etc/udev/rules.d/99-xpu_driver.rules
fi

cp -rf ${kofile} ${ko_install_dir}/kunlun.ko
echo "Driver file installed"

cd ${kernel_modules_dir}; depmod
echo "System modules map regenerated"

if [[ -d ${rtout}/udev_rules ]]; then
    cp -rf ${rtout}/udev_rules/99-kunlun.rules /etc/udev/rules.d
    echo "udev rule installed."
else
    echo "udev_rules/ not found, skipped"
fi

######################################
## Install Library and Executables
######################################
install_prefix=${XRE_INSTALL_PREFIX:-/usr/local}

rt_install_dir=${install_prefix}/xpu-${version}
rt_install_link=${install_prefix}/xpu

if [[ -d ${rt_install_dir} ]]; then
    rm -rf ${rt_install_dir}
fi
mkdir -p ${rt_install_dir}

if [[ -h ${rt_install_link} ]]; then
    rm -rf ${rt_install_link}
fi
if [[ -d ${rt_install_link} ]]; then
    rm -rf ${rt_install_link}
fi
ln -sf ${rt_install_dir} ${rt_install_link}

cp -af ${rtout}/bin/ ${rt_install_dir}/

if [[ -f /usr/local/bin/xpu_smi ]]; then
    rm -rf /usr/local/bin/xpu_smi
fi
ln -sf ${rt_install_dir}/bin/xpu_smi /usr/local/bin/xpu_smi

cp -af ${rtout}/include/ ${rt_install_dir}/
cp -af ${rtout}/tools/ ${rt_install_dir}/
cp -af ${rtout}/version ${rt_install_dir}/

mkdir -p ${rt_install_dir}/lib64
cp -af ${rtout}/lib/* ${rt_install_dir}/lib64/
cp -af ${rtout}/so/* ${rt_install_dir}/lib64/

echo "Kunlun Runtime installed successfully."

echo "Now load KUNLUN driver..."
modprobe kunlun
echo "Done"

echo "Please execute 'export PATH=${rt_install_dir}/bin:$PATH' or add it to your .bashrc to be able
to run Kunlun binaries anywhere."
