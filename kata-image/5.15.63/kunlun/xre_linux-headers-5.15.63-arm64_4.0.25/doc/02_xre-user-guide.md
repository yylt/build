# 1 安装 XRE

## 1.1 环境要求

* 架构：X86、ARM
* 操作系统：Centos7、Centos8、Ubuntu 16.04~22.04、银河麒麟服务器版

## 1.2 安装方式

用户可以选择 XRE 提供的安装脚本进行一键安装，把所有 XRE 组件都安装的系统内，也可以选择手动安装方式，只安装部分组件到系统内。

昆仑驱动安装后，可以使用 `modprobe kunlun` 命令加载驱动，加载后可使用 `rmmod kunlun` 卸载驱动。

昆仑运行时库（libxpurt）安装后：

1. 除非显示指定，否则使用 xpu-clang 编译昆仑程序会默认链接安装到系统内的 libxpurt；
2. 运行动态链接的昆仑可执行程序时，除非搜索路径中有更高优的 libxpurt，否则执行时会自动加载安装到系统内的 libxpurt；

## 1.3 安装前检查

利用 lspci 命令检查昆仑设备已经被操作系统正确识别，且状态正常，主要检查以下几点：

1. 操作系统识别到的昆仑设备数量与实际机器上的昆仑卡数量一致，否则驱动加载后会发现缺少设备文件。一台含一张R200和一张K100卡的机器应当有类似下面的输出：

``` bash
$ /sbin/lspci -d 1d22: -nv
01:00.0 0780: 1d22:3684 (rev 02)
	Subsystem: 0002:0001
	Flags: bus master, fast devsel, latency 0, IRQ 158, IOMMU group 13
	Memory at 623800c000 (64-bit, prefetchable) [size=16K]
	Memory at 6210000000 (64-bit, prefetchable) [size=256M]
	Memory at 6200000000 (64-bit, prefetchable) [size=256M]
	Capabilities: <access denied>

06:00.0 1200: 1d22:3684 (rev 01)
	Flags: bus master, fast devsel, latency 0, IRQ 159, IOMMU group 16
	Memory at a0800000 (64-bit, non-prefetchable) [size=4M]
	Memory at 6000000000 (64-bit, prefetchable) [size=8G]
	Memory at a0c00000 (64-bit, non-prefetchable) [size=2M]
	Capabilities: <access denied>
```
Vendor_id: 1d22 表示该卡为昆仑芯公司的产品，device_id: 3684 表示该卡为AI加速卡产品。Subsystem: 0002:0001 表示该卡为R200标准卡，无subsystem表示该卡为昆仑1系列的K100或K200。

2. 所有板卡的 BAR 地址均分配正确，否则驱动无法正确初始化该板卡。每张卡有 3 个 Memory，一张单卡机器应当有类似下面的输出，如果有任何 Memory 地址为 0，就是分配失败。

``` bash
$ sudo /sbin/lspci -v -d 1d22: | grep Memory
Memory at b0000000 (64-bit, non-prefetchable) [size=128M]
Memory at bb200000 (64-bit, non-prefetchable) [size=64K]
Memory at ba000000 (64-bit, non-prefetchable) [size=2M]
```

3. 板卡 PCIe 连接速度正常，否则会影响 DMA 性能，正常情况下，昆仑1系列（K100&K200）Speed 有 8GT/s，Width 为 8x。昆仑2系列（R200等）Speed 有 16GT/s，Width 为 16x。一张单卡机器应当有类似下面的输出：

``` bash
$ sudo /sbin/lspci -vv -d 1d22: | grep LnkSta:
LnkSta:	Speed 16GT/s (ok), Width x16 (ok)
LnkSta: Speed 8GT/s, Width x8, TrErr- Train- SlotClk+ DLActive- BWMgmt- ABWMgmt-
```


## 1.4 一键自动安装

昆仑运行时提供安装脚本 install_rt.sh 和卸载脚本 uninstall_rt.sh，可在产出目录中找到，该脚本会把驱动和可执行程序安装到系统目录内，**必须**使用 root 权限执行该脚本。

XRE 目前支持 CentOS、Ubuntu 等若干系统，不同系统需要不同的安装文件包。这里以 Ubuntu 为例，其他系统步骤类似。

1. 解压安装包

``` bash
$ tar xzf REPLACEMENT00.tar.gz
```

2. 进入安装目录

