#!/usr/bin/env bash
#set -e


HOSTROOT=${HOSTDIR:-"/proc/1/root"}
HOSTDIRS=("/opt/kata/bin" "/etc/ecr" "/usr/share/ecr")
DIR=$(dirname "$(readlink -f "$0")")

# key: file in image, value: file in host
declare -A copyfiles

# define copy files

# container shim files
copyfiles["/containerd-shim-ecr-v2"]="/usr/bin/containerd-shim-ecr-v2"
copyfiles["/ecr-runtime"]="/usr/bin/ecr-runtime"

# configration file
copyfiles["/configuration.toml"]="/etc/ecr/configuration.toml"

# kernel
copyfiles["/vmlinuz.container"]="/usr/share/ecr/vmlinuz.container"

# qemu and virtiofsd, qemu have to extract.
copyfiles["/virtiofsd"]="/opt/kata/bin/virtiofsd"

# uefi boot on arm64 platform
copyfiles["/ecr/ecr-flash0.img"]="/usr/share/ecr/ecr-flash0.img"
copyfiles["/ecr/ecr-flash1.img"]="/usr/share/ecr/ecr-flash1.img"

# stop ecr service, for ecr-shim issue ...
copyfiles["/stop_ecr.service"]="/etc/systemd/system/stop_ecr.service"

# NOTE, different qemu image on different arch, so have to search-compare-copy


log() {
    local message="$1"
    echo "$(date +'%Y-%m-%d %H:%M:%S') [INFO] $message"
}

logfatal() {
    local message="$1"
    echo "$(date +'%Y-%m-%d %H:%M:%S') [FATAL] $message" >&2 # 输出到标准错误
    exit 1 
}

hashFile() {
    local file="$1"
    sha256sum "$file" | awk '{print $1}'
    if [ $? -ne 0 ]; then
        logfatal "Error: Failed to calculate SHA256 hash for file '$file'."
    fi
}

compareCopy() {
    file1="$1"
    file2="$2"

    # 检查文件1是否存在且是普通文件
    if [ ! -f "$file1" ]; then
        logfatal "Error: File '$file1' not found or is not a regular file."
    fi

    sha256_file1=$(hashFile "$file1")
    log "SHA256 of '$file1': $sha256_file1"

    if [ -f "$file2" ]; then

        sha256_file2=$(hashFile "$file2")
        log "SHA256 of '$file2': $sha256_file2"

        if [ "$sha256_file1" != "$sha256_file2" ]; then
            log "Hashes differ. Deleting '$file2' and copying '$file1' to '$file2'..."
            rm -f "$file2"
            if [ $? -ne 0 ]; then
                logfatal "Error: Failed to delete '$file2'."
            fi
            cp "$file1" "$file2"
            if [ $? -ne 0 ]; then
                logfatal "Error: Failed to copy '$file1' to '$file2'."
            fi
        else
            log "Same '$file1' with '$file2': No need to copy."
        fi
    else
        log "File '$file2' does not exist. Copying '$file1' to '$file2'..."
        cp "$file1" "$file2"
        if [ $? -ne 0 ]; then
            logfatal "Error: Failed to copy '$file1' to '$file2'."
        fi
    fi
}

copyFiles() {
    for key in "${!copyfiles[@]}"; do
        compareCopy "$key" "$HOSTROOT${copyfiles[$key]}"
    done    
}

# rootfs image should be in the same directory as the script
prepareQemuImages() {
    imageHostDir="$HOSTROOT/usr/share/ecr"

    find "${DIR}" -maxdepth 1 -type f -name "*.img" | while read -r source_image_path; do
        # 提取文件名，例如 "my_image.img"
        imageName=$(basename "${source_image_path}")
        compareCopy "${source_image_path}" "${imageHostDir}/${imageName}"
    done
}

# qemu-kvm-{arch}-experimental
prepareQemuKvm() {
    src="/kata-static-qemu-experimental.tar.gz"
    dst="${HOSTROOT}/opt/kata/kata-static-qemu-experimental.tar.gz"

    if [ -f "$dst" ]; then
        if [[ $(hashFile ${src}) == $(hashFile ${dst}) ]]; then
            log "prepareQemuKvm: QemuKvm hash is equal, skipping."
            return
        fi
    fi

    # extract file success then copy.
    log "prepareQemuKvm: Extracting ${src} to /"
    tar -xf "${src}" -C "${HOSTROOT}/"

    compareCopy "${src}" "${dst}"
}

prepareService() {
    nsenter --mount=/proc/1/ns/mnt -- bash -c "systemctl daemon-reload && systemctl enable stop_ecr && systemctl start stop_ecr"
}

prepareContainerd() {
    if [[ $(cat "${HOSTROOT}/etc/containerd/config.toml" | grep -c "privileged_without_host_devices = true") -eq 0 ]];then
        if [[ $(nsenter --mount=/proc/1/ns/mnt "systemctl status containerd"  | grep -c "active (running)") -eq 1 ]];then

            log " add containerd.service configuration 'privileged_without_host_devices = true' for ecr runtime"
            sed -i '/io.containerd.ecr.v2/a\         privileged_without_host_devices = true' "${HOSTROOT}/etc/containerd/config.toml"

            # 重启节点 containerd.service，使配置生效
            log " prepareContainerd(): restarting containerd.service"
            nsenter --mount=/proc/1/ns/mnt "systemctl restart containerd"
            log "prepareContainerd(): restart containerd.service done."
        else
            logfatal "containerd.service isn't running: modifying containerd failed."
        fi
    else
        log "prepareContainerd(): containerd configuration 'privileged_without_host_devices' for ecr runtime is already modified."
    fi
}


makeHostDir() {
    for mp in "${HOSTDIRS[@]}"; do
        log "makeHostDir(): mkdir -p ${HOSTROOT}${mp}"
        mkdir -p "${HOSTROOT}${mp}"
    done
}

makeHostDir
copyFiles
prepareQemuKvm
prepareQemuImages
prepareContainerd
prepareService

log "Jobs Done! Will run infinitely"
sleep infinity