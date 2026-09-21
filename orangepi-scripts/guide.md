# Orange Pi 5 Pro 内核构建指南（orangepi-scripts）

本目录用于把 [orangepi-xunlong/orangepi-build](https://github.com/orangepi-xunlong/orangepi-build)
（`next` 分支）裁剪后，仅编译 **Orange Pi 5 Pro（rk3588）** 的内核，并把产物打包成镜像推送到私有仓库。

核心目标：

- 拉取上游 `orangepi-build` `next` 分支
- 应用本目录的补丁（板级配置 + 内核配置 + 自定义扩展）
- 只编译内核（`BUILD_OPT=kernel`），支持 `4k` / `16k` 两种页大小
- 去除 MALI / DRM（panfrost 及其 helper）相关配置
- 把编译出的内核 `*.deb` 装入 alpine 镜像并上传到 `MY_HUB`，tag 使用时间

配套文件：

- 工作流：`.github/workflows/opi.yaml`
- 镜像：根目录 `Dockerfile-opi`

---

## 1. 目录说明

```
orangepi-scripts/
├── guide.md                         # 本说明
└── opi5pro-current.d/               # 需要覆盖进 orangepi-build 仓库的文件（覆盖目录）
    ├── opi5pro-4k.patch             # 不含页大小修改（沿用上游默认 4k）
    ├── opi5pro-16k.patch            # 含 16k 页大小修改
    └── external/
        └── patch/kernel/
            ├── 0000-log-bcmdhd.patch   # 关闭 bcmdhd 驱动的噪声日志
            └── 0002-log-fuxi.patch     # 关闭 fuxi 网卡驱动的噪声日志
```

`opi5pro-current.d/` 下所有路径都是相对于 `orangepi-build` 仓库根目录的，直接在 checkout 出来的仓库里 `git apply` 即可。

两个 `*.patch` 文件的区别：

| 文件 | 内容 | 页大小 |
|------|------|--------|
| `opi5pro-4k.patch` | 板级 + 内核配置 + 自定义扩展，沿用 `next` 分支默认的 4k | 4k（上游默认） |
| `opi5pro-16k.patch` | 板级 + 内核配置 + 自定义扩展，并把页大小设置为 16k | 16k |

> 两个文件都**已去除** MALI / DRM（`CONFIG_MALI*`、`CONFIG_DRM_PANFROST`、
> `CONFIG_DRM_GEM_SHMEM_HELPER`、`CONFIG_DRM_SCHED`）。

补丁对 `orangepi-build` 仓库的改动点：

1. `external/config/boards/orangepi5pro.conf`：追加 `enable_extension "custom-extension"`
2. `external/config/kernel/linux-rockchip-rk3588-current-opi5pro.config`：内核配置（BPF、TCP、MPTCP、
   页大小、去除 MALI/DRM 等）
3. `external/extensions/custom-extension.sh`（新建）：在内核配置阶段应用
   `external/patch/kernel/0000-log-bcmdhd.patch` 与 `0002-log-fuxi.patch`

---

## 2. 使用方法

### 2.1 通过 GitHub Actions（推荐）

工作流 `.github/workflows/opi.yaml` 已封装全部流程，手动触发即可：

1. 在仓库的 **Actions → opi** 页面点击 **Run workflow**
2. 选择 `page`：`4k` 或 `16k`（默认 `16k`）
3. 工作流会：
   - 拉取本仓库 + `orangepi-build` `next` 分支
   - 根据 `page` 选择 `opi5pro-4k.patch` / `opi5pro-16k.patch` 并 `git apply`
   - 复制两个 log 补丁到 `external/patch/kernel/`
   - 用官方 `external/config/templates/Dockerfile` 起容器，在容器内执行
     `BOARD=orangepi5pro BRANCH=current BUILD_OPT=kernel KERNEL_CONFIGURE=no`
   - 收集 `output/debs/*.deb`，用 `Dockerfile-opi` 打包为 `linux/arm64` 镜像
4. 结果镜像推送到：

   ```
   ${MY_HUB_NAME}/arm64v8/opi-kernel:<时间戳>
   ${MY_HUB_NAME}/arm64v8/opi-kernel:<page>-latest
   ```

> 运行器使用 `ubuntu-22.04`（x86_64）。`orangepi-build` 在 amd64 上会自动下载
> aarch64 交叉工具链完成内核交叉编译，无需 arm64 机器。

### 2.2 本地手动构建

```bash
# 1) 获取上游代码
git clone --depth 1 --branch next https://github.com/orangepi-xunlong/orangepi-build.git
cd orangepi-build

# 2) 应用补丁（4k 或 16k 二选一）
git apply ../orangepi-scripts/opi5pro-current.d/opi5pro-16k.patch      # 16k
# git apply ../orangepi-scripts/opi5pro-current.d/opi5pro-4k.patch       # 4k

# 3) 复制 log 补丁（custom-extension 会用到）
mkdir -p external/patch/kernel
cp ../orangepi-scripts/opi5pro-current.d/external/patch/kernel/*.patch external/patch/kernel/

# 4) 用官方 Docker 镜像在容器内编译内核（仅内核）
docker build -t opi-builder -f external/config/templates/Dockerfile .
docker run --rm --privileged -v "$PWD:/root/orangepi" opi-builder \
  BOARD=orangepi5pro BRANCH=current BUILD_OPT=kernel KERNEL_CONFIGURE=no

# 5) 收集产物
ls output/debs/
```

### 2.3 把产物打包成镜像

```bash
mkdir -p opi-artifacts
cp output/debs/linux-image-*.deb output/debs/linux-headers-*.deb \
   output/debs/linux-dtb-*.deb output/debs/linux-libc-dev-*.deb opi-artifacts/

cd opi-artifacts
docker buildx build --platform linux/arm64 \
  --build-arg PAGE=16k \
  --tag ${MY_HUB_NAME}/arm64v8/opi-kernel:$(date +%Y%m%d%H%M) \
  --file ../Dockerfile-opi .
```

> 内核 `*.deb` 是 glibc（Debian/Ubuntu）用户态产物，alpine（musl）无法直装，
> 因此 `Dockerfile-opi` 把它们作为**产物**放进 `/opt/opi-debs`，再 `docker cp` 或挂载取出刷机。

---

## 3. orangepi-build 速览（精简）

> 以下内容只保留与本目录相关的关键概念；完整文档见上游仓库。

### 3.1 关键构建变量

| 变量 | 说明 | 本目录取值 |
|------|------|-----------|
| `BOARD` | 目标板型 | `orangepi5pro` |
| `BRANCH` | 内核分支 | `current`（rk3588 对应内核 `orange-pi-6.1-rk35xx`） |
| `BUILD_OPT` | 构建选项 | `kernel`（仅内核；另有 `u-boot`/`rootfs`/`image`） |
| `KERNEL_CONFIGURE` | 是否交互配置内核 | `no` |
| `RELEASE` | 发行版 | `jammy`（仅 image/rootfs 必需，kernel 也可带） |

### 3.2 配置与扩展的优先级

orangepi-build 支持多种可提交到 git 的定制方式（均不依赖被 `.gitignore` 排除的 `userpatches/`）：

| 方式 | 路径 | 用途 |
|------|------|------|
| 板级配置 | `external/config/boards/<board>.conf` | 板型定义、启用扩展 |
| 内核配置 | `external/config/kernel/linux-<family>-<branch>[-<board>].config` | 内核编译选项 |
| 补丁 | `external/patch/{kernel,u-boot,atf}/<family>-<branch>/` | 源码级补丁，按文件名字母序应用 |
| 扩展 | `external/extensions/<name>.sh` | 通过 Hook 在构建各阶段插桩 |

本目录用的是「板级配置 + 内核配置 + 扩展」组合：板级配置 `enable_extension` 启用
`custom-extension`，该扩展在内核配置阶段（`custom_kernel_config__*`）把两个 log 补丁应用进内核树。

### 3.3 扩展 Hook 要点

扩展脚本通过函数命名约定挂载到构建流程：`hook_point__extension_func`。

与本目录相关的内核 Hook：

- `pre_config_kernel__*` / `post_config_kernel__*`：内核配置前/后
- `kernel_config__*`：修改内核配置
- `post_compile_kernel__*`：内核编译后

Hook 可用数字前缀控制顺序（`100_` 先、`500_` 中、`900_` 后）。

### 3.4 常见问题

- **修改了内核配置但没生效？** 确认改的是板型实际引用的 config：
  `orangepi5pro` 在 `BRANCH=current` 下引用
  `linux-rockchip-rk3588-current-opi5pro.config`。
- **自定义补丁没应用？** 补丁目录名必须等于 `KERNELPATCHDIR`
  （current 分支为 `rockchip-rk3588-current`），且文件名按字母序。
- **想加自己的驱动 / 日志补丁？** 在 `opi5pro-current.d/external/patch/kernel/` 增加补丁，
  并在 `custom-extension.sh` 的 `PATCH_FILE` 数组里登记即可。

---

## 4. 修改本目录

- **调整内核配置**：直接改 `opi5pro-4k.patch` / `opi5pro-16k.patch`
  中对应的 hunk（建议从 `opi5pro-16k.patch` 出发，再用脚本拆分）。
- **新增 log / 源码补丁**：放进 `opi5pro-current.d/external/patch/kernel/` 并更新
  `custom-extension.sh` 的 `PATCH_FILE`。
- **切换板型 / 分支**：修改工作流里的 `BOARD` / `BRANCH`，并确认引用的 config 文件名正确。
