# FFmpeg Android 平台依赖分析

> 对象：FFmpeg（多媒体框架，C 为主，通过 NDK 交叉编译为 Android 共享库 libavcodec / libavformat / libavutil / libavfilter / libavdevice / libswscale / libswresample）。
> 数据来源：基于 FFmpeg 官方 configure 构建系统、NDK 工具链行为，以及 ffmpeg-kit / 社区 Android 交叉编译实践（2026-10-25 整理）。
> 说明：本仓库不含 FFmpeg 源码，本文档为平台依赖梳理，非针对某个具体 fork。

## 1. Android 上的构建形态

FFmpeg 在 Android 上**不走 Gradle/CMake 默认路径**，而是用自带的 `configure` + `make` 脚本做交叉编译，再用 NDK 工具链（clang + ld）产出 `.so`。

典型流程：
```bash
# 用 NDK 提供的 clang 作为交叉编译器，--cross-prefix 指向 llvm 工具链
./configure \
  --cross-prefix="$NDK/toolchains/llvm/prebuilt/.../bin/${TARGET}-" \
  --target-os=android --arch=aarch64 --enable-cross-compile \
  --sysroot="$NDK/toolchains/llvm/prebuilt/.../sysroot" \
  --extra-cflags="-fPIC" ...
make -j$(nproc)
```
产物是 7 个基础库（libav*）+ 可选的 `ffmpeg`/`ffprobe` 可执行文件，常见做法是把它们再封装进一个 JNI `.so`（如 ffmpeg-kit 的 `libffmpegkit.so`）给上层 Java/Kotlin 调用。

ABI 与 NDK target 前缀对应关系：
| Android ABI | configure --arch | NDK target 前缀 |
|---|---|---|
| arm64-v8a | aarch64 | aarch64-linux-android |
| armeabi-v7a | arm | armv7a-linux-androideabi |
| x86 | x86 | i686-linux-android |
| x86_64 | x86_64 | x86_64-linux-android |

---

## 2. NDK 系统库（FFmpeg 默认链接）

FFmpeg 核心（不开启任何外部库时）链接的 Android NDK 系统库：

| 系统库 | 用途 | 备注 |
|---|---|---|
| **libc** (bionic) | 标准 C 运行时 | 默认链接，pthread/rt 已并入 libc，**无需 -lpthread/-lrt** |
| **libm** | 数学函数 | 默认链接 |
| **libdl** | dlopen/dlsym（运行时动态加载） | 默认链接；--enable-dlopen 等特性依赖 |
| **liblog** | __android_log_print 日志 | FFmpeg 的 av_log 在 Android 上通常重定向到 logcat |
| **libz** (zlib) | gzip/deflate 解压、Matroska/MOV 等容器 | FFmpeg 内置 zlib 检测，链接 NDK 的 libz.so |
| **libc++** | C++ 运行时（libav* 含少量 C++） | --stdc++/c++_shared，需与 App 运行时一致 |

> 与本项目 ndk-multi-api-test 一样，NDK 库通过 find_library/链接器默认路径获取，不进 Gradle 依赖声明。

---

## 3. 可选外部编解码/功能库（编译期 --enable-* 决定）

这些**不是 Android 专属**，但在 Android 构建中常被一同交叉编译进 FFmpeg：

### 3.1 音视频编解码
| 库 | 功能 | configure 开关 |
|---|---|---|
| **libx264** | H.264 编码 | --enable-libx264 |
| **libx265** | H.265/HEVC 编码 | --enable-libx265 |
| **libvpx** | VP8/VP9 编解码 | --enable-libvpx |
| **libaom** | AV1 编码 | --enable-libaom |
| **libsvtav1** | AV1 编码（SVT） | --enable-libsvtav1 |
| **libdav1d** | AV1 解码 | --enable-libdav1d |
| **libmp3lame** | MP3 编码 | --enable-libmp3lame |
| **libshine** | MP3 编码（定点） | --enable-libshine |
| **libopus** | Opus 编解码 | --enable-libopus |
| **libspeex** | Speex 编解码 | --enable-libspeex |
| **libtheora** | Theora 编码 | --enable-libtheora |
| **libvorbis** | Vorbis 编码 | --enable-libvorbis |
| **libfdk-aac** | AAC 编解码（高质） | --enable-libfdk-aac |
| **libopenh264** | H.264 编解码 | --enable-libopenh264 |
| **libwebp** | 图像 | --enable-libwebp |

### 3.2 封装/滤镜/其他
| 库 | 功能 | configure 开关 |
|---|---|---|
| **libass** | 字幕渲染 | --enable-libass |
| **libfreetype** | 字体/字幕 | --enable-libfreetype |
| **libfribidi** | 双向文本 | --enable-libfribidi |
| **fontconfig** | 字体配置 | --enable-fontconfig |
| **libharfbuzz** | 文本整形 | --enable-libharfbuzz |
| **libxvid** | MPEG-4 编码 | --enable-libxvid |
| **libgsm** | GSM 音频 | --enable-libgsm |
| **libopenjpeg** | JPEG2000 | --enable-libopenjpeg |
| **libzimg** | 色彩/缩放 | --enable-libzimg |
| **libsoxr** | 重采样 | --enable-libsoxr |
| **libtwolame** | MP2 编码 | --enable-libtwolame |
| **libvidstab** | 视频稳定滤镜 | --enable-libvidstab |
| **libmysofa** | HRTF 空间音频 | --enable-libmysofa |
| **libxml2** | 字幕/DASH | --enable-libxml2 |