``` bash
$ cd REPLACEMENT00
```

3. 执行安装脚本

``` bash
$ sudo bash install_rt.sh
Driver file installed
System modules map regenerated
udev rule installed.
Kunlun Runtime installed successfully.
Now load KUNLUN driver...
Done
Please execute 'export PATH=/usr/local/xpu/bin:$PATH' or add it to your .bashrc to be able to run Kunlun binaries anyware
```

4. 添加PATH（最好加入到 .bashrc 中）

``` bash
$ export PATH=/usr/local/xpu/bin:$PATH
```

5. 确认驱动已经加载

``` bash
$ lsmod | grep kunlun
kunlun            655360  0
```

6. 查看昆仑设备文件
昆仑一代的 K100 板卡对应一个设备文件，K200 板卡对应两个设备文件; 昆仑二代的 R200 板卡对应一个设备文件。

``` bash
$ ls -l /dev/xpu*
crw-rw-rw- 1 root root 243,   0 Dec 15 14:12 /dev/xpu0
crw-rw-rw- 1 root root 243,   1 Dec 15 14:12 /dev/xpu1
crw-rw-rw- 1 root root 243, 128 Dec 15 14:12 /dev/xpuctrl
```

7. 执行 xpu_smi 可以看到 K200 设备文件的信息：

