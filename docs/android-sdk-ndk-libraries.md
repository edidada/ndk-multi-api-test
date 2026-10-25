# Android SDK / NDK 路径与可链接原生库分析

> 数据来源：本机实际路径扫描（2026-10-25）。所有 .so / .a 列表均来自 NDK 28.2.13676358 的 sysroot/usr/lib/<abi>/<api> 目录真实读取。

## 1. 结论速览

- **真正的 Android SDK / NDK 路径**：全部位于 `C:/Users/wdidada/AppData/Local/Android/Sdk/ndk/<version>/...`（共 3 个 NDK 版本：26.3.11579264、27.0.12077973、28.2.13676358）。
- **MyRNApp 自身产物（非 SDK）**：`D:/develops/git/github/android/MyRNApp/android/app/.cxx/Debug/.../prefab/...`，是该 React Native 应用的 prefab 原生依赖（fbjni / hermes-engine / ReactAndroid）。
- **Rust / CLion 路径（与 Android SDK 无关）**：`rustup` 与 `JetBrains CLion` 下的 `.../backtrace/ci/docker/<abi>` 只是 Rust 工具链为 Android 目标交叉编译时自带的构建脚本/索引副本，不是 Android SDK。
- **MyRNApp 实际构建的 ABI 为 4 个**：`arm64-v8a`、`armeabi-v7a`、`x86`、`x86_64`（prefab 目录中只有这 4 个，无 riscv64）。

---

## 2. 路径分类

| 类别 | 判定 | 示例路径 |
|---|---|---|
| **A. Android SDK / NDK** | 是 | `C:/Users/wdidada/AppData/Local/Android/Sdk/ndk/28.2.13676358/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/lib/aarch64-linux-android` |
| **B. MyRNApp 构建产物** | 应用层，非 SDK | `D:/develops/git/github/android/MyRNApp/android/app/.cxx/Debug/2d406i3j/prefab/arm64-v8a/prefab/lib/aarch64-linux-android` |
| **C. Rust 工具链** | 否 | `C:/Users/wdidada/.rustup/toolchains/stable-x86_64-pc-windows-msvc/lib/rustlib/src/rust/library/backtrace/ci/docker/aarch64-linux-android` |
| **D. CLion Rust 标准库副本** | 否 | `C:/Users/wdidada/AppData/Local/JetBrains/CLion2026.2/intellij-rust/stdlib-local-copy/.../library/backtrace/ci/docker/x86_64-linux-android` |

> 类别 C / D 中的 `<abi>` 目录只是 Rust 源码里 `backtrace` crate 的 docker 构建脚本片段，用于描述各 Android target 的编译环境，**不提供任何可链接的 Android 库**。

---

## 3. Android NDK 详情

### 3.1 已安装的 NDK 版本

| 版本 | 路径 |
|---|---|
| 26.3.11579264 | `.../Sdk/ndk/26.3.11579264` |
| 27.0.12077973 | `.../Sdk/ndk/27.0.12077973` |
| 28.2.13676358 | `.../Sdk/ndk/28.2.13676358` |

每个项目通过 `local.properties` 的 `sdk.dir` + `build.gradle` 的 `ndkVersion` 选用其中一个。

### 3.2 ABI 与 NDK 文件夹名映射

| Android ABI | NDK sysroot 目录名 | Clang target triple | 典型用途 |
|---|---|---|---|
| `arm64-v8a` | `aarch64-linux-android` | `aarch64-linux-android` | 64 位 ARM（主流） |
| `armeabi-v7a` | `arm-linux-androideabi` | `armv7-linux-androideabi` | 32 位 ARM |
| `x86` | `i686-linux-android` | `i686-linux-android` | 32 位 x86（模拟器） |
| `x86_64` | `x86_64-linux-android` | `x86_64-linux-android` | 64 位 x86（模拟器） |
| `riscv64` | `riscv64-linux-android` | `riscv64-linux-android` | RISC-V（NDK 26+ 有 sysroot 库，27+ 才有 clang toolchain 包装） |

> 注意 `armeabi-v7a` 在 NDK 的**库目录**叫 `arm-linux-androideabi`，而 **Clang target** 叫 `armv7-linux-androideabi`——两者都出现在你给出的路径列表中，分别对应 `usr/lib/...` 与 `build/core/toolchains/...`。

### 3.3 NDK 提供的可链接原生库（稳定公开库）

以下来自 NDK 28.2.13676358 `sysroot/usr/lib/aarch64-linux-android/35`（其他 ABI/版本同名）：