> 所有这些库都需先用 NDK 交叉编译为对应 ABI 的静态库/共享库，再被 FFmpeg 链接。ffmpeg-kit 就是把这些依赖全部预编译好再统一构建。

---

## 4. Android 平台专属 / 硬件加速相关

| 组件 | 形式 | 说明 |
|---|---|---|
| **MediaCodec** (libmediandk) | NDK：media/ndk/NdkMedia*.h，链接 libmediandk.so | 硬解/硬编，通过 --enable-mediacodec + --enable-jni 启用；AMediaCodec/AMediaFormat 调用 |
| **MediaNdk** (libmediandk) | 同上 | 也用于 AMediaExtractor/AMediaMuxer 解封装/封装 |
| **OpenSL ES** | NDK：SLES/OpenSLES.h，链接 libOpenSLES.so | 音频输入输出（--enable-opensles），NDK 中已废弃，被 AAudio 取代 |
| **AAudio** | NDK：aaudio/AAudio.h，链接 libaaudio.so | 低延迟音频（--enable-aaudio），需 android-26+ |
| **camera** | NDK camera (NdkCamera*.h) 经 libmediandk/libcamera2ndk | libavdevice 的 android_camera 输入设备（--enable-libcamera） |
| **JNI / libandroid** | libandroid.so | Java 与 native 互通（MediaCodec 回调、ANativeWindow 渲染需要 libandroid.so） |
| **EGL / GLES** | libEGL.so/libGLESv2.so | 硬件滤镜（如 opencl/vulkan 渲染路径） |

> 与本项目 ndk-multi-api-test 已验证的 API 一致：AAudio、ANativeWindow（libandroid）、硬件缓冲（AHardwareBuffer）都是 FFmpeg 在 Android 上可选对接的 NDK 接口。

---

## 5. 构建期工具链依赖（非运行时）

| 工具 | 作用 |
|---|---|
| **Android NDK**（r21+ 推荐，clang 工具链） | 交叉编译器、sysroot、链接器 |
| **make / ninja** | 构建驱动 |
| **pkg-config**（交叉版） | 探测外部库（如 libx264.pc 的 cflags/libs） |
| **gas-preprocessor.pl** | 汇编（如 libvpx/libx264 的汇编）在 clang 下的预处理 |
| **ndk-build / CMake**（封装场景） | ffmpeg-kit 等用 CMake 包一层 |
| **Java/Kotlin + JNI 头** | 若生成 libffmpegkit.so 等封装层，需 JDK 的 jni.h |

---

## 6. 结论：FFmpeg Android 依赖总览

| 类别 | 依赖 |
|---|---|
| NDK 系统库（默认） | libc(bionic)、libm、libdl、liblog、libz、libc++ |
| NDK 平台/硬件 API（可选） | libmediandk（MediaCodec/MediaExtractor）、libaaudio、libOpenSLES、libandroid、libcamera2ndk、libEGL/libGLESv2 |
| 外部编解码库（可选，需先交叉编译） | libx264、libx265、libvpx、libaom、libdav1d、libsvtav1、libmp3lame、libopus、libfdk-aac、libvorbis、libspeex、libtheora、libopenh264、libwebp、libshine… |
| 外部封装/滤镜库（可选） | libass、libfreetype、libfribidi、fontconfig、libharfbuzz、libxvid、libopenjpeg、libzimg、libsoxr、libxml2、libvidstab、libtwolame、libmysofa… |
| 构建工具链 | Android NDK(clang)、make/ninja、pkg-config(交叉)、gas-preprocessor、ndk-build/CMake、JDK(jni.h) |
| 运行时集成 | 通常再封装一层 JNI .so（如 libffmpegkit.so）供 Java/Kotlin 通过 System.loadLibrary 调用；c++_shared 需与 App 一致 |

要点：
1. **最小依赖** = NDK 系统库（libc/libm/libdl/liblog/libz/libc++），即可构建基础音视频处理能力的 FFmpeg。
2. **功能增强** = 按需 --enable-lib* 引入外部库，每个外部库都要先用 NDK 交叉编译。
3. **平台加速/IO** = 通过 MediaCodec、AAudio/OpenSL、camera、ANativeWindow 对接 Android 原生能力，对应链接 libmediandk / libaaudio / libOpenSLES / libandroid 等。
4. **ABI 矩阵** = 每个外部库 + FFmpeg 都要对 arm64-v8a / armeabi-v7a / x86 / x86_64 分别构建。

> 对比本项目与之前的 Mars 分析：Mars 把第三方库（Boost/OpenSSL/Zstd）**内置源码随编译**；FFmpeg 的第三方库则是**先独立交叉编译再链接**，二者管理外部依赖的方式不同，但 NDK 系统库（liblog/libz/libc++）的依赖形态一致。