![xpu_smi](http://bj.bcebos.com/ibox-thumbnail98/149601da53aca92a069d57148694933e?authorization=bce-auth-v1%2Ffbe74140929444858491fbf2b6bc0935%2F2021-01-06T09%3A22%3A53Z%2F1800%2F%2F8a5fe491b6f42fe3d325538fa23e4825295649a32853fc7a0b67d7657d45db44)

图中为一台有两张 K200 的机器，每块 K200 抽象出两个计算设备，因此一共四个计算设备。


## 1.5 手动安装

### 1.5.1 安装和加载驱动

本文档中**安装**指把驱动对应的 ko 文件注册到操作系统内，使操作系统知道有一个名为 kunlun 的驱动；而**加载**指的是把 ko 中的代码真正加载到当前正在执行的操作系统中。驱动只有安装之后，才能在操作系统重启后自动加载；只有加载之后，昆仑设备才可以被使用。安装和加载这两个步骤相互独立，可以安装但并不加载，也可以加载但并不安装。

在测试场景下，可以选择不安装直接加载。正式部署线上环境、长期使用稳定发行版的场景下，应当选择安装驱动。

#### 1.5.1.1 安装驱动

1. 把 ko 文件拷贝到 /lib/modules/$(uname -r)/extra 目录下
2. 在 /lib/modules/$(uname -r) 目录下执行 `depmod` 更新注册信息

如要从系统中删除已安装的驱动，则把 ko 文件从 /lib/modules/$(uname -r)/extra 目录中删除，然后再次执行 `depmod` 即可。

#### 1.5.1.2 加载驱动

如果驱动已经安装

```
# modprobe kunlun
```

如果驱动没有安装

```
# insmod <path_to_ko_file>
```

#### 1.5.1.3 卸载驱动

```
# rmmod kunlun
```

### 1.5.2 拷贝 udev 规则文件

```
# cp REPLACEMENT00/udev_rules/99-kunlun.rules /etc/udev/rules.d
```

udev 子系统会根据规则文件的配置来设置设备文件的访问权限，如果没有把昆仑设备规则文件拷贝到 /etc/udev/rules.d，加载昆仑驱动后生成的设备文件默认访问权限为  `crw-rw----`，只能使用 root 权限访问。

如果不希望拷贝 udev 规则文件，遇到普通用户无法访问昆仑设备文件的情况，也可以通过

```
# chmod 666 /dev/xpu*
```

来临时给设备文件加上非 root 用户的访问权限（重新加载驱动后失效）。

### 1.5.3 安装昆仑运行时库

运行时库包含头文件、动态库、静态库三个部分，安装根目录为 /usr/local/xpu。安装昆仑运行时库只需将头文件拷贝到  /usr/local/xpu/include，把库文件（.so 和 .a）拷贝到 /usr/local/xpu/lib64/ 目录下，然后将 /usr/local/xpu/lib64/ 添加到 LD_LIBRARY_PATH 环境变量中即可：

```
# mkdir -p /usr/local/xpu/lib64
# cp -r include /usr/local/xpu/
# cp so/libxpurt.so /usr/local/xpu/lib64
# cp lib/libxpurt.a /usr/local/xpu/lib64
# export LD_LIBRARY_PATH = /usr/local/xpu/lib64:$LD_LIBRARY_PATH
```

# 2 升级 XRE

1. 执行 `rmmod kunlun`，保证昆仑驱动已经从当前系统中卸载
2. 执行老版本安装包内的 `uninstall_rt.sh`，从系统中卸载当前版本的 XRE
3. 执行新版本安装包内的 `install_rt.sh`，把新版本 XRE 安装到系统内

# 3 重新加载驱动

在某些场景下，可能会需要重新加载驱动，此时只需要先 [卸载驱动](#1.5.1.3 卸载驱动)，然后再 [加载驱动](#1.5.1.2 加载驱动) 即可。

# 4 xpu_smi

## 4.1 显示设备信息

xpu_smi 可执行程序可以用来显示昆仑设备信息，运行结果如下图所示：

![xpu_smi](http://bj.bcebos.com/ibox-thumbnail98/16d1601c8162a8cf25608650c3225ca2?authorization=bce-auth-v1%2Ffbe74140929444858491fbf2b6bc0935%2F2021-09-14T04%3A35%3A44Z%2F1800%2F%2F52a1287a32bc32cf004c9bed9a637a5b826314983d264ed6fddb212d10cec6d2)

DEVICES 表格每行为一个计算设备，每列含义为：

* DevID：计算设备号
* PCI Addr：物理设备 PCI 地址
* Model：板卡型号
* SN：板卡序列号
* INODE：设备VFS文件结点
* State：设备状态，N表示Normal，E表示Error
* UseRate：利用率
* L3：高速内存用量
* Memory：主存用量
* Power(W)：功率
* Temp：温度
* Freq(MHz)：频率
* Firmware Version：固件版本
* CPLD Version：硬件版本

PROCESSES 表格显式使用昆仑设备的进程信息，每列含义为

* DevID：计算设备号
* PID：计算进程号
* Streams：计算进程包含多少Stream（包含一个默认Stream）
* L3：进程高速内存用量
* Memory：进程主存用量
* Command：进程命令行

图中的机器上有四张 K200，抽象出八个计算设备 （/dev/xpu0-7），可以看到相邻两个计算设备信息的 PCI 地址、SN信息一致，说明是同一张 K200 计算卡。

## 4.2 设置SR-IOV虚拟化功能

### 4.2.1 SR-IOV虚拟化功能简介
部分昆仑设备支持SR-IOV虚拟化功能，该功能可让单个昆仑物理设备(Physical Function, PF)虚拟出多个昆仑虚拟设备(Virtual Function, VF)。每个VF设备独立拥有所属物理设备的部分或者全部算力和设备内存，各VF设备可独立工作，不会相互影响。

### 4.2.2 SR-IOV板卡支持情况
| 板卡型号 | 芯片型号 | 内存大小 | 最大可支持VF数量 |
| -------- | -------- | -------  | ---------------- |
| R200     | KLX2-S1P | 16GB     | 3                |
| R200_8F  | KLX2-S1P | 32GB     | 3                |
| R200_8FS | KLX2-S1P | 32GB     | 3                |
| RG800    | KLX2-S1P | 32GB     | 3                |
| R300     | KLX2-S1P | 32GB     | 3                |

### 4.2.3 SR-IOV资源分配说明

1. **昆仑2 SR-IOV资源分配表**

| 资源\SR-IOV模式 | PF/1VF | 2VF   | 3VF    |
| --------------- | ------ | ----- | ------ |
| Cluster	      | 8	   | 3	   | 2      |
| SDNN	          | 6	   | 3	   | 2      |
| L3	          | 64M	   | 32M   | 16M    |
| Device Memory	  | 16G	   | 8G	   | 约5G   |
| Dec/Enc/Img     | 9/3/6  | 4/1/3 | 3/1/2  |

上表中,Device Memory为ECC关闭时的值，若开启ECC，Device Memory将减少约1/8以（xpu_smi显示为准）。

### 4.2.4 SR-IOV配置方式说明
使用 `xpu_smi -d <device_id> -V <num_vfs>` 或 `xpu_smi -s <pcie_addr> -V <num_vfs>` 命令可设置指定昆仑设备的SR-IOV虚拟化功能。其中参数**num_vfs**表示目标虚拟设备(VF)个数，若num_vfs为0，表示关闭虚拟化功能，修改配置时需确保PF及相关VF当前均未被相关进程使用,否则配置失败。

以下是在R200(xpu0)上开关SR-IOV虚拟化功能的示例:

1.**开启SR-IOV**

使用命令`$ xpu_smi -d 0 -V 3`可在设备0开启SR-IOV，开启后设备状态如下所示(已省略部分的信息):
```
$ xpu_smi
Runtime Version: 4.0
Driver Version: 4.0
  DEVICES
--------------------------------------------------------------------------------------------------
|DevID |   PCI Addr   |       Model       | ... |    INODE   | ... |    L3     |    Memory   | ...
--------------------------------------------------------------------------------------------------
|    0 | 0000:01:00.0 | R200-PF     | ... | /dev/xpu0  | ... | 0 / 63 MB | 0 / 13568 MB|
|    1 | 0000:01:01.0 | R200-VF-5G  | ... | /dev/xpu1  | ... | 0 / 15 MB | 0 / 4006 MB | ...
|    2 | 0000:01:01.1 | R200-VF-5G  | ... | /dev/xpu2  | ... | 0 / 15 MB | 0 / 4006 MB |
|    3 | 0000:01:01.2 | R200-VF-5G  | ... | /dev/xpu3  | ... | 0 / 15 MB | 0 / 4006 MB |
--------------------------------------------------------------------------------------------------
```
如上所示，设备0开启SR-IOV(VF个数为3)之后，其Model从R200变为**R200-PF**，同时新增了三个设备(ID分别为1、2、3)，对应的Model均为**R200-5G**。此时设备0不再具备计算能力，仅用做SR-IOV功能控制及设备状态查询。

2.**关闭SR-IOV**

使用命令`$ xpu_smi -d 0 -V 0`可关闭设备0的SR-IOV,关闭后设备状态如下所示(已省略部分的信息):

```
$ xpu_smi
Runtime Version: 4.0
Driver Version: 4.0
  DEVICES
---------------------------------------------------------------------------------------
| DevID |   PCI Addr   | Model | ... | INODE     | ... |    L3     |    Memory    | ...
---------------------------------------------------------------------------------------
|     0 | 0000:01:00.0 | R200  | ... | /dev/xpu0 | ... | 0 / 63 MB | 0 / 13568 MB | ...
---------------------------------------------------------------------------------------
```
如上所示，设备0此时已经恢复正常初始状态, Model变回R200。

# 5 PROC 文件系统

除了使用 xpu_smi 命令外，昆仑驱动也会使用 PROC 文件系统向用户暴露一些驱动、设备相关的信息。可以通过 `cat` 命令查看这些文件的内容，部分可以修改的文件可以通过 `echo` 命令修改其内容。完整文件目录如下所示：

```
├── auto_reset
├── dev0
│   ├── errinfo
│   ├── errtask
│   ├── info
│   ├── limits
│   ├── mminfo
│   ├── profile
│   ├── regular_timer_toggle
│   ├── stash_for_ltloop
│   ├── state
│   ├── tqinfo
│   └── use_ratio
├── load_time
├── probe_error
├── version
└── wait_mode
```

根目录（/proc/xpu）下的文件会显示驱动相关的信息，设备目录（如/proc/xpu/dev0）下则显示和设备相关的信息，这里设备目录是与设备文件一一对应的，/proc/xpu/dev0 就对应 /dev/xpu0。

驱动相关信息包括

| 文件名      | 描述                                                         |
| ----------- | ------------------------------------------------------------ |
| auto_reset  | 是否开启了自动复位功能，0代表否，1代表是。可通过 echo 0 或 1 来动态修改配置（仅对昆仑1生效）。 |
| load_time | 设备驱动加载时内核jiffies计数值。 |
| probe_error | 显示设备初始化过程中发生了什么错误。每行对应一个错误设备，每行信息自左向右依次为：PCIE地址、SN号、设备编号、板卡型号、错误码、错误原因。如无错误发生，则结果为空。 |
| version     | 驱动版本号。                                                   |
| wait_mode   | 内部实现相关，客户无需关注（仅对昆仑1生效）。                                   |

设备相关信息包括

| 文件名      | 描述                                                         |
| ----------- | ------------------------------------------------------------ |
| errtask | 设备错误任务信息。记录上一次发生异常的计算核（Kernel）信息，包括具体异常的原因、计算核名称、配置、参数等。主要用来调试和记录问题，多为内部使用，客户不太需要关注。errtask 信息一次复位后不会清除，但再次复位后会被清除。|
| info | 设备的基本信息，包括板卡型号、PCIE地址、SN、固件版本、硬件版本、驱动版本。 |
| limits | 设备资源可用限制，PLVL limits和cgroup limits。 |
| mminfo | 设备可用内存资源信息，包括L3缓存和MAIN缓存。 |
| profile | 性能分析工具。 |
| regular_timer_toggle | regular定时器开关。 |
| stash_for_ltloop | ?? |
| state | 当前设备工作状态，正常为 RUNNING。 |
| tqinfo | SSE工作队列状态信息。 |
| use_ratio | 设备资源使用率。 |

# 6 固件烧写

## 6.1 固件说明

在发布包中firmware/路径下存放昆仑1和昆仑2发布的不同版本固件。

使用`xpu_smi`工具可查看当前固件版本，如下表所示
| Firmware Version  |
| ----|
| 0001.0002.0022 |
对应固件版本为1.2.22。

固件分为三部分，分别对应三位版本号。如下表为xpu_smi信息中版本号对应的烧写文件。
| 版本号 | 昆仑1 | 昆仑2 |
| --- | --- | --- |
| 第一位(1) | flash_burner.online | pbl.1.x.x.ota |
| 第二位(2) | flash_init_bootloader.online.x.2.x | pciefw.x.2.x.ota|
| 第三位(22) | main.online.x.x.22 | sbl.x.x.22.ota |
| | |

**昆仑1**

当前稳定版本为 1.0016.0021(x86机器)，1.0015.0022(飞腾机器)。

**昆仑2**

当前稳定版本为2.4.0029 。

## 6.2 固件烧写

昆仑1使用ota工具(该工具位于 tools/ 路径下）对固件进行烧写。详细操作可执行`./ota -h`查看。
昆仑2使用mcu_util工具(该工具位于 tools/ 路径下）对固件进行烧写。具体操作命令为
`./mcu_util <dev> ota <file_path>`。


# 7 常见问题

## 7.1 驱动加载问题

### 7.1.1 缺少设备文件

加载驱动后，如果 /dev/xpu* 设备文件显示不玩完整（`xpu_smi` 只能看到部分设备文件），或者 /proc/xpu/probe_error 文件内有内容，说明驱动加载、初始化设备的过程中出错。

这种情况下首先排除硬件连接和系统问题，根据 [安装前检查](#1.3 安装前检查) 中的步骤，检查操作系统是否识别到了每个昆仑设备，且每个设备均正确的分配了 BAR 地址。

- 如果有未识别的卡，则应当尝试重启，重启后依然不行请尝试关机重新插拔或更换槽位后重启。多次尝试均不识别，大概率是卡出现了损坏；
- 如果有设备没有正确分配 BAR 地址，也首先尝试重启系统，无法解决后请确认 BIOS 内 '“Above 4G decoding' 选项已经打开，如果确认该选项开启后还不行，可能是系统内计算卡过多导致 IO 空间不足；

如果硬件链接没有问题，则查看 /proc/xpu/probe_error 中给出的错误原因，一般情况下设备初始化失败的原因有：

* **CPU memory not enough**，分配 CPU 内存失败，可重启后再次尝试
* **System error**，大概率是设备没有正确分配 BAR 地址

### 7.1.2 缺少当前内核版本的驱动

如果执行 [一键自动安装](#1.4 一键自动安装) 提示

````
Cannot find kunlun driver file ***
````

说明 XRE 产出缺少与当前内核版本匹配的驱动，可以更换 XRE 支持的内核版本（ *driver/* 目录下有对应的 ko 文件），或是联系昆仑团队申请与所需内核版本匹配的 KO。

### 7.1.3 安装驱动与当前内核版本不匹配

如果在手动 [加载驱动](#1.5.1.2 加载驱动) 的时候提示

```
insmod: error inserting 'output/kunlun_module/kunlun.ko': -1 Invalid module format
```

说明所加载的 ko 文件与当前系统内核不匹配。可以通过 `modinfo kunlun.ko` 命令查看 `vermagic` 来确认 ko 所对应的内核版本，`vermagic` 必须与 `uname -r` 的输出匹配。
