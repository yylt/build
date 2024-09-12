#!/bin/bash

set -x


# 安装 kernel deb
dpkg -i /tmp/linux*deb 

# 安装 nvidia 
chmod +x /tmp/NVIDIA*.run
/tmp/NVIDIA*.run -x
/NVIDIA*/nvidia-installer -k 5.15.63 -q -s --no-x-check

# 安装 mlnx
#/tmp/MLNX_OFED_LINUX*/mlnxofedinstall -k 5.15.63 --add-kernel-support --distro ubuntu22.04

# clean
rm -rf /NVIDIA* 
# 安装 toolkit
cat > /etc/apt/sources.list.d/nvidia-container-toolkit.list <<-'EOF'
deb [trusted=yes] https://nvidia.github.io/libnvidia-container/stable/deb/$(ARCH) /
EOF
apt update && apt --no-install-recommends -y install nvidia-container-toolkit && apt clean all

# prepare oci hook
hookpath="/usr/share/oci/hooks/prestart"
mkdir -p $hookpath

cat > $hookpath/nvidia-container-toolkit.sh <<-'EOF'
#!/bin/bash -x

/usr/bin/nvidia-container-toolkit -debug $@

EOF

chmod 755 $hookpath/nvidia-container-toolkit.sh

# 清理
rm -rf /var/lib/dpkg

# 更新内核模块依赖
depmod -a 5.15.63
