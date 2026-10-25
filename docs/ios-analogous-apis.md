# 类比 Android：iOS 提供的 OS 层 API

> 视角：把前一篇「Linux → Android」的分层类比，再映射到 iOS。回答「和 Android 类似，iOS 这个 OS 直接给开发者提供了哪些能力/API」，并给出 Linux / Android / iOS 三方可对照的清单。
> 数据来源：Apple 公开框架（iOS SDK）、XNU/Darwin 常识（2026-10-25 整理）。

---

## 0. 三层总览：Linux / Android / iOS

| 层次 | Linux | Android | iOS |
|---|---|---|---|
| ① OS 内核 + IPC | Linux 内核 + 系统调用；IPC 用 socket/pipe/**D-Bus** | 同一 Linux 内核 + Android 补丁；IPC 用 **Binder**（驱动） | **XNU**（Mach 微内核 + BSD 层）；系统调用走 BSD，IPC 用 **Mach msg / XPC** |
| ② C 标准库 | glibc（或 musl） | bionic（已合 pthread/rt） | Apple **Libc**（libSystem，BSD 派生） |
| ③ C++ 标准库 | libstdc++ / libc++ | libc++ | libc++（Xcode 默认） |
| ④ OS 平台服务/框架 | systemd、X11/Wayland、ALSA、V4L2、GStreamer、OpenGL/Mesa、OpenCL | NDK 原生库 + Java 框架服务（见上篇） | **Apple 原生框架族**（Foundation/Core * 系列、Metal、VideoToolbox…） |
| ⑤ 第三方/包管理 | Boost/OpenSSL/Qt；Make/CMake | 同左 + Gradle/Maven(Jetpack) | 同左 + **CocoaPods / SwiftPM** |
| 运行时 | 原生 ELF（可选 JVM） | **ART** 虚拟机跑 Java/Kotlin，经 JNI 调 native | Swift/ObjC 编译为**原生 ARM 机器码**（libobjc + Swift runtime），**无 VM** |

> 关键差异：Android 的 App 逻辑大多跑在 ART 虚拟机上（类似 Linux 上跑 JVM）；iOS 的 Swift/ObjC **直接编译成原生机器码**，更接近 Linux 原生模型。两者都允许内嵌 C/C++（iOS 用 bridging-header，Android 用 JNI/NDK）。

---

## 1. iOS 底层：XNU 内核

- **Mach**（微内核）：负责任务/线程/虚拟内存/IPC（`mach_msg`）。这是 iOS 的「Binder 等价物」——进程间通信的底层机制。
- **BSD 层**：提供 POSIX 兼容的系统调用、文件系统、网络（BSD socket）、信号。
- **libSystem**：把所有系统库（libc、libpthread、libm、libdispatch…）聚合成一个基础 dylib。
- 上层 IPC 框架 **XPC**：基于 Mach 消息，对标 Android 的 Binder/AIDL（系统服务间通信）。

---

## 2. iOS OS 直接提供的原生框架（对标 Android NDK 平台 API）

| iOS 框架（.framework / dylib） | 头/模块 | 提供的 OS 能力 | Android 对标 |
|---|---|---|---|
| **Foundation** (ObjC/Swift) / **Core Foundation (C)** | `Foundation/Foundation.h` `CoreFoundation/...h` | 基础对象/集合/字符串/文件；CF 是 C API，与 Foundation 无缝桥接（toll-free bridged） | bionic + libc++ 基础能力 |
| **CFNetwork / Network.framework** | `CFNetwork/...h` `Network/Network.h` | 网络（URL/TLS/连接），基于 BSD socket | 无直接 NDK 对标；用 BSD socket 自写 |
| **AudioToolbox / AudioUnit / AVAudioEngine** | `AudioToolbox/...h` | 低延迟音频单元、播放/录制 | libaaudio / libOpenSLES |
| **VideoToolbox** | `VideoToolbox/VideoToolbox.h` | **硬件编解码**（VTCompressionSession / VTDecompressionSession：H.264/HEVC） | libmediandk 的 **MediaCodec** |
| **AVFoundation** | `AVFoundation/AVFoundation.h` | 媒体播放/录制/编辑；**AVCaptureSession** 相机采集 | Camera2 NDK；MediaCodec/Extractor |
| **Core Graphics / Quartz / Core Animation / Core Image / Core Video** | `CoreGraphics/...h` 等 | 2D 绘制、图层合成、图像处理、视频帧 | SurfaceFlinger + EGL/GLES |
| **Metal** | `Metal/Metal.h` | GPU 渲染 + 通用计算（取代 OpenGL ES） | OpenGL ES / Vulkan（libvulkan） |
| **MetalKit / Metal Performance Shaders** | — | Metal 辅助/神经网络算子 | — |
| **EAGL**（OpenGL ES，已弃用） | `EAGL/eagl.h` | OpenGL ES 与窗口系统的桥接（类似 EGL） | libEGL |
| **Core ML / Apple Neural Engine (ANE)** | `CoreML/...h` | 调用 NPU 做 ML 推理 | **NNAPI**（libneuralnetworks） |
| **IOSurface** | `IOSurface/IOSurface.h` | 跨进程/跨 API 共享的 GPU/图形内存 | **AHardwareBuffer** / Surface |
| **os_log (Unified Logging)** | `os/log.h` | 系统统一日志 | liblog（`__android_log_print`） |
| **Grand Central Dispatch (libdispatch)** | `dispatch/dispatch.h` | 线程池/队列/并发调度 | pthread + 业务层线程池/Handler |
| **Darwin Notify / notify_post** | `notify.h` | 系统级轻量通知 | 部分 Binder 系统通知 |
| **SQLite / Core Data** | `sqlite3.h` | 本地存储 | SQLite（两者共通） |
| **Core Location / Core Bluetooth** | — | 定位/蓝牙 | LocationManager / Bluetooth API |

> 注意 Apple 的「框架」= 系统预装、稳定 ABI 的动态库，本质与 Android 的 `lib*.so` 平台库、以及 Java 框架服务是同一角色：**OS 直接给你的能力**。

---

## 3. iOS 与 Android 的逐能力对照表

| 能力 | Linux | Android | iOS |
|---|---|---|---|
| 内核 / IPC | Linux + socket/D-Bus | Linux + Binder | XNU + Mach msg / XPC |
| C 库 | glibc | bionic | Apple Libc(libSystem) |
| 日志 | syslog/journald | liblog(logcat) | os_log |
| 显示/窗口 | X11 / Wayland | SurfaceFlinger + ANativeWindow/EGL | UIKit/Core Animation + EAGL/Metal |
| 图形 API | OpenGL / Mesa | OpenGL ES / Vulkan | Metal（旧 OpenGL ES 已弃） |
| 音频 | ALSA / PulseAudio | AAudio / OpenSL ES | AudioUnit / AVAudioEngine |
| 相机 | V4L2 | Camera2 NDK | AVFoundation AVCapture |
| 硬件编解码 | GStreamer/FFmpeg+V4L2 | MediaCodec / NdkMedia | **VideoToolbox** |
| ML/NPU | OpenCL / 厂商 HAL | NNApi | **Core ML / ANE** |
| 共享图形内存 | DRM/GEM/GBM(dma-buf) | AHardwareBuffer | IOSurface |
| 进程间通信 | D-Bus | Binder(AIDL) | XPC / Mach msg |
| 并发模型 | pthread/线程池 | pthread + ART 线程/Handler | **GCD(libdispatch)** |
| 应用运行时 | ELF / 可选 JVM | **ART 虚拟机**(Java/Kotlin) | **原生机器码**(Swift/ObjC) |
| 包/依赖管理 | Make/CMake | Gradle/Maven(Jetpack) | CocoaPods / SwiftPM |

---

## 4. 给「Android NDK 开发者」的上手提示

- **想找 Android 的 MediaCodec 对标** → iOS 用 VideoToolbox（`VTCompressionSession`）。
- **想找 AHardwareBuffer 对标** → iOS 用 IOSurface。
- **想找 AAudio 对标** → iOS 用 AudioUnit / AVAudioEngine。
- **想找 Camera2 对标** → iOS 用 AVFoundation 的 `AVCaptureSession`。
- **想找 NNAPI 对标** → iOS 用 Core ML（底层调度 Apple Neural Engine）。
- **想找 NDK C/C++ 自由** → iOS 同样能写 C/C++，但通过 **bridging header** 与 Swift/ObjC 互调（对应 Android 的 JNI）。
- **没有 ART 这种 VM**：iOS 没有「Java 跑在虚拟机上」的层次，App 直接是原生代码，更像你在 Linux 上写 C/C++ 程序。

---

## 5. 结论

- **OS 能力三件套对照**：Linux = 内核+glibc+桌面框架；Android = Linux 内核+bionic+Binder+NDK 系统库+Java 框架；iOS = XNU+Apple Libc+Mach/XPC+Apple 原生框架族。
- Android「bionic/binder 之外」的 OS API（liblog/libandroid/libmediandk/libcamera2ndk/libaaudio/EGL·GLES·Vulkan/NNAPI/AHardwareBuffer…），在 iOS 上几乎都有一一对应（os_log / AVFoundation / VideoToolbox / AudioUnit / Metal / Core ML / IOSurface…）。
- 最大架构区别在**运行时**：Android 多一层 ART 虚拟机（Java→JNI→native），iOS 是原生机器码 + ObjC/Swift runtime，二者接入 C/C++ 的方式分别为 JNI 与 bridging header。
