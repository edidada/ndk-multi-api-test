# Android 开发中的 OS 层 API（bionic / binder 之外）

> 视角：以 Linux C/C++ 开发者熟悉的「分层」来类比 —— 操作系统内核 + 系统调用、C/C++ 标准库、OS 提供的平台服务/框架 API、再到第三方库。
> 本文回答：在 Android 上，对应 Linux 那些「系统平台能力」（X11/ALSA/D-Bus/systemd/GStreamer 等）的 **OS 提供 API** 都有哪些，除了已经熟知的 bionic（C 库）和 binder（IPC）。
> 数据来源：Android NDK 公开稳定原生 API（android-ndk 文档）与 Android 框架常识（2026-10-25 整理）。

---

## 0. 类比框架：Linux C/C++ 开发的层次

| 层次 | Linux 下的典型组成 | 作用 |
|---|---|---|
| ① OS 内核 + 系统调用 | Linux 内核；open/read/write/fork/socket/epoll/mmap… 经 glibc 暴露 | 最底层的「OS 能力」 |
| ② C 标准库 | glibc（或 musl） | 基础运行时 |
| ③ C++ 标准库 | libstdc++ / libc++ | C++ 运行时 |
| ④ OS 平台服务 / 框架 API | systemd、D-Bus、X11/Wayland、ALSA/PulseAudio、V4L2、GStreamer/FFmpeg、OpenGL/Mesa、OpenCL | 内核之上「OS 直接提供」的能力 |
| ⑤ 第三方库 / 框架 | Boost、OpenSSL、Qt、SDL、Protobuf… | 生态补充 |

---

## 1. 对应到 Android 的分层

| 层次 | Android 组成 | 与 Linux 的关系 |
|---|---|---|
| ① OS 内核 + 系统调用 | **同一个 Linux 内核**，但打了 Android 专属补丁：wakelocks、binder 驱动、ashmem/ion、lowmemorykiller、paranoid networking 等 | 系统调用接口与 Linux 基本一致，经 bionic 暴露 |
| ② C 标准库 | **bionic**（替代 glibc），已集成 pthread/rt；无 -lpthread/-lrt | 对应 glibc |
| ③ C++ 标准库 | **libc++**（libc++_shared / libc++_static） | 对应 libstdc++/libc++ |
| ④ OS 平台服务 / 框架 API | **本文重点**：NDK 暴露的一套稳定原生平台 API + Java 框架服务（见第 2、3 节） | 对应 Linux 的 ④ 层 |
| ⑤ 第三方库 / 框架 | 同 Linux 的库可交叉编译进 Android；Java 侧有 Jetpack / OkHttp 等 | 对应 ⑤ 层 |

> 用户已熟悉的 **binder** 属于 ①/④ 之间的 IPC 机制（类 D-Bus）；**bionic** 属于 ②。下面专讲 ④ 层「bionic / binder 之外，Android OS 还直接给了你哪些能力」。

---

## 2. NDK 原生平台 API（OS 直接提供，bionic/binder 之外）

这些都是 Android 在 NDK 中以「稳定 ABI」形式承诺的 OS 能力，链接对应系统 .so 即可用：

| 系统库(.so) | 头文件 | 提供的 OS 能力 | Linux 类比 |
|---|---|---|---|
| **liblog** | `android/log.h` | `__android_log_print` 日志输出到 logcat | syslog / journald |
| **libandroid** | `android/native_window.h` `android/native_activity.h` `android/configuration.h` `android/input.h` `android/sensor.h` `android/asset_manager.h` `android/bitmap.h` `android/trace.h` `android/choreographer.h` | NativeActivity、ANativeWindow、AInputQueue、传感器、Asset 读取、Bitmap、ATrace、AChoreographer(vsync) | X11/Wayland + libinput + evdev + 传感器子系统 |
| **libnativewindow** | `android/hardware_buffer.h` `android/hardware_buffer_jni.h` | **AHardwareBuffer**：跨进程/跨 API（GL/Vulkan/视频/相机）共享的图形内存 | DRM/GEM/GBM(dma-buf) |
| **libmediandk** | `media/NdkMedia*.h` `media/NdkImage*.h` | **MediaCodec** 硬件编解码、MediaExtractor、MediaMuxer、AImageReader | GStreamer/FFmpeg + V4L2 |
| **libcamera2ndk** | `camera/NdkCamera*.h` `camera/NdkCameraMetadata*.h` | **Camera2** 相机采集（NdkCameraDevice/Manager） | V4L2 |
| **libaaudio** | `aaudio/AAudio.h` | 低延迟音频流（需 android-26+） | ALSA |
| **libOpenSLES** | `SLES/OpenSLES.h` `SLES/OpenSLES_Android.h` | 音频播放/录制（旧 API，已被 AAudio 取代） | ALSA/PulseAudio |
| **libEGL / libGLESv1_CM / libGLESv2** | `EGL/egl.h` `GLES/gl.h` `GLES2/gl2.h` `GLES3/gl3.h` | OpenGL ES 渲染上下文/绘制 | OpenGL/EGL(Mesa) |
| **libvulkan** | `vulkan/vulkan.h` | 现代跨平台 GPU API（计算/图形） | Vulkan(Mesa) |
| **libneuralnetworks (NNAPI)** | `neuralnetworks/ANeuralNetworks.h` | 调用片上 NPU/加速器的 ML 推理 | （无直接对标，类似 OpenCL/HAL 抽象） |
| **libjnigraphics** | `android/bitmap.h` | Java Bitmap 与 native 像素互操作 | — |
| **libz** | `zlib.h` | zlib 压缩 | zlib |
| **libm / libdl** | 已并入 bionic | 数学/动态加载 | libm/libdl |

