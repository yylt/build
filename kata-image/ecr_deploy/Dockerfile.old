FROM docker.io/library/ubuntu:20.04

RUN apt-get update && \
    apt-get install -y --no-install-recommends openssh-client && \
    rm -rf /var/lib/apt/lists/* && \
    mkdir -p /ecr && \
    mkdir -p /ecr/rpms

# rpms not required for 621+
ADD rpms/qemu /ecr/rpms
# prepare ecr-flash for uefi boot
ADD uefi/aarch64/ecr-flash.tar.gz /ecr

# COPY 6.1.0 qemu static wich cpuhotplug
COPY qemu-static/ecr-static-qemu.tar.gz /ecr


# files which created from build steps
COPY vmlinuz.container /ecr
COPY *.img /ecr
COPY containerd-shim-ecr-v2 /ecr
COPY ecr-runtime /ecr
COPY configuration.toml /ecr

COPY scripts/entrypoint.sh /entrypoint.sh

# Entrypoint ["/entrypoint.sh"]
