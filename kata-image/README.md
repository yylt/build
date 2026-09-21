# Kata 容器构建说明

本目录包含 Kata Containers（ECR 部署）的容器镜像构建体系，用于生成 Kata 运行时、内核与根文件系统镜像。所有 Kata 相关的构建说明集中在本文件中，主仓库的 `AGENTS.md` 仅保留总览，不再重复展开。

## 概况

- **源码**：`easystack/kata-containers`
- **分支**：`stable-2.5`
- **内核版本**：5.15.63（在所有构建目标上统一固定）
- **Makefile**：`kata-image/kata-image.mk`
- **构建脚本**：`kata-image/build.sh`

## 构建目标（Targets）

Kata 按目标平台分别构建，`./kata-image/build.sh -t` 的可选值为：

- `ecr`：基础目标
- `mellanox`：集成 Mellanox（MLNX）网卡驱动
- `nvidia`：集成 NVIDIA GPU 驱动
- `kunlun`：ARM 平台（昆仑）目标

目标通过环境变量确认依赖下载地址：
- `MLNX_X64_URL`：Mellanox 驱动下载地址
- `NVIDIA_X64_URL`：NVIDIA 驱动下载地址

## 构建产物

一次完整构建会产出以下组件：

- `containerd-shim-ecr-v2`：Containerd shim
- `ecr-runtime`：Kata 运行时
- `vmlinuz.container`：内核镜像
- `ecr-containers.img`：根文件系统镜像
- `configuration.toml`：运行时配置

## 环境要求

- Kata 构建需使用**自托管运行器**（`runs-on: [self-hosted, linux, x64]`）。
- 需要 **Rust 1.77.2** 与 **Go 1.22** 编译环境。
- 架构探测使用 `uname -m`（返回 `x86_64` 或 `aarch64`），据此切换镜像仓库路径与 QEMU 二进制。

## 构建命令

### 方式一：直接调用 build.sh

```bash
./kata-image/build.sh -k "5.15.63" -s "<kata-src-path>" -t "<target>"
# target 取值之一：kunlun、mellanox、nvidia、ecr
```

脚本逻辑：
1. 解析参数（`-t` 目标、`-k` 内核版本、`-s` kata 源码路径）。
2. 校验源码、patch、config 目录存在，且 target 合法。
3. `kernel()`：拷贝内核 config 片段，调用 kata 的 `build-kernel.sh` 编译并生成 `vmlinuz.container`（x86_64 额外打 deb 包）。
4. `image()`：准备 rootfs（拷贝 `5.15.63/<target>` 定制内容），下载 mellanox/nvidia 驱动，调用 osbuilder 生成 `ecr-containers.img`。

### 方式二：使用 kata-image.mk（在 kata-containers 检出处执行）

```bash
cd kata-containers

# 生成按架构区分的运行时配置文件
make -f ../kata-image/kata-image.mk generate-config

# 构建运行时与 shim
make -f ../kata-image/kata-image.mk containerd-shim-v2 ecr-runtime

# 编译内核
make -f ../kata-image/kata-image.mk build-kernel

# 构建并打包 agent 与根文件系统镜像
make -f ../kata-image/kata-image.mk agent
make -f ../kata-image/kata-image.mk build-image

# 构建指定目标的镜像（mellanox、nvidia、ecr、kunlun）
TARGET=mellanox make -f ../kata-image/kata-image.mk target
TARGET=nvidia make -f ../kata-image/kata-image.mk target

# 构建并推送 Docker 镜像
make -f ../kata-image/kata-image.mk docker-push
```

`kata-image.mk` 关键变量：
- `REGISTRY_NAME`：镜像仓库（默认 `ghcr.io`）
- `IMAGE_NAME`：按架构自动选择 `${REGISTRY_NAME}/captain/ecr-deploy`（x86_64）或 `${REGISTRY_NAME}/arm64v8/ecr-deploy`（aarch64）
- `TARGET`：构建目标（kunlun/nvidia/mellanox/ecr）
- `SOURCE_DIR` / `AGENT_DIR`：kata 运行时与 agent 源码目录

`make package` 会安装内核与 agent 编译所需的交叉/本地工具链，并把 `cargo-config.toml` 写入 `~/.cargo/`。

## 目录结构

- `kata-image/build.sh`：一键构建脚本（kernel + image）
- `kata-image/kata-image.mk`：Makefile 构建体系（shim/runtime/kernel/agent/image/docker）
- `kata-image/5.15.63/`：内核版本目录
  - `configs/`：内核配置片段
  - `<target>/`：各目标（ecr/mellanox/nvidia/kunlun）的 rootfs 定制内容
- `kata-image/ecr_deploy/`：最终镜像打包目录（含 `Dockerfile`）
- `kata-image/cargo-config.toml`、`sources.list`：Rust 与 apt 源配置
