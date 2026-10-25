# 第三方 C/C++ 库能否交叉编译进 Android？（以 libevent 为例）

> 问题：Linux 上的第三方 C/C++ 库/框架（如 libevent）能否交叉编译进 Android？怎么移植？
> 结论：**可以**。libevent（纯 C 事件通知库）已在新建工程 `android-libevent-demo` 中实测成功：用 Android NDK 交叉编译出 arm64-v8a 静态库，并链接进 native 可执行文件，CI 已验证产物为 `ELF 64-bit ARM aarch64`（链接 /system/bin/linker64）。
> 数据来源：android-libevent-demo 工程的 GitHub Actions 实测（2026-10-25）。

---

## 1. 原理：Android NDK 提供交叉编译器

Android 与 Linux 共用同一个 Linux 内核，且 NDK 自带一套 **LLVM/clang 交叉编译工具链**，因此可以像在 Linux 上做常规交叉编译一样把第三方 C/C++ 库移植到 Android：

- 编译器：`$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android<API>-clang`（C/C++ 包装，已内置 sysroot 与 target）。
- CMake：用 NDK 自带的 `$NDK/build/cmake/android.toolchain.cmake` 作为 toolchain 文件，传入 `-DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24`。
- 汇编/链接：`llvm-ar` / `llvm-ranlib` / `ld`（lld）。

> 类比：在 Linux 上给 ARM 板子交叉编译用 `arm-linux-gnueabihf-gcc`；在 Android 上等价物就是 NDK 的 `aarch64-linux-android<API>-clang`。

---

## 2. 通用交叉编译流程

### 2.1 用 CMake 的库（如 libwebsockets、nCine 依赖等）
```bash
cmake -S . -B build-android \
  -DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24 \
  -DCMAKE_INSTALL_PREFIX="$PWD/prebuilt" \
  -D<LIB>_ROOT=...   # 类似本项目 -DLIBEVENT_ROOT
cmake --build build-android && cmake --install build-android
```

### 2.2 用 autotools（configure）的库（如 libevent 2.1.x）
```bash
export CC="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android24-clang"
export CXX=...clang++ ; export AR=llvm-ar ; export RANLIB=llvm-ranlib
export STRIP=llvm-strip
./configure --host=aarch64-linux-android \
  --prefix="$PWD/prebuilt" --enable-static --disable-shared \
  --disable-openssl ...      # 按需关闭不需要的特性
make -j$(nproc) && make install
```

### 2.3 依赖传递
若目标库还依赖其它库（如 libwebsockets 启用 SSL 需要 mbedTLS/OpenSSL），**先把这些依赖也按上面任一方式交叉编译**，再用对应 `-D*_ROOT` / `--with-*` 指过去。本项目 libevent 关闭了 OpenSSL，所以无需额外依赖。

---

## 3. 移植时的关键差异 / 坑（已在 libevent 实测中踩到并解决）

| 问题 | 现象 | 解决 |
|---|---|---|
| **pthread 在 bionic 中并入 libc** | libevent 的 CMake `find_package(Threads)` 在 NDK 上 `CMAKE_HAVE_LIBC_PTHREAD` 检测失败，导致配置报错 | 改用 libevent 的 `configure` 并 `--disable-thread-support`（定时器演示不需要线程）；或在 CMake 构建时避开 Threads 检测 |
| **NDK 没有 `-lpthread` 独立库** | 链接 `-lpthread` 找不到库（bionic 已把 pthread 集成进 libc） | 不链 `-lpthread`；需要线程时由 libc 提供 |
| **可能需要 `-ldl`** | 部分库用到 dlopen | 链接时显式加 `dl`（bionic 提供） |
| **CMake 源目录路径** | 在 build 子目录里把 `.` 当源码目录，报“找不到 CMakeLists.txt” | configure/cmake 的源码路径要指向 libevent 根目录（`..`） |
| **GitHub tag 归档不含 configure** | `release-x.y.z.tar.gz` 自动归档只有 `configure.ac`，没有生成的 `configure` | 改用官方 release 资产 tarball（如 `libevent-2.1.12-stable.tar.gz`），它内置生成好的 `configure` |
| **C++ 运行时一致** | 多 so 混用不同 libc++ 可能冲突 | 统一使用 NDK 的 `libc++_shared`（或全静态） |

---

## 4. 实测工程：android-libevent-demo

> 仓库：https://github.com/edidada/android-libevent-demo （公开）

结构：
- `src/main.cpp`：用 `event_base` 建 0.5s 周期定时器，触发 3 次退出；Android 用 `__android_log_print`，Linux 用 `printf`（同一份代码跨平台）。
- `build_libevent.sh`：Android 用 NDK clang + libevent `configure` 交叉编译 arm64-v8a 静态库；传空 NDK 路径则 host 原生 CMake 构建。
- `CMakeLists.txt`：把预编译 libevent 链接成 `libevent_demo`，兼容 NDK 与 Linux。
- `.github/workflows/build.yml`：两个 job。

CI 结果（均绿）：
- `android` job：产物 `file` 确认 = `ELF 64-bit LSB pie executable, ARM aarch64 ... interpreter /system/bin/linker64`，即真实 arm64 Android 可执行文件，已链接 libevent，作为 artifact 上传。
- `linux-smoke` job：host 编译并**实际运行**同一份代码，日志打印 `timer fired #1/#2/#3` + `libevent demo done`，证明 libevent 逻辑可用（无需 Android 设备即可冒烟）。

---

## 5. 哪些库容易移植，哪些要注意

| 类别 | 可否交叉编译进 Android | 说明 |
|---|---|---|
| 纯 C/C++、用 CMake/autotools 的库（libevent、sqlite、zlib、libpng…） | ✅ 容易 | 直接套上面流程，注意 pthread/dl/libc++ |
| 依赖其它原生库的库（OpenSSL、mbedTLS、FFmpeg 的外部编解码库…） | ✅ 可行 | 先把依赖交叉编译好再指向 |
| 提供 NDK 原生 API 的库（已见本仓库 ndk-multi-api-test） | ✅ | 直接链接 NDK 系统库 |
| 纯 Java/Kotlin 的库（Jetpack、OkHttp 等） | ❌ 不适用 | 那是 Android 框架层，不走 NDK 交叉编译 |
| 依赖特定桌面服务（X11/Wayland/ALSA 等）的库 | ⚠️ 需适配 | 要用 Android 等价 NDK API（EGL/AAudio/MediaCodec…）替换 |

---

## 6. 结论

1. **Linux 的第三方 C/C++ 库可以交叉编译进 Android**，方式与普通 ARM 交叉编译一致：用 NDK 的 clang 工具链（`aarch64-linux-android<API>-clang`）替代 gcc，再用 autotools/CMake 产出静态/动态库链接进你的 native 模块。
2. **libevent 已实测可行**：`android-libevent-demo` 用 NDK 交叉编译出 arm64-v8a libevent 静态库，并编译运行了基于 `event_base` 的定时器 demo，GitHub Actions 双 job 均通过。
3. **主要坑在 Android 的 C 库差异**（pthread 并入 bionic、无 `-lpthread`）与构建系统的细节（configure 来源、CMake 源目录、Threads 检测），按第 3 节处理即可平稳移植。
