# 1 昆仑运行时

昆仑运行时库（XPU Runtime Library，以下简称 XRT）具体指的是 `libxpurt` 库，使用昆仑运行时的程序可以在编译时选择静态连接 `libxpurt.a` 或是动态连接 `libxpurt.so`。开发者使用 XRT，需要在代码中引用 XRT 头文件 `xpu/runtime.h`，XRT 所提供的函数与数据类型均定义在该头文件中。

XRT 提供类 CUDA 编程接口，帮助开发者进行设备管理、内存管理、计算核调用、并行度管理等功能。

# 2 使用样例

下面是一段简单的XPU Demo，演示了简单的 `axpby` 计算核的实现，以及调用该计算核的 CPU 代码，可以使用 *xpu-clang* 编译器编译后执行。

``` C++
#include "xpu/kernel/cluster_header.h"
#include "xpu/kernel/simd.h"
#include "xpu/runtime.h"
#include <cassert>
#include <cstdlib>

// XPU计算核axpby，该函数会被编译为XPU可执行的二进制指令流
// y = a * x + b * y
__global__ void axpby(float* x, float* y, int len, float a, float b) {
    int cid = core_id();
    int ncores = core_num();
    if (cid >= ncores) {
        return;
    }
    const int buf_size = 1024;
    __local__ float local_x[buf_size];
    __local__ float local_y[buf_size];

    int len_per_loop = buf_size;
    for (int i = cid * len_per_loop; i < len; i += ncores * len_per_loop) {
        int read_len = len_per_loop;
        if (i + read_len > len) {
            read_len = len - i;
        }
        // y = a * x + b * y + c
        GM2LM(x + i, local_x, read_len * sizeof(float));
        GM2LM(y + i, local_y, read_len * sizeof(float));
        for (int k = 0; k < read_len; k += 8) {
            _x256_svmul_ls(a, &local_x[k], &local_x[k]);
            _x256_svmul_ls(b, &local_y[k], &local_y[k]);
            _x256_vvadd_ls(&local_x[k], &local_y[k], &local_y[k]);
        }
        LM2GM(local_y, y + i, read_len * sizeof(float));
    }
}
// CPU代码入口
int main() {
    float *x_cpu = NULL, *y_cpu = NULL;
    void *x_xpu = NULL, *y_xpu = NULL;
    int len = 4096;
    int err = 0;

    // 设定当前工作设备为 /dev/xpu0
    err = xpu_set_device(0);
    assert(err == 0);

    // 为X和Y分配CPU内存
    x_cpu = (float *)malloc(len * sizeof(float));
    assert(x_cpu != NULL);
    y_cpu = (float *)malloc(len * sizeof(float));
    assert(y_cpu != NULL);

    // 为X和Y分配XPU内存
    err = xpu_malloc(&x_xpu, len * sizeof(float));
    assert(err == 0);
    err = xpu_malloc(&y_xpu, len * sizeof(float));
    assert(err == 0);

    // 在CPU上初始化输入X和Y
    for (int i = 0; i < len; ++i) {
        x_cpu[i] = i;
        y_cpu[i] = 2.0f;
    }

    // 把输入X和Y拷贝到XPU上
    err = xpu_memcpy(x_xpu, x_cpu, len * sizeof(float), XPU_HOST_TO_DEVICE);
    assert(err == 0);
    err = xpu_memcpy(y_xpu, y_cpu, len * sizeof(float), XPU_HOST_TO_DEVICE);
    assert(err == 0);

    // 调用XPU axpby计算核
    err = axpby<<<4, 16, NULL>>>(x_xpu, y_xpu, len, 1.0f, 1.0f);
    assert(err == 0);

    // 等待axpby计算完成
    err = xpu_wait();

    // 把计算结果拷贝回CPU
    err = xpu_memcpy(y_cpu, y_xpu, len * sizeof(float), XPU_DEVICE_TO_HOST);
    assert(err == 0);

    // 检查计算结果正确性
    // ...

    return 0;
}
```

Demo 中展示了使用 XPU 计算比较重要的几个步骤：

1. 设置当前工作设备
2. 准备输入数据（XPU内存）
3. 调用计算核
4. 等待执行完成

调用任何 XPU 计算核，都大体上遵循这个过程，这些步骤的意义与细节将在下面展开说明。

# 3 基本功能

## 3.1 选择计算设备

开发者要想使用昆仑设备进行计算，必须首先告诉 XRT 要使用的具体是哪个设备，如果开发者不指定*工作设备*，XRT 会选取*默认设备*。

