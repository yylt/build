# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository Overview

This is a container image build repository that uses GitHub Actions to build and publish various container images for Kubernetes, cloud-native, and infrastructure components. The repository contains 60+ Dockerfiles and corresponding GitHub Actions workflows for automated multi-architecture (amd64/arm64) image builds.

## Architecture

### Build System Structure

The repository follows a consistent pattern across all image builds:

1. **Dockerfiles**: Root directory contains `Dockerfile-<component>` files (e.g., `Dockerfile-k8snew`, `Dockerfile-flannel`, `Dockerfile-ciliumagent`)
2. **GitHub Actions Workflows**: `.github/workflows/<component>.yaml` files trigger builds on branch pushes or manual dispatch
3. **Build Scripts**: Component-specific build scripts in subdirectories (`ingress/`, `kata-image/`, `scripts/`)
4. **Source Code**: External projects are checked out during CI builds into a `<PROJECT>` directory

### Workflow Pattern

Most workflows follow this structure:
- Trigger on branch push (e.g., `branches: [ k8snew ]`) or `workflow_dispatch` with configurable inputs
- Check out external repository (e.g., `easystack/kubernetes`, `flannel-io/flannel`)
- Use Docker Buildx for multi-platform builds
- Push to private registry defined in secrets (`MY_HUB_NAME`, `MY_HUB_USER`, `MY_HUB_PASSWORD`)
- Tag images with version/branch name or timestamp

### Registry Organization

Images are pushed to different registry paths based on architecture:
- **amd64**: `${REGISTRY_NAME}/captain/<image>` and `${REGISTRY_NAME}/production/<image>`
- **arm64**: `${REGISTRY_NAME}/arm64v8/<image>`

## Common Build Commands

### Building Images Locally

For standard components using Docker Buildx:
```bash
# Build for single architecture
docker buildx build \
  --platform linux/amd64 \
  --tag <registry>/<image>:<tag> \
  --file ./Dockerfile-<component> ./<source-dir>

# Build for multiple architectures
docker buildx build \
  --platform linux/arm64,linux/amd64 \
  --tag <registry>/<image>:<tag> \
  --file ./Dockerfile-<component> ./<source-dir>
```

### Kata Container Images

Kata container builds use a custom Makefile system:

```bash
# Build kata kernel and image (from kata-containers checkout)
cd kata-containers
make -f ../kata-image/kata-image.mk generate-config
make -f ../kata-image/kata-image.mk containerd-shim-v2 ecr-runtime
make -f ../kata-image/kata-image.mk build-kernel
make -f ../kata-image/kata-image.mk agent
make -f ../kata-image/kata-image.mk build-image

# Build target-specific images (mellanox, nvidia, ecr, kunlun)
TARGET=mellanox make -f ../kata-image/kata-image.mk target
TARGET=nvidia make -f ../kata-image/kata-image.mk target

# Build and push Docker image
make -f ../kata-image/kata-image.mk docker-push
```

The kata build script can also be invoked directly:
```bash
./kata-image/build.sh -k "5.15.63" -s "<kata-src-path>" -t "<target>"
# where target is one of: kunlun, mellanox, nvidia, ecr
```

### Ingress NGINX Builds

Custom ingress builds with GmSSL support:
```bash
cd ingress
./build-1.9.sh  # Builds NGINX 1.21.6 with GmSSL and various modules
```

## Key Components

### Kubernetes (k8snew)

- **Source**: `easystack/kubernetes`
- **Dockerfile**: `Dockerfile-k8snew`
- **Workflow**: `.github/workflows/k8snew.yaml`
- **Default Branch**: `v1.32.2-es`
- **Output**: `hyperkube` image for both amd64 and arm64

### Flannel

- **Source**: `flannel-io/flannel`
- **Dockerfile**: `Dockerfile-flannel`
- **Workflow**: `.github/workflows/flannel.yaml`
- **Default Version**: `v0.26.4`

### Kata Containers (ECR Deploy)

- **Source**: `easystack/kata-containers`
- **Branch**: `stable-2.5`
- **Makefile**: `kata-image/kata-image.mk`
- **Kernel Version**: 5.15.63
- **Targets**: mellanox (MLNX drivers), nvidia (GPU drivers), ecr (base), kunlun (ARM)
- **Components Built**:
  - `containerd-shim-ecr-v2`: Containerd shim
  - `ecr-runtime`: Kata runtime
  - `vmlinuz.container`: Kernel image
  - `ecr-containers.img`: Root filesystem image
  - `configuration.toml`: Runtime configuration

### Cilium Agent

- **Source**: `yylt/cilium`
- **Branch**: `eni`
- **Dockerfile**: `Dockerfile-ciliumagent`

## Environment Variables and Secrets

GitHub Actions workflows use these secrets:
- `MY_HUB_NAME`: Container registry hostname
- `MY_HUB_USER`: Registry username
- `MY_HUB_PASSWORD`: Registry password
- `KPULL`: GitHub token for pulling private repositories
- `MLNX_X64_URL`: Mellanox driver download URL (for kata builds)
- `NVIDIA_X64_URL`: NVIDIA driver download URL (for kata builds)
- `WEWORK_ECRGROUP_BUILD`: WeChat Work webhook for build notifications

## Build Flags

Common Docker Buildx flags used across workflows:
- `--no-cache`: Disable build cache
- `--provenance false --sbom false`: Disable attestations (set via `BUILDX_NO_DEFAULT_ATTESTATIONS=1`)
- `--platform linux/amd64` or `--platform linux/arm64`: Target architecture
- `--output "type=image,push=true"`: Push image after build

## Development Workflow

1. **Adding a New Component**:
   - Create `Dockerfile-<component>` in root directory
   - Create `.github/workflows/<component>.yaml` following existing patterns
   - Set appropriate branch trigger or workflow_dispatch inputs
   - Configure registry secrets if needed

2. **Modifying Existing Builds**:
   - Update Dockerfile in root directory
   - Modify workflow file if build arguments or steps change
   - Test locally with Docker Buildx before pushing

3. **Kata Container Development**:
   - Modify kernel configs in `kata-image/5.15.63/configs/`
   - Update rootfs customizations in `kata-image/5.15.63/<target>/`
   - Test with `kata-image/build.sh` script
   - Update `kata-image/kata-image.mk` for build process changes

## Special Directories

- `kata-image/`: Kata container kernel and image build system
- `ingress/`: Custom NGINX ingress builds with GmSSL support
- `scripts/`: Utility scripts for image management (pullpush.sh, pushctr.sh)
- `source/`: Local source code projects (ecsnode, etcdcluster, hello-rs)
- `target/`: Build artifacts (rust-analyzer)
- `xray/`: Xray proxy related files

## Notes

- Most builds use self-hosted runners for kata builds (`runs-on: [self-hosted, linux, x64]`)
- Standard builds use GitHub-hosted runners (`runs-on: ubuntu-latest`)
- Kata builds require Rust 1.77.2 and Go 1.22
- Architecture detection uses `uname -m` (returns `x86_64` or `aarch64`)
- Kata kernel version is pinned to 5.15.63 across all targets