| 库文件 | 提供的能力 |
|---|---|
| `libandroid.so` | NativeActivity、NativeWindow、`AConfiguration`、传感器等原生 API |
| `libnativewindow.so` | `ANativeWindow`（Surface 交互） |
| `libaaudio.so` | AAudio 低延迟音频 |
| `libbinder_ndk.so` | `AIBinder` 等 Binder NDK 底层 API |
| `libcamera2ndk.so` | NDK Camera2 |
| `libmediandk.so` | MediaCodec / MediaMuxer |
| `libamidi.so` | MIDI |
| `libvulkan.so` | Vulkan |
| `libEGL.so` / `libGLESv1_CM.so` / `libGLESv2.so` / `libGLESv3.so` | OpenGL ES / EGL |
| `libOpenMAXAL.so` / `libOpenSLES.so` | OpenMAX AL / OpenSL ES 音频 |
| `libneuralnetworks.so` | NNAPI |
| `libjnigraphics.so` | `AndroidBitmap` JNI 图形 |
| `libicu.so` | ICU 国际化 |
| `liblog.so` | `__android_log_print`（logcat） |
| `libsync.so` | 同步栅栏（`sync_wait` 等） |
| `libz.so` | zlib |
| `libnativehelper.so` | JNI 辅助 |
| `libc++.so` / `libc++.a` | LLVM libc++（C++ 运行时，建议用 `libc++_shared.so`） |
| `libc.so` / `libm.so` / `libdl.so` | C 运行时（默认链接，无需手写） |

> 每个 ABI 目录下还有按 `minSdkVersion` 划分的 API 级别子目录（21–35），NDK 会按目标 `android-<api>` 自动挑选对应版本的库与符号版本。

---

## 4. MyRNApp 可链接的库

MyRNApp 是一个 **React Native + Rust** 的 Android 应用，其原生链接关系分三层：

### 4.1 应用构建的 ABI（4 个）

prefab 目录 `MyRNApp/android/app/.cxx/Debug/2d406i3j/prefab/` 下只有：

```
arm64-v8a/   armeabi-v7a/   x86/   x86_64/
```

即该 app 为这 4 个 ABI 编译，并**未**包含 riscv64。

### 4.2 通过 Prefab（AAR 原生依赖）引入的第三方库

`prefab/<abi>/prefab/lib/<ndk-abi>/cmake/` 下解析出的模块：

| Prefab 模块 | 对应库 | 说明 |
|---|---|---|
| `fbjni` | `libfbjni.so` | Facebook JNI 桥接助手，React Native / Rust(JNI) 通用 |
| `hermes-engine` | `libhermes.so` | Hermes JS 引擎（RN 默认 JS 运行时） |
| `ReactAndroid` | `libreactnative.so` 等 | React Native 原生端（JNI / 渲染 / 桥） |

> 该 prefab 目录中只含 `cmake/*.cmake` 配置（无直接 `.so`），实际 `.so` 由对应 AAR 在打包阶段链接进 APK。

### 4.3 应用自身原生库

- **Rust cdylib**：由 `cargo + cargo-ndk` 为每个 target（`aarch64-linux-android`、`armv7-linux-androideabi`、`i686-linux-android`、`x86_64-linux-android`）编译出的 `lib<app>.so`，再经 `.cxx`/CMake 链接进 APK。
- 上述 Rust target 与 NDK ABI 的对应见 3.2；注意 Rust 对 `armeabi-v7a` 使用的 triple 是 `armv7-linux-androideabi`（与 NDK clang 一致）。

### 4.4 链接依赖链

```
MyRNApp APK
+-- lib<app>.so            (Rust cdylib, 4 ABI)
|     -- 链接 NDK 公开库: libandroid / libnativewindow / libc++_shared / liblog ...
+-- libfbjni.so           (prefab: fbjni)
+-- libhermes.so          (prefab: hermes-engine)
+-- libreactnative.so ... (prefab: ReactAndroid)
|     -- 链接 NDK 公开库: libandroid / liblog / libEGL / GLESv* ...
+-- (系统库 libc/libm/libdl 由 NDK 默认提供)
```

---

## 5. 不是 Android SDK 的路径（避免误用）

- `C:/Users/wdidada/.rustup/.../backtrace/ci/docker/<abi>` —— Rust 标准库源码里的 docker 描述，仅用于 Rust 自身为 Android target 做 CI 构建，**不含可链接的 Android 库**。它出现的 `<abi>` 有：`aarch64-linux-android`、`arm-linux-androideabi`、`armv7-linux-androideabi`、`i686-linux-android`、`x86_64-linux-android`（**无 riscv64**，说明 Rust stable 尚未把 riscv64-android 作为一等 target）。
- `C:/Users/wdidada/AppData/Local/JetBrains/CLion*/intellij-rust/stdlib-local-copy/.../backtrace/ci/docker/<abi>` —— 同上，只是 CLion 为代码补全/索引另存的 Rust 标准库副本，与 Android SDK 无关。

---

## 6. 备注：Binder NDK 的公开符号边界

本仓库 `ndk-multi-api-test` 在 CI 中验证过：NDK 提供 `libbinder_ndk.so`，但**只导出 `AIBinder_*` 系列底层 API**（如 `AIBinder_incStrong` / `AIBinder_decStrong` / `AIBinder_getCallingPid`）。`AServiceManager_getService` / `AServiceManager_addService` 仅出现在头文件注释示例中，**不是公开导出的 NDK 符号**，普通进程也无法与 `servicemanager` 交互。因此若 MyRNApp 或本仓库需要 Binder 能力，应使用 `AIBinder` 级 API，而非 `AServiceManager`。
