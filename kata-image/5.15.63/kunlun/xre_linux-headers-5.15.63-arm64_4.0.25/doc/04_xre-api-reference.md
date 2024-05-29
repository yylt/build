# 1 版本管理

``` c
/// \brief  返回驱动版本号。
/// \param[out] major  返回主版本号
/// \param[out] minor  返回副版本号
int xpu_get_driver_version(uint32_t* major, uint32_t* minor);

/// \brief  返回当前驱动的GIT提交哈希值。
/// \param[out] commit  返回GIT提交哈希值字符串
int xpu_get_driver_commit(char* commit);

/// \brief  返回XPU Runtime版本号。
/// \param[out] major  返回主版本号
/// \param[out] minor  返回副版本号
int xpu_get_runtime_version(uint32_t* major, uint32_t* minor);

/// \brief  返回当前版本XPU Runtime的GIT提交哈希值。
/// \param[out] commit  返回GIT提交哈希值字符串
int xpu_get_runtime_commit(char* commit);
```

# 2 设备管理

``` c
/// \brief  设定当前工作设备。
///
///         当前工作设备是一个线程级的状态，设定当前工作设备之后，后续所有的内存分配、搬运、
///         Stream管理以函数调用都作用于该设备。如果用户希望在一个线程内同时使用两个设备，
///         需要确保利用该函数进行设备切换。
///
///         如果该函数从未被调用，默认使用0号设备。
/// \param[in] devid  目标设备号
int xpu_set_device(int devid);

/// \brief  获取当前工作设备。
/// \param[out] devid  返回当前工作设备设备号
/// \return \ref XPU_SUCCESS, \ref XPUERR_INVALID_PARAM
int xpu_current_device(int* devid);

/// \brief  获取当前系统中XPU设备的个数。
/// \param[out] dev_count  返回XPU设备个数
int xpu_device_count(int* dev_count);

/// \brief  获取当前系统中XPU设备的设备号列表。
///         Caller负责分配并传入dev_list数组，其大小为xpu_device_count()个int，
///         Runtime通过该数组返回当前进程可见的XPU设备号。
///         注意Caller需要正确的分配dev_list数组，Runtime无法对该数组进行检查，如果Caller
///         传入了非法地址，可能会导致内存访问错误。
/// \param[out] dev_list  XPU设备号列表
int xpu_device_list(int* dev_list);

/// \brief  获取当前工作设备的信息。
///
///         在 \p v 中返回 \p attr 所指定的设备信息， \p v 本身类型为 uint64_t，根据
///         所查询信息的不同，返回值的有效 bit 位数也不尽相同。当前支持查询的设备信息为：
///         - \ref XPUATTR_PCI_ADDRESS ：物理设备的PCI地址，其中比特域[31:16][15:8][7:3][2:0]
///           分别表示 Domain、Bus、Slot（Device）、Function
///         - \ref XPUATTR_DEVID：计算设备的 ID，对应 /dev/xpu{ID}
///
/// \param[out] v  返回所要查询的设备信息值
/// \param[in] attr  指定索要查询的设备信息
/// \param[in] devid  目标设备号
/// \return \ref XPU_SUCCESS, \ref XPUERR_INVALID_PARAM
int xpu_device_get_attr(uint64_t* v, XPUDeviceAttr attr, int devid);
```

# 3 内存管理

``` c
/// \brief  在当前工作设备上分配内存。
/// \param[out] pdevptr  返回分配的XPU设备地址
/// \param[in]  sz       所需分配的内存大小，单位为字节
/// \param[in]  kind     分配的内存种类，默认值为 \ref XPU_MEM_MAIN ，如果希望在L3内存上分配，
///                      使用 \ref XPU_MEM_L3
int xpu_malloc(void** pdevptr, uint64_t sz, XPUMemoryKind kind = XPU_MEM_MAIN);

/// \brief  释放当前工作设备上的内存。
/// \param[in] devptr  XPU Pointer to be freed
int xpu_free(void* devptr);

/// \brief  在CPU和XPU件进行内存拷贝。
/// \param[in] dst   目标地址
/// \param[in] src   源地址
/// \param[in] sz    拷贝字节数
/// \param[in] kind  内存拷贝种类, 可以是 \ref XPU_DEVICE_TO_HOST ，
///                  \ref XPU_HOST_TO_DEVICE 或 \ref XPU_DEVICE_TO_DEVICE
int xpu_memcpy(void* dst, const void* src, uint64_t sz, XPUMemcpyKind kind);
```

