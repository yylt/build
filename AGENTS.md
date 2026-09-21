# 仓库概览

本仓库是一个**容器镜像构建仓库**，使用 GitHub Actions 自动构建并发布各类面向 Kubernetes、云原生及基础设施组件的容器镜像。仓库内含 60+ 个 Dockerfile 以及对应的 GitHub Actions 工作流，用于实现跨架构（amd64/arm64）的自动化镜像构建。

# 整体架构

仓库内所有的镜像构建都遵循一套一致的约定：

1. **Dockerfile**：根目录下存放 `Dockerfile-<组件>` 文件，每个文件负责一个镜像的构建。
2. **GitHub Actions 工作流**：`.github/workflows/<组件>.yaml` 文件在分支推送或手动触发时启动对应镜像的构建。
3. **构建脚本**：组件相关的构建脚本放在子目录中（`ingress/`、`kata-image/`、`scripts/`）。
4. **源码**：外部项目在 CI 构建期间被检出到 `<PROJECT>` 目录。

# GitHub Workflow 的作用

GitHub Actions 工作流（`.github/workflows/*.yaml`）是整个仓库的**自动化构建与发布引擎**，其职责包括：

- **触发构建**：监听特定分支推送（如 `branches: [ k8snew ]`）或响应 `workflow_dispatch` 手动触发，并支持通过输入参数（版本号、目标架构等）灵活控制构建。
- **拉取源码**：检出外部仓库（如 `kubernetes/kubernetes`、`flannel-io/flannel`），为镜像构建提供原材料。
- **调度构建环境**：根据构建类型选择运行器——标准镜像使用 GitHub 托管运行器（`runs-on: ubuntu-latest`），Kata 等重型构建使用自托管运行器（`runs-on: [self-hosted, linux, x64]`）。
- **驱动多架构构建**：调用 Docker Buildx 完成 amd64/arm64 跨平台构建。
- **推送与打标签**：使用仓库密钥（`MY_HUB_NAME`、`MY_HUB_USER`、`MY_HUB_PASSWORD`）将镜像推送到私有镜像仓库，并按版本/分支名或时间戳打标签。

# Dockerfile 的作用

Dockerfile（根目录下的 `Dockerfile-<组件>`）是**单个组件镜像的构建蓝图**，其职责包括：

- **定义基础镜像与构建阶段**：通过多阶段构建（multi-stage build）分离编译环境与最终运行环境，减小镜像体积。
- **声明构建参数（ARG）/ 环境变量（ENV）**：接收工作流传入的版本号、源码路径、架构等参数。
- **编排构建步骤**：安装依赖、编译组件（如 hyperkube、cilium-agent、kata 运行时）、拷贝产物，最终生成可运行的容器镜像。
- **确定产物形态**：决定镜像对外暴露的入口（ENTRYPOINT/CMD）以及最终交付的镜像内容。

> 工作流负责"何时、用什么参数、把镜像推到哪里"，Dockerfile 负责"镜像里到底装什么、怎么装"。两者通过 `--file` 与 `--build-arg` 衔接。

# 工作流通用模式

大多数工作流遵循如下结构：

- 触发条件：分支推送（如 `branches: [ k8snew ]`）或带可配置输入的 `workflow_dispatch`。
- 检出外部仓库（如 `flannel-io/flannel`）。
- 使用 Docker Buildx 进行多平台构建。
- 推送到密钥定义的私有仓库（`MY_HUB_NAME`、`MY_HUB_USER`、`MY_HUB_PASSWORD`）。
- 按版本/分支名或时间戳为镜像打标签。

# 镜像仓库组织

镜像按架构推送到不同的仓库路径：

- **amd64**：`${REGISTRY_NAME}/captain/<image>` 与 `${REGISTRY_NAME}/production/<image>`
- **arm64**：`${REGISTRY_NAME}/arm64v8/<image>`

# 常用构建命令

## 本地构建镜像

针对使用 Docker Buildx 的标准组件：

```bash
# 单架构构建
docker buildx build \
  --platform linux/amd64 \
  --tag <registry>/<image>:<tag> \
  --file ./Dockerfile-<component> ./<source-dir>

# 多架构构建
docker buildx build \
  --platform linux/arm64,linux/amd64 \
  --tag <registry>/<image>:<tag> \
  --file ./Dockerfile-<component> ./<source-dir>
```

# Dockerfile 用途一览

下表列出根目录中每个 `Dockerfile-<组件>` 的构建目的（按类别分组，均为一句话说明，不含详细构建过程）：

## Kubernetes 及控制面

- `Dockerfile-k8snew`：基于 `easystack/kubernetes`（v1.32.2-es）构建 Kubernetes `hyperkube` 镜像（amd64/arm64）。
- `Dockerfile-k8s`：将已编译的 `kube*` 二进制打包为 Kubernetes 运行时镜像，内含 ceph/xfs 等运维工具。
- `Dockerfile-multik8s`：从源码编译 kubelet/kube-proxy/apiserver/scheduler 等多控制面组件。
- `Dockerfile-kube`：打包 `kubemark` 与 e2e 测试工具（`e2e.test`、`kubemark`）。
- `Dockerfile-kcrowplugin`：构建 kcrow 多架构 Go 插件二进制镜像。
- `Dockerfile-capi`：构建 Cluster API / eks-kubean 相关镜像（CRD + 二进制）。
- `Dockerfile-autoscaler`：构建 cluster-autoscaler 集群自动扩缩容管理器。
- `Dockerfile-capsule`：构建 Capsule 多租户 Operator 二进制镜像。