> 本项目 `ndk-multi-api-test` 已验证其中 AAudio、ANativeWindow(`libandroid`)、AHardwareBuffer、binder(`AIBinder_*`) 的可链接性 —— 这些都是「OS 直接给的 API」而非第三方库。

---

## 3. Java 框架层 OS 服务（App 开发者看到的「OS API」）

在 native 之上一层，Android 通过 **ART 虚拟机 + Binder** 向 Java/Kotlin 暴露整套系统服务（对应 Linux 的 systemd/D-Bus 一类「系统服务」角色）：

- **组件模型 OS API**：Activity / Service / BroadcastReceiver / ContentProvider（四大组件，由 ActivityManager/SystemServer 调度）
- **Manager 服务**（均经 Binder 跨进程）：
  - ActivityManager、PackageManager、WindowManager
  - NotificationManager、LocationManager、ConnectivityManager
  - TelephonyManager、SensorManager、PowerManager、AudioManager
  - StorageManager、WallpaperManager、ClipboardManager …
- **图形/窗口合成 OS**：SurfaceFlinger（合成）/ View 体系（对应 Linux 的 X11/Wayland 合成器）
- **运行时**：ART（AOT+JIT），对应 Linux 上跑 JVM——Java/Kotlin 跑在 VM 里，native 经 JNI 调用

> 类比：Linux 桌面程序的「OS 服务」是 systemd + D-Bus + X/Wayland；Android 的等价物是 SystemServer 里的各 Manager + Binder + SurfaceFlinger。

---

## 4. 第三方 / 框架层类比（⑤ 层）

| Linux 第三方 | Android 等价（native / Java） |
|---|---|
| Boost / OpenSSL / Protobuf | 同样可交叉编译进 NDK；Java 侧常用同款（如 conscrypt 包 OpenSSL） |
| Qt / SDL | Qt for Android、SDL2（均基于上面 ②/④ 层 API） |
| GStreamer | 可移植到 Android；或用 OS 自带的 MediaCodec(NdkMedia) |
| FFmpeg | 见本仓库 `docs/ffmpeg-android-dependencies.md`（底层就是 ②④ 层） |

---

## 5. 小结：Android「OS 提供的 API」清单（bionic/binder 之外）

1. **日志**：liblog（logcat）—— 对标 syslog。
2. **图形/窗口/输入**：libandroid（ANativeWindow/NativeActivity/Input/Sensor）+ libnativewindow（AHardwareBuffer）+ libEGL/GLES/Vulkan —— 对标 X11/Wayland/GL/Mesa。
3. **音频**：libaaudio / libOpenSLES —— 对标 ALSA/Pulse。
4. **相机**：libcamera2ndk（Camera2）—— 对标 V4L2。
5. **媒体编解码/封装**：libmediandk（MediaCodec/Extractor/Muxer/ImageReader）—— 对标 GStreamer/FFmpeg+V4L2。
6. **ML 加速**：libneuralnetworks（NNAPI）—— 对标 OpenCL/厂商 HAL。
7. **系统服务/UI 框架**：Java 框架层各大 Manager + SurfaceFlinger + ART —— 对标 systemd/D-Bus/X/Wayland。
8. **压缩/数学/动态加载**：libz / libm / libdl（含于 bionic）。

> 一句话：**bionic 是「C 库」（②），binder 是「IPC 机制」（①↔④ 的通道）；而 Android OS 真正“像 Linux 的 X11/ALSA/D-Bus 那样直接给你的平台能力”，是第 2、3 节那一整张 NDK 系统库 + Java 框架服务表。**
