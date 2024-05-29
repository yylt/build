# 1. XRE 介绍

XRE（XPU Runtime Environment） 主要包含以下内容：

1. 昆仑芯驱动（kunlun_module/kunlun*.ko）和配套 udev 规则文件（udev_rules/99-kunlun.rules）
2. 昆仑芯运行时头文件（include/xpu/runtime.h）
3. 昆仑芯运行时库（so/libxpurt.so, lib/libxpurt.a）
4. 昆仑芯相关可执行程序（如 bin/xpu_smi）
5. 昆仑芯运行时工具箱 （tools/)
6. 昆仑芯运行时脚本工具（scripts/)
7. 昆仑芯设备固件 (firmware/kunlun1, firmware/kunlun2)

**昆仑芯驱动**负责具体与昆仑计算设备通信。在 Linux 系统上，昆仑芯驱动加载后会对设备进行初始化，并通过文件系统向用户暴露**设备文件**（/dev/xpu0、/dev/xpu1 等），用户态计算程序可以通过这些文件接口与昆仑芯设备进行通信。

通过设备文件暴露的接口为底层接口，对开发者很不友好，**昆仑芯运行时**（包括头文件和库文件）对这些接口进行了进一步封装，向用户提供了更为友好的开发接口，开发者在XPU上编程的时候，只会接触到运行时接口。

昆仑芯驱动的生命周期是与操作系统相关的，而运行时则嵌入在计算进程内部，不同的计算进程内有不同的运行时实例，这些实例互不感知，驱动则拥有全局信息。

XRE提供固件烧写工具(ota)和可用固件，用于测试和调试的各种小工具。此外XRE 还提供了查询、管理设备状态的可执行程序。

# 2. XRE Release Note

v4.0

- 支持完整昆仑1和昆仑2
- 昆仑2支持虚拟化
- 更完善的异常处理

v3.8

- 减少BAR0使用量至4M
- xpu_malloc如果size为0，返回XPUERR_INVALID_PARAM
- 更新全新的设备查询接口
- 计算超时会将卡置为异常状态，并记录errtask/errinfo（errtask只做参考，并不总是准确）
- XPURT_DISPATCH_MODE支持BATCH值
- OTA工具更新NoDriver模式

v3.7

- KL1添加Event支持
- XRE使用文档
- 使用Pingpong缓存优化端到端DMA性能

v3.6

- SSE AXI异常后会进入auto reset（如果开启）
- BUGFIX: PD1异常未正确处理

v3.5

- 驱动加载检查固件版本
- 记录驱动加载jiffies
- fix: XPU_SMI内存显式不一致问题
- fix: RESET流程与TIMER冲突问题

v3.4

- 添加并默认使用Hybrid模式，改善xpu_wait阶段CPU利用率
- soft_rst添加--no_pcie_init选项，配合22版本的fw，解决复位过程中pcie断连接（触发NMI）的问题
- 解决DMA Coherence问题

v3.3

- FW训练HBM失败后，驱动会尝试多次重训练HBM
- 添加xpu_br/xpu_bw接口
- 添加运维相关proc文件，reset_count/errtask

v3.2

- run_sse_tc 工具添加 --reloac 选项支持

v3.1

- SN号支持64bit和32bit两种版本
- 添加/proc/xpu/probe_error，展示未成功初始化的卡信息和错误信息
- 添加基本的reset流程

v3.0

- 为设备文件添加设备信息查询接口，针对对3.0以后版本的驱动，SMI不再使用contrlnode作为查询接口，
  而是通过xpu_device_list获取设备列表后依次查询
- 添加交叉编译的选项