## 网络 / CNI

- `Dockerfile-flannel`：构建 Flannel 网络插件 `flanneld` 镜像。
- `Dockerfile-ciliumagent`：构建 Cilium Agent 镜像（含 cilium-envoy、Hubble CLI）。
- `Dockerfile-cni`：打包 CNI 插件集合（sriov-cni、ib-sriov-cni、whereabouts、flannel cni-plugin）。
- `Dockerfile-esmultus`：构建定制版 Multus CNI 多网络插件镜像。
- `Dockerfile-kubeovn`：构建 Kube-OVN 网络插件镜像。
- `Dockerfile-sriov`：构建 SR-IOV 网络设备插件（sriovdp、sriov-network-operator）。
- `Dockerfile-istio`：构建 Istio 运行时基础镜像（pilot-agent 等）。
- `Dockerfile-istiod`：构建 Istio 控制面镜像（`pilot-discovery`）。
- `Dockerfile-istioproxy`：基于 envoy proxyv2 构建 Istio sidecar 代理镜像。
- `Dockerfile-linkerd`：构建 Linkerd 服务网格控制面/代理镜像。
- `Dockerfile-gw`：构建 gateway-api admission webhook 镜像。
- `Dockerfile-envoy`：打包 Envoy 代理构建产物镜像。
- `Dockerfile-dnsperf`：构建 DNS 性能测试工具 `dnsperf` 镜像。
- `Dockerfile-dnstap`：构建 `dnstap` DNS 抓包/转发工具镜像。
- `Dockerfile-smartdns`：构建 SmartDNS 智能 DNS 服务镜像。

## 服务网格 / 可观测 / 证书

- `Dockerfile-kiali`：构建 Kiali 服务网格可观测性控制台镜像。
- `Dockerfile-cert`：构建 cert-manager（webhook/cainjector/controller）镜像。
- `Dockerfile-cert-istio-csr`：构建 cert-manager istio-csr 证书代理镜像。

## 存储

- `Dockerfile-cephcsi`：构建 Ceph CSI 驱动镜像。
- `Dockerfile-etcd`：基于官方 etcd 镜像精简打包 etcd/etcdctl/etcdutl 镜像。
- `Dockerfile-etcdcluster`：构建 etcd 集群管理工具镜像（基于 etcd 镜像）。
- `Dockerfile-pykube`：构建集成 Python k8s/etcd 客户端的工具镜像。

## 安全 / 运行时

- `Dockerfile-falco`：构建 Falco 运行时安全探测器（`falco-builder`）。
- `Dockerfile-gvisor`：打包 gVisor（`runsc`）沙箱运行时镜像。
- `Dockerfile-firecracker`：构建 Firecracker 微虚拟机 rootfs 镜像。
- `Dockerfile-qemu`：构建 QEMU 虚拟化组件镜像。

## GPU / 加速

- `Dockerfile-dcgm`：基于 CUDA 基础镜像构建 NVIDIA DCGM / accelerator-manager GPU 监控镜像。

## AI / 推理

- `Dockerfile-vllm`：构建 vLLM 大模型推理镜像（含 CPU 构建/运行支持）。
- `Dockerfile-paddle`：构建 PaddlePaddle / RKNN 工具链镜像。

## 网关 / 门户

- `Dockerfile-defaultbackend`：构建 ingress-nginx 默认后端镜像。
- `Dockerfile-dashboard`：构建 Kubernetes Dashboard 前端镜像（Node 构建 + nginx 托管）。
- `Dockerfile-backstage`：构建 Backstage 开发者门户镜像。
- `Dockerfile-pandora`：构建 Pandora Node 应用镜像。
- `Dockerfile-spice`：构建 SPICE HTML5 远程桌面镜像（caddy + spice-html5 + websockify）。

## 工具 / 运维

- `Dockerfile-build`：通用 Go/构建基础工具镜像（alpine + go/make/rsync 等），常作为构建阶段基础。
- `Dockerfile-tool`：运维工具集镜像（helm/kubectl/etcd/containerd/yq/mc/gomplate 等）。
- `Dockerfile-helm`：构建 Helm 包管理器镜像。
- `Dockerfile-ansible`：构建 Ansible 自动化运维镜像。
- `Dockerfile-testool`：测试工具集镜像（curl/nmap/sysbench/iperf/fio/stress-ng 等）。
- `Dockerfile-fio`：构建 fio 磁盘 IO 性能测试工具镜像。
- `Dockerfile-bpftool`：构建 bpftool 内核调试工具镜像。
- `Dockerfile-runner`：构建 GitHub Actions 自托管 runner 镜像。
- `Dockerfile-n2n`：构建 n2n P2P VPN（edge/supernode）镜像。
- `Dockerfile-weixin`：构建微信桌面客户端（deepin-wine）镜像。
- `Dockerfile-opi`：打包 Orange Pi 5 Pro（rk3588）内核 artifact 镜像。