### 3.1.1 计算设备和物理设备

昆仑**计算设备**由昆仑驱动抽象并提供给开发者使用，每个昆仑*计算设备*对应一个 */dev* 目录下的设备文件，设备文件命名方式为 *xpu{devid}*，例如 */dev/xpu0*、*/dev/xpu1*，正常情况下，昆仑计算设备的设备号都是从 0 开始递增的。一个昆仑**物理设备**就是一张昆仑卡，可能对应多个*计算设备*（K200包含两个计算设备，K100包含一个），因此，即使系统内只有一张 K200，也依然能看到 */dev/xpu0* 和 */dev/xpu1* 两个*计算设备*。

使用昆仑的第一步是利用 `xpu_set_device` 接口指定一个*计算设备*为**当前工作设备**（Current Device），所有 XRT 提供的函数都是执行在*当前工作设备*上的，如果系统中存在多个昆仑*设备*，开发者需要显式地告诉 XRT 接下来要工作在哪个设备上。

一般而言，与开发者直接相关的概念都是*计算设备*，因此本文档中出现的*设备*一词默认都表示*计算设备*这个概念，当需要表示物理昆仑卡的时候，会使用*物理设备*一词。

### 3.1.2 默认设备与设备列表

XRT 的**默认设备**为*设备列表*中的第一个。如果开发者没有调用  `xpu_set_device`，那么 XRT 会自动设置*默认设备*作为*当前工作设备*，因此在编程的时候需格外注意，如果在显式调用 `xpu_set_device` 之前就已经调用了其他 XRT 函数（如 `xpu_malloc`），那么之前的函数实际上是执行在*默认设备*上的。

 `xpu_device_count` 可以获取当前系统上可用设备的个数， `xpu_device_list` 可以获取可用的**设备列表**。开发者可以通过设置 `XPU_VISIBLE_DEVICES` 环境变量来显式设定*设备列表*，否则 XRT 会自动查看操作系统 */dev/* 目录下所有的 xpu 设备文件，并按照设备号递增的顺序返回。例如 `XPU_VISIBLE_DEVICES=0,2,3`，那么 `xpu_device_count` 返回值为 3，`xpu_device_list` 返回值为长度为 3 的数组 `[0, 2, 3]`。

## 3.2 内存管理与拷贝

```c
// 分配设备内存
int xpu_malloc(void** pdevptr, uint64_t sz, XPUMemoryKind kind = XPU_MEM_HBM);
// 释放设备内存
int xpu_free(void* devptr);
// 内存拷贝
int xpu_memcpy(void* dst, const void* src, uint64_t sz, XPUMemcpyKind kind);
```

开发者调用 XPU 计算核需要使用偏上存储作为输入输出，XRT 提供以上接口来管理昆仑片上存储空间（包括主存和高速缓存），包括内存的分配、释放、主机与设备间的内存拷贝等。

XRT 内存管理相关的接口均在*当前工作设备*上执行，在调用时需格外注意。拷贝与释放时的工作设备如果与分配时的工作设备不同，是一种未定义的行为，可能会导致运行时状态错误。

内存拷贝支持两个方向，`HOST_TO_DEVICE` 把一段数据从 CPU 内存拷贝到指定的昆仑内存上；`DEVICE_TO_HOST` 则正相反。XRT 提供的内存拷贝接口为同步接口，也就是说 `xpu_memcpy` 函数返回，代表内存拷贝已经结束。

## 3.3 计算核调用

计算核（Compute Kernel）是一段可被昆仑设备执行的二进制指令流，用来完成某种计算操作，如矩阵乘法等。开发者可以使用 `<<< >>>` 语法进行计算核调用：

```c
kernel_name<<<nblock, nthread, stream>>>(args...)；
```

其中：

* `nblock` 表示数据分块个数，不同的分块可以同时执行，一般情况下，`nblock == 4` 的情况下昆仑执行效率较高；
* `nthread` 表示每个分块内的计算线程数，昆仑有两种计算单元，XPU-Cluster 类型的计算核最多支持 16 个线程，XPU-SDNN 类型的计算核最多支持 8 个线程；
* `stream` 表示昆仑执行流，这一概念会在 [后面章节](#4.1 昆仑执行流) 详细介绍，这里 `stream` 参数可以不指定，使用默认执行流；

### 3.3.1 等待计算核完成

计算核调用是一个**异步接口**，也就是说当 `axpby<<<4, 8>>>(args...);` 返回的时候，计算核 `axpby` 只是被写入到命令队列中去了，并未真正执行完成，开发者需要调用 `xpu_wait` 来显式同步计算状态，等待计算核完成。利用异步调用的机制，开发者可在短时间内调用大量计算核进入命令队列，之后进行一些 CPU 计算，然后再利用 `xpu_wait` 函数等待所有已调用的计算核完成计算。

### 3.3.2 结果拷回

由于计算核调用时异步调用，开发者如果希望把计算结果从 XPU 拷回到 CPU，必须先使用 `xpu_wait` 同步执行状态，否则无法拷回正确的计算结果。

### 3.3.3 DMA 与计算核的并行执行

昆仑运行时提供的 `xpu_memcpy` 是同步接口，调用后立刻执行，因此如果开发者需要将一段内存拷贝和某些计算核并行起来，可以先通过异步接口将计算核发送到昆仑设备上，然后进行内存拷贝，即可实现内存拷贝与计算核的并行。

``` c
// kernel1、kernel2 顺序执行，memcpy 可能与 kernel1、kernel2 同时执行
kernel1<<<...>>>(...);
kernel2<<<...>>>(...);
xpu_memcpy(dst_xpu, src_cpu, size, XPU_HOST_TO_DEVICE);
```

值得说明的是，在此机制下如果要保证计算核执行和 DMA 之间的先后顺序（`dst_xpu` 必须在 kernel2 执行完成后才能被写），需要在 CPU 端主动调用 `xpu_wait` 同步执行流状态。

``` c
// kernel1、kernel2 顺序执行完成后，xpu_wait()返回，然后开始 memcpy
kernel1<<<m, n>>>(...);
kernel2<<<m, n>>>(...);
xpu_wait();
xpu_memcpy(dst_xpu, src_cpu, size, XPU_HOST_TO_DEVICE);
```

> 一个常见的计算场景是，计算核执行完成之后利用内存拷贝把计算结果拷回到 CPU，为了简化开发者在这一场景下的负担，昆仑运行时在 `xpu_memcpy` 接口中加入了自动同步的功能。具体来说，当且仅当用户在进行 `DEVICE_TO_HOST` 方向的内存拷贝时，运行时会自动同步默认执行流（详见 [昆仑执行流](#4.1 昆仑执行流)）。注意自动同步仅会发生在默认执行流上，并且仅对 `DEVICE_TO_HOST` 这一个方向有效，如果用户使用了自定义的执行流，则需要显式的对命令队列进行同步。

# 4. 进阶功能

## 4.1 昆仑执行流

如果开发者希望昆仑**同时执行两个计算核**，需要利用到**执行流**（Stream）机制。昆仑执行流本质上是一个软件队列，可以方便开发者管理计算核的执行顺序，同一个执行流内的计算核（也包括事件）按照调用顺序依次执行，不同执行流内的计算核可以同时执行。

昆仑执行流的使用比较简单，用户通过 `xpu_stream_create` 创建一个自定义执行流，然后在计算核调用和 `xpu_wait` 的时候将自定义执行流作为参数传入即可。如果我们的 `axpby` 样例想要使用执行流机制，需要做如下改变：

``` c
int main() {
    // 其他变量声明
    // ...
    XPUStream s1;

    // 创建自定义执行流 s1
    err = xpu_stream_create(&s1);
    assert(err == 0);

    // 其他初始化代码相同
    // ...

    // 调用XPU axpby计算核
    err = axpby<<<4, 16, s1>>>(x_xpu, y_xpu, len, 1.0f, 1.0f);
    assert(err == 0);

    // 等待axpby计算完成
    err = xpu_wait(s1);
    assert(err == 0);

    // 结果拷回检查正确性
    // ...
}
```

### 4.1.1 默认执行流

在我们的 [axpby 样例](#2 使用样例) 和基本功能介绍中，并未使用执行流，程序依然可以正确执行，这是因为 XRT 维护了**默认执行流**（NULL Stream），当开发者不需要考虑复杂的执行控制和极致的性能优化的时候，使用*默认执行流*可以简化代码复杂度。在 XRT 所有涉及到 `XPUStream` 的接口中，指定 `stream = NULL` 或者不指定该参数（默认参数为 NULL）即使用*默认执行流*。默认执行流内的计算核也可以与自定义执行流内的同时执行。

*默认执行流*是一个**进程级状态**，也就是说*同一个进程在每个设备上都只有一个默认执行流*。如果进程内使用了多线程，且每个线程内都使用*默认执行流*，那么多线程发射出去的计算核实际上不会并行执行，而是在同一个执行流内顺序执行。

### 4.1.2 使用多个执行流

回到「同时执行两个计算核」这一目的，开发者必须使用 2 个执行流来实现。以下伪代码样例简单展示了如何在同一个线程内，通过两个自定义执行流，达到同时计算多个计算核的目的。

```c
// 该样例中使用了两个自定义执行流 s1 和 s2
// 其中 kern0、kern1 在 s1 上，kern2、kern3 在 s2 上
// 实际执行的时候，1 会在 0 完成后开始执行，3 会在 2 完成后开始执行，但是整体上 0/1 和 2/3 可以同时执行
XPUStream s1, s2;

// 创建 s1 和 s2
xpu_stream_create(&s1);
xpu_stream_create(&s2);

// 把 kern0 和 kern1 发射到 s1 上
kern0<<<4, 8, s1>>>(...);
kern1<<<4, 8, s1>>>(out1_xpu, ...);

// 把 kern2 和 kern3 发射到 s2 上
kern2<<<4, 8, s2>>>(...);
kern3<<<4, 8, s2>>>(out3_xpu, ...);

// 拷回 kern1 的计算结果，拷贝前需要显示同步 s1
xpu_wait(s1);
xpu_memcpy(out1_cpu, out1_xpu, out1_size, XPU_DEVICE_TO_HOST);

// 拷回 kern3 的计算结果，因为 kern3 在自定义执行流上，这里需要显示同步
xpu_wait(s2);
xpu_memcpy(out3_cpu, out3_xpu, out3_size, XPU_DEVICE_TO_HOST);
```

## 4.2 多线程

本节主要说明在单进程多线程场景下，XRT 会有怎样的行为。

### 4.2.1 多线程设备管理

*当前工作设备*是一个**线程级**的状态，因此同一个进程下不同的线程，可以通过设定不同的*工作设备*，达到一个进程同时利用多个设备的效果。但是需要注意，当一个线程被新创建的时候，其*当前工作设备*处于未定义状态，如果开发者未通过 `xpu_set_device` 显式指定，那么这个新线程会使用*默认设备*。

一个*常见的错误*是，开发者在主线程上指定*工作设备*为 *xpu1*，然后创建了线程2，并以为线程2的*工作设备*也是 *xpu1*，这是不对的，如果没有显式指定，线程2会使用默认设备 *xpu0*。

### 4.2.2 多线程共用执行流

执行流本身是一个线程安全的机制，也就是说可以支持多个 CPU 线程同时往同一个执行流内发射计算核，但这种情况下，由于发射顺序与线程执行顺序等动态信息相关，因此多线程发射的计算核之间没有确定的执行顺序保证。

正如在 [默认执行流](#4.1.1 默认执行流) 一节中提到的，多个线程同时使用默认执行流（或者相同的自定义执行流）发射计算核，这些计算核在硬件上其实是串行执行的。

### 4.2.3 多线程并行计算

如果希望多个线程同时使用一个昆仑设备，且不同线程调用的计算核可以同时计算，最好的方法是给每一个线程创建一个新的执行流，然后该线程所有的计算都在这一执行流上完成。

## 4.3 使用多个昆仑设备

如果一台机器上有多张昆仑设备，开发者需要选择某个具体的设备作为计算设备，我们比较推荐的做法是，为每个昆仑设备创建一个工作线程，然后在线程内通过 `xpu_set_device` 指定其所使用的设备。这里需要注意 [多线程设备管理](#4.2.1 多线程设备管理) 一节提到注意事项：

1. 每个线程均需在启动的时候显式地通过 `xpu_set_device` 指定工作设备；
2. 尽量避免在主线程分配资源（执行流、内存等），然后在工作线程使用的情况，容易出现主线程在 *xpu0* 上分配内存，子线程在 *xpu1* 上尝试使用这段内存的情况。如果一定需要，必须保障主线程分配资源时的*当前工作设备*与工作线程所使用的设备一致；

如果必须在一个线程内同时使用多个设备。开发者也可以频繁调用 `xpu_set_device` ，在多个设备之间来回切换。但由于内存分配、拷贝、执行流创建销毁、计算核调用、同步等接口全部受到*当前工作设备*的影响，尤其是计算内存资源是与工作设备强绑定的，因此在编写代码的时候需要格外注意，避免出现把在 A 设备上分配的内存作为参数传给一个要在 B 设备上执行的计算核这样的f错误
