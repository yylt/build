#!/usr/bin/env bash
set -e

runtimeName=${RUNTIME_NAME:-"containerd-shim-ecr-v2"}
systemBinDir=${BIN_DIR:-"/host/usr/bin/"}
ecrRuntimeName=${ECR_RUNTIME:-"ecr-runtime"}
configname="configuration.toml"
configDir=${CONFIG_DIR:-"/host/etc/ecr/"}
kernelName=${KERNEL_NAME:-"vmlinuz.container"}
kernelDir=${KERNEL_DIR:-"/host/usr/share/ecr/"}
imageName=${IMAGE_NAME:-"ecr-containers.img"}
NvidiaImageName=${IMAGE_NAME:-"ecr-containers-nvidiaGpu.img"}
imageDir=${IMAGE_DIR:-"/host/usr/share/ecr/"}
flashTarName=${FLASH_TAR_NAME:-"ecr-flash.tar.gz"}
flashDir=${FLASH_DIR:-"/host/usr/share/ecr/"}
sourceDir=${SOURCE_DIR:-"/ecr"}
localDir=$(pwd)

logPrint() {
    date=`date +'%Y-%m-%d %H:%M:%S'`
    echo "$date: $1"
}

prepareShim() {
    if [[ $(hashID ${sourceDir}/${runtimeName}) != $(hashID ${systemBinDir}${runtimeName}) ]]
    then
        # force copy for solving problem "Text file busy"
        cp -f ${sourceDir}/${runtimeName} ${systemBinDir}
        if [ $? -eq 0 ];then
            logPrint "Prepare runtime shim: success"
        else
            logPrint "Prepare runtime shim: failed"
            exit 1
        fi
    else
        logPrint "Same runtime shim: No need to upgrade"
    fi
}

prepareEcrRuntime() {
    if [[ $(hashID ${sourceDir}/${ecrRuntimeName}) != $(hashID ${systemBinDir}${ecrRuntimeName}) ]]
    then
        # force copy for solving problem "Text file busy"
        cp -f ${sourceDir}/${ecrRuntimeName} ${systemBinDir}
        if [ $? -eq 0 ];then
            logPrint "Prepare runtime: success"
        else
            logPrint "Prepare runtime: failed"
            exit 1
        fi
    else
        logPrint "Same runtime: No need to upgrade"
    fi
}

prepareConfig() {
    mkdir -p ${configDir}
    if [[ $(hashID ${sourceDir}/${configname}) != $(hashID ${configDir}${configname}) ]]
    then
        cp -f ${sourceDir}/${configname} ${configDir}
        if [ $? -eq 0 ];then
            logPrint "Prepareruntime configuration: success"
        else
            logPrint "Prepare runtime configuration: failed"
            exit 1
        fi
    else
        logPrint "Same runtime configuration: No need to upgrade"
    fi
}

prepareKernel() {
    mkdir -p ${kernelDir}
    if [[ $(hashID ${sourceDir}/${kernelName}) != $(hashID ${kernelDir}${kernelName}) ]]
    then
        cp -f ${sourceDir}/${kernelName} ${kernelDir}
        if [ $? -eq 0 ];then
            logPrint "Prepare kernel: success"
        else
            logPrint "Prepare kernel: failed"
            exit 1
        fi
    else
        logPrint "Same kernel: No need to upgrade"
    fi
}

prepareImage() {
    mkdir -p ${imageDir}

    find "${sourceDir}" -maxdepth 1 -type f -name "*.img" | while read -r source_image_path; do
        # 提取文件名，例如 "my_image.img"
        imageName=$(basename "${source_image_path}")

        logPrint "Processing image: ${imageName}"

        # 计算源文件哈希
        source_hash=$(hashID "${source_image_path}")
        # 计算目标文件哈希
        target_hash=$(hashID "${imageDir}${imageName}")

        if [[ "${source_hash}" != "${target_hash}" ]]; then
            logPrint "Image hash mismatch for ${imageName}. Updating..."

            # 尝试删除旧文件并拷贝新文件
            # 使用 -f 确保即使目标文件不存在也不会报错
            rm -f "${imageDir}${imageName}" && \
            cp "${source_image_path}" "${imageDir}${imageName}"

            if [ $? -eq 0 ]; then
                logPrint "Prepare image: success for ${imageName}"
            else
                logPrint "Prepare image: failed for ${imageName}"
                exit 1 # 拷贝失败则退出脚本
            fi
        else
            logPrint "Same image: No need to upgrade for ${imageName}"
        fi
    done

    logPrint "All .img files processed."
}

