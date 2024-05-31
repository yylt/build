#  常见错误码解释及解决方法
本文档列举了一些常见的错误码和对应的解决方法，请在有昆仑 xre 产出包的前提下，参照下文解决问题

## 1. [ERROR][XPURT] Initialize default context for dev -1 fail

### 原因：

找不到昆仑设备，有四种原因：a.机器尚未插昆仑卡; b.机器已经插卡但是 pcie 未识别；c.pcie 可以识别但是没有装昆仑驱动; d.驱动已经安装但用户进程没有昆仑设备的权限

### 推荐解决方法：

#### 首先检查机器是否插卡以及 pcie 是否已经识别：

``` bash
$ /sbin/lspci -d 1d22: -vvv
# 操作系统识别到的昆仑设备数量应当与实际机器上的昆仑卡数量一致，例如两卡机器应当看到如下两块输出：
37:00.0 Class 1200: Device 1d22:3684 (rev 01)
        Control: I/O- Mem+ BusMaster+ SpecCycle- MemWINV- VGASnoop- ParErr- Stepping- SERR- FastB2B- DisINTx+
....
....

38:00.0 Class 1200: Device 1d22:3684 (rev 01)
        Control: I/O- Mem+ BusMaster+ SpecCycle- MemWINV- VGASnoop- ParErr- Stepping- SERR- FastB2B- DisINTx+
....
....

```

如果已经插卡但 lspci 没有返回值，可以通过重启机器/重新插拔卡的方法解决，直到pcie可以正常识别

#### 确认pcie正常识别后，检查设备是否有昆仑驱动：

``` bash
# 4.0 以前版本采用：
$ lsmod | grep xpu_driver
# 预期输出如下：
xpu_driver            655360  0

#4.0 及以后：
$ lsmod | grep kunlun
# 预期输出如下：
kunlun               1944359  4
```

如没有预期输出，请参考 02_xre-user-guide.md 安装 xpu 驱动

#### 如驱动已经安装请检查昆仑设备权限，正确的文件访问权限应该是 `crw-rw-rw-`

``` bash
$ ls -l /dev/xpu*
crw-rw-rw- 1 root root 243,   0 Dec 15 14:12 /dev/xpu0
crw-rw-rw- 1 root root 243,   1 Dec 15 14:12 /dev/xpu1
crw-rw-rw- 1 root root 243, 128 Dec 15 14:12 /dev/xpuctrl
```

如权限错误，可以进行权限修改：

``` bash
$ sudo chmod +666 /dev/xpu*
```

## 2. [WARN][XPURT][xpu_llwait:729] ioctl() fail, (713) Device abnormal due to previous error

### 原因：
设备状态异常，需要进行软复位

### 推荐解决方法：

#### 首先检查卡的状态, 如错误进程用的是dev0, 命令如下, 其他 dev 进行数字替换即可
``` bash
cat /proc/xpu/dev0/state
# 预期输出为 RUNNING 或 NORMAL
# 如输出为 ERROR 表明卡状态异常了
```

#### 执行软复位

注意如果用的是昆仑1 K200，对某个 dev 进行软复位时需要保证该 dev 所在物理卡上没有任何进程在使用该卡，

可以通过 xpu_smi 查看，同一张物理卡上两个 dev 的 PCI Addr 是一致的，例如下图中/dev/xpu0 和 /dev/xpu1 在同一张物理卡上

杀掉所有该物理卡上的进程后，才能执行软复位

``` bash
Runtime Version: 4.0
Driver Version: 4.0
  DEVICES
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
| DevID |   PCI Addr   | Model |        SN        |    INODE   | State | UseRate |    L3     |    Memory    | Power(W) | Temp | Freq(MHz) | Firmware Version | CPLD Version |
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
|     0 | 0000:01:00.0 | R200  | 02K00Y6217V00009 | /dev/xpu0  |     N |     0 % | 0 / 63 MB | 0 / 16384 MB |       25 |   49 |      1300 | 0001.0002.0022   |          103 |
|     1 | 0000:06:00.0 | K100  | 000000000000022f | /dev/xpu1  |     N |     0 % | 0 / 16 MB |  0 / 8064 MB |       22 |   51 |       900 | 0001.0015.0022   |       eb1302 |
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  PROCESSES
-------------------------------------------------
| DevID | PID | Streams | L3 | Memory | Command |
-------------------------------------------------
-------------------------------------------------
```

如果用的是昆仑1 K100, 或者昆仑2，同样需要杀掉该 dev 上的所有进程后，执行软复位

可以使用如下的命令查看打开某个昆仑设备的所有进程号：

``` bash
$ sudo lsof /dev/xpu0
COMMAND     PID USER   FD   TYPE DEVICE SIZE/OFF      NODE NAME
face-gene 26038 jenf  131u   CHR  242,0      0t0 156131938 /dev/xpu0
media_str 26590 jenf   28u   CHR  242,0      0t0 156131938 /dev/xpu0
```

复位命令(昆仑1、昆仑2通用)：
``` bash
$ ./tools/soft_reset [devid]
# 如/dev/xpu0状态异常，则执行：
$ ./tools/soft_reset 0
```

mcu_util复位命令(仅对昆仑2生效，内部使用）：

``` bash
$ ./output/tools/mcu_util [devid] reset mode0
# 如/dev/xpu0状态异常，则执行：
$ ./output/tools/mcu_util 0 reset mode0
```

如复位未能正常完成，或复位完成后卡的状态没有回到 RUNNING 或者 NORMAL，则需要重启机器。