> 注：`ingress/`、`kata-image/` 子目录下另有各自的 Dockerfile（如 `ingress/Dockerfile-nginx`、`kata-image/5.15.63/*/Dockerfile.in`），分别服务于自定义 ingress 与 Kata 容器构建体系，此处不展开。

# 如何新增构建组件

新增一个镜像构建组件通常只需两步：**编写 Dockerfile** 与 **编写对应工作流**。

## 1. 编写 Dockerfile

在仓库根目录创建 `Dockerfile-<组件名>`，建议采用多阶段构建。最小骨架如下：

```dockerfile
# 阶段一：编译（按需选择 golang/node/rust 等基础镜像）
FROM golang:1.22 AS builder
ARG VERSION
WORKDIR /workspace
COPY . .
RUN go build -o /out/app ./cmd/app

# 阶段二：运行（尽量精简基础镜像）
FROM alpine:3.20
COPY --from=builder /out/app /usr/local/bin/app
ENTRYPOINT ["/usr/local/bin/app"]
```

## 2. 编写工作流

在 `.github/workflows/` 创建 `<组件名>.yaml`，沿用既有模式。最小骨架如下：

```yaml
name: <component>
on:
  push:
    branches: [ <branch> ]
  workflow_dispatch:
    inputs:
      VERSION:
        description: '镜像版本/分支'
        required: false
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - name: 检出源码
        uses: actions/checkout@v4
        with:
          repository: <org>/<repo>
          ref: ${{ github.event.inputs.VERSION || 'main' }}
          token: ${{ secrets.KPULL }}
      - name: 设置 Buildx
        uses: docker/setup-buildx-action@v3
      - name: 登录镜像仓库
        uses: docker/login-action@v3
        with:
          registry: ${{ secrets.MY_HUB_NAME }}
          username: ${{ secrets.MY_HUB_USER }}
          password: ${{ secrets.MY_HUB_PASSWORD }}
      - name: 构建并推送
        uses: docker/build-push-action@v5
        with:
          context: .
          file: ./Dockerfile-<component>
          platforms: linux/amd64,linux/arm64
          push: true
          tags: ${{ secrets.MY_HUB_NAME }}/captain/<component>:<tag>
          build-args: |
            VERSION=${{ github.event.inputs.VERSION }}
```

## 3. 收尾

- 设置合适的分支触发条件或 `workflow_dispatch` 输入参数。
- 若需私有仓库拉取，确保 `KPULL` 等密钥已配置。
- 推送前先用 `docker buildx build` 本地验证 Dockerfile 可正常构建。
- 组件命名保持 `Dockerfile-<组件名>` 与 `.github/workflows/<组件名>.yaml` 一致，便于检索。

# 环境变量与密钥

GitHub Actions 工作流使用以下密钥：

- `MY_HUB_NAME`：容器镜像仓库主机名
- `MY_HUB_USER`：镜像仓库用户名
- `MY_HUB_PASSWORD`：镜像仓库密码
- `KPULL`：用于拉取私有仓库的 GitHub Token

# 构建参数

工作流中通用的 Docker Buildx 参数：

- `--no-cache`：禁用构建缓存
- `--provenance false --sbom false`：禁用证明信息（通过 `BUILDX_NO_DEFAULT_ATTESTATIONS=1` 设置）
- `--platform linux/amd64` 或 `--platform linux/arm64`：指定目标架构
- `--output "type=image,push=true"`：构建完成后推送镜像

# 特殊目录

- `kata-image/`：Kata 容器构建体系（Makefile + 构建脚本 + 内核/rootfs 定制），详细说明见 `kata-image/README.md`
- `ingress/`：带 GmSSL 支持的自定义 NGINX ingress 构建
- `scripts/`：镜像管理工具脚本（`pullpush.sh`、`pushctr.sh`）
- `source/`：本地源码项目（`ecsnode`、`etcdcluster`、`hello-rs`）

# 智能体规范

## 重要规则
> **⚠️ IMPORTANT**: 如架构或目录有更新，或当修改涉及 Makefile 目标、PR 要求、代码生成流程、验证步骤等本文档引用的信息时，**AGENTS.md 必须同步更新**，以保持与实际项目状态一致。

## 核心原则
- **先思考，再行动**：不确定时必须提问澄清，禁止假设。
- **极致简约**：用最少代码解决问题，拒绝过度工程。
- **精准修改**：只改动任务直接相关的代码，不进行无关重构。

## 强制行为规则
- **思考方式**：所有数据分析必须编写代码（如 Python/JavaScript）执行，只返回结果，禁止将原始数据读入上下文。
- **回复风格**：常规回复限制在一个短段落内。
- **操作限制**：禁止使用 `git add -A`，必须显式指定操作的文件路径。
- **权限申请**：执行 Git Push、删除文件、安装依赖等、执行命令需要授权的操作，须先向用户申请权限，经确认后再执行。