# 4 执行流管理

``` c
/// \brief  在当前设备上创建一个Stream。
/// \param[out] pstream  指向待创建的Stream
int xpu_stream_create(XPUStream* pstream);

/// \brief  销毁一个Stream。
/// \param[in] stream  待销毁的Stream。
int xpu_stream_destroy(XPUStream stream);

/// \brief  等待目标Stream上所有的函数调用完成。
///
///         如果不指定Stream，则等待默认Stream上所有的函数调用完成。
/// \param[in] stream  待同步的XPU Stream
int xpu_wait(XPUStream stream = NULL);
```

# 5 错误码

所有运行时提供的接口，均会返回一个 `int` 标记函数执行是否成功，成功则返回 0，否则返回一个负数，其绝对值为错误码。开发者应当在使用的时候检查函数的返回值，遇到错误及时上报，避免出现错误了很久程序依然在执行的情况。XRT 提供了如下函数可以获取错误码的字符串描述：

``` c
const char *xpu_strerror(int errno);
```

昆仑运行时定义的错误码如下：

**701**：非法设备

> Invalid XPU device

运行时打开昆仑设备的时候出错了。

可能原因与解决方法：

* 检查 `/dev/xpu{N}` 设备文件是否存在，如果不存在，则可能是驱动安装失败，进一步定位是否是操作系统未识别设备或设备状态异常。当然也可能是计算程序写错了参数
  * 可以通过 `lspci` 命令确认设备是否存在以及设备状态是否正确
* 检查设备文件的权限，如果当前用户不可读写该设备文件，可以用 root 执行测试程序，或者使用 `sudo chmod` 修改设备文件的读写权限

**703**：无可用设备

> No available XPU device

当前系统内未找到任何可用的工作设备。在用户未指定工作设备的场景下，运行时会根据 `xpu_device_list` 返回值选择默认的工作设备，如果该函数返回空列表，则触发该错误。

可能原因与解决方法：

* 是否没有安装驱动
  * 通过 `lsmod | grep kunlun` 判断
* 如驱动已经安装，大概率是系统未识别到昆仑设备，请参考 XRE 使用手册检查系统状态

**705**：设备内存不足

> Device memory not enough

一般 `xpu_malloc` 会返回此错误，表示片上内存不足。

**706**：CPU 内存不足

> CPU memory not enough

运行时运行过程中需要分配一些 CPU 内存，如果分配失败则会触发此错误。

**707**：非法参数

> Invalid parameter

传入参数未通过合法性检查。

**713**：设备处于异常状态

> Device abnormal due to previous error

当计算设备出现 714、715、808 错误后会进入异常状态后，此后再次进行计算核调用、`xpu_wait` 等会返回 713 错误。这一错误并不是由报错函数引起的，而是由于之前的错误使设备状态异常，此时需要进行系统复位才可以恢复正常执行；若要进行问题定位，则需要查看异常日志，看最早是由什么错误引起的。

**714**：计算核执行异常

> Kernel execution exception

计算核执行异常，请保存 dmesg、*/proc/xpu/devN/errtask*文件的信息并反馈给昆仑团队。

**715**：计算核执行超时

> Kernel execution timed out

计算核执行超时，请保存 dmesg、*/proc/xpu/devN/errtask*、*/proc/xpu/devN/tqinfo* 等信息并反馈给昆仑团队。

**808**：DMA 超时

> DMA timed out, a reboot maybe needed

一般是由硬件状态异常引起的，这种情况下需要**系统重启**以进行 PCIe 复位，如果重启后依然出现 808，请反馈给昆仑团队。

**809**：DMA 异常

> DMA aborted due to error, possibly wrong address or hardware state

绝大部分情况下，809 都是因为传入的昆仑地址或 CPU 地址非法导致的，这种情况下 809 错误不会影响后续的正常 DMA，请检查代码逻辑和传入参数是否正确。

小部分是因为 PCIe 状态异常，这种情况下所有的 DMA 都会出现 809 错误，此时需要**系统重启**以复位 PCIe。
