ARG BASE_IMAGE=""

FROM ${BASE_IMAGE} as static

FROM docker.io/library/alpine:edge

RUN  apk -U --no-cache add bash && mkdir /ecr

# prepare ecr-flash for uefi boot on arm64 platform
ADD uefi/aarch64/ecr-flash.tar.gz /ecr 

# COPY qemu, kernel and virtiofsd 
COPY --from=static /virtiofsd /virtiofsd
COPY --from=static /kata-static-qemu-experimental.tar.gz /kata-static-qemu-experimental.tar.gz
COPY vmlinuz.container /

# reference: yylt/build/kata-image/build.sh
# arm64 have images: ecr-containers.img, kunlun-containers.img
# x86_64 have images: ecr-containers.img, ecr-containers-nvidiaGpu.img
COPY *.img /  

COPY containerd-shim-ecr-v2 /
COPY ecr-runtime /
COPY configuration.toml /

COPY scripts/stop_ecr.service /stop_ecr.service
COPY scripts/entrypoint.sh /entrypoint.sh

Entrypoint ["/entrypoint.sh"]