prepareStopEcrService() {
  node=$(hostname)

  ssh -o "StrictHostKeyChecking=no" $node \
  "cat <<EOF> /etc/systemd/system/stop_ecr.service
[Unit]
Description=stop ecr shim process
Before=containerd.service
After=rbdmap.service

[Service]
Type=oneshot
ExecStop=/usr/bin/pkill -15 -ef containerd-shim-ecr-v2
RemainAfterExit=yes
ExecReload=/bin/true
ExecStart=/bin/true

[Install]
WantedBy=multi-user.target
EOF"
    ssh -o "StrictHostKeyChecking=no" $node \
    "systemctl daemon-reload && systemctl enable stop_ecr &&systemctl start stop_ecr"
}

hashID() {
    if [ -f $1 ];then
        md5sum $1 | awk '{print $1}'
    else
        echo ""
    fi
}

loop() {
    sleep infinity
}

# 准备升级 qemu 组件
prepareQemu() {
    # 拷贝文件
    nodeName=$(hostname)
    tmpDir=$(ssh $(hostname) "mktemp -d")
    arch=$(ssh ${nodeName} "arch")
    scp -r /ecr/rpms/${arch}/* root@${nodeName}:${tmpDir}/

    # 卸载 qemu
    logPrint "uninstall old qemu-kvm* rpm"
    for i in $(ssh ${nodeName} "yum list --installed  | grep -i qemu-kvm" | awk '{print $1}');do ssh ${nodeName} "yum remove -y $i";done

    # 升级 qemu
    logPrint "upgrade qemu-kvm* rpm"
    ssh ${nodeName} "tar -C ${tmpDir} -xf ${tmpDir}/qemu-kvm*.tar.gz"
    ssh ${nodeName} "yum localinstall -y ${tmpDir}/*.rpm"
    # WARNING: 危险操作，需要判空
    # ssh ${nodeName} "rm -rf ${tmpDir}"
    # 使用社区的6.1.0 qemu来支持cpu-hotplug功能
    if [[ $(arch) == "aarch64" ]]
    then
        logPrint "install 6.1.0 qemu with cpu-hotplug patch"
        scp -r /ecr/ecr-static-qemu.tar.gz root@${nodeName}:${tmpDir}/
        ssh ${nodeName} "tar zxvf ${tmpDir}/ecr-static-qemu.tar.gz -C /opt/"
    fi
}

# 准备 ecr-flash 文件
prepareFlash() {
    if [[ $(arch) == "aarch64" ]]
    then
        if [[ $(hashID ${sourceDir}/ecr-flash0.img) != $(hashID ${flashDir}ecr-flash0.img) ]]
        then
            cp -f ${sourceDir}/ecr-flash0.img ${flashDir}
            cp -f ${sourceDir}/ecr-flash1.img ${flashDir}
            if [ $? -eq 0 ];then
                logPrint "Prepare ecr flash: success"
            else
                logPrint "Prepare ecr flash: failed"
                exit 1
            fi
        else
            logPrint "Same ecr flash: No need to upgrade"
        fi
    else
        logPrint "non-aarch64 platform, no execution"
    fi

}

prepareContainerd() {
    for node in $(hostname);do
        if [[ $(ssh $node "cat /etc/containerd/config.toml" | grep -c "privileged_without_host_devices = true") -eq 0 ]];then
            if [[ $(ssh $node "systemctl status containerd"  | grep -c "active (running)") -eq 1 ]];then

                logPrint "$node: add containerd.service configuration 'privileged_without_host_devices = true' for ecr runtime"
                ssh $node "sed -i '/io.containerd.ecr.v2/a\         privileged_without_host_devices = true' /etc/containerd/config.toml"

                # 重启节点 containerd.service，使配置生效
                logPrint "$node: prepareContainerd(): restarting containerd.service"
                ssh $node "systemctl restart containerd"
                logPrint "$node: prepareContainerd(): restart containerd.service done."
            else
                logPrint "$node: containerd.service isn't running: modifying containerd failed."
                logPrint "prepareContainerd(): sleep 5s and script exit"
                sleep 5 # 等待 5s，脚本退出，重试
                exit 1
            fi
        else
            logPrint "$node: prepareContainerd(): containerd configuration 'privileged_without_host_devices' for ecr runtime is already modified."
        fi
    done
}

main() {
    prepareShim
    prepareConfig
    prepareKernel
    prepareImage
    prepareEcrRuntime
    prepareStopEcrService
    prepareQemu
    prepareFlash
    prepareContainerd

    echo "Jobs Done! Will run infinitely"
    loop
}

# for script debug
mkdir -p ${systemBinDir}
mkdir -p ${configDir}
mkdir -p ${kernelDir}
mkdir -p ${imageDir}

main