#!/bin/bash

set -x

# 安装 deb
dpkg -i /tmp/libxpu-container0_0.1.0_arm64.deb 
dpkg -i /tmp/libxpu-container-tools_0.1.0_arm64.deb 
dpkg -i /tmp/xpu-container-toolkit_0.1.0~rc.1-1_arm64.deb

# module
if [ ! -f /etc/modules-load.d/modules.conf ]; then
    touch /etc/modules-load.d/modules.conf
fi

echo "kunlun" >> /etc/modules-load.d/modules.conf

# prepare oci hook
hookpath="/usr/share/oci/hooks/prestart"
mkdir -p $hookpath

cat > $hookpath/xpu-container-toolkit.sh <<-'EOF'
#!/bin/bash -x

chmod 666 /dev/xpu*

/usr/bin/xpu-container-toolkit -debug $@
EOF

chmod 755 $hookpath/xpu-container-toolkit.sh
