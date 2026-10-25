# Mars（腾讯/微信开源）Android 相关代码依赖分析

> 对象仓库：Tencent/mars（WeChat 团队开源的跨平台网络组件，C++ 为主，Android 通过 JNI 封装）。
> 数据来源：直接读取仓库 master 分支的 Gradle 文件与 mars/CMakeLists.txt（2026-10-25 抓取）。

## 1. Android 相关代码在仓库中的位置

```
mars/                              # Android Gradle 工程（库工程）
  settings.gradle                # 仅 include 两个库模块
  build.gradle                   # 根工程：AGP classpath、发布插件
  CMakeLists.txt                 # 原生（C/C++）构建：产出 libmarsxlog.so / libmarsstn.so
  libraries/
    mars_android_sdk/            # Android 库模块（JNI 封装，产出 mars-core AAR）
    mars_xlog_sdk/               # Android 库模块（xlog，产出 xlog AAR）
  app/                           # 原生 Demo（CMake + JNI + src），非 Gradle app module
  boost/  openssl/  zstd/  rapidjson/  googlemock/  googletest/  # 第三方/测试（C++）
  comm/ stn/ sdt/ xlog/ baseevent/ boot/ app/                    # Mars 自有 C++ 模块

samples/android/                   # 示例 App
  marsSampleChat/                # 聊天示例（含 app 模块，依赖 protobuf / support 库）
  xlogSample/                    # xlog 示例
```

要点：Android 的 Java 层几乎不依赖外部 AAR，真正的逻辑全在原生 C++（stn 网络、xlog 日志、sdt 等），通过 JNI 暴露给 Java。

---

## 2. Gradle 构建期依赖（插件 / classpath）

### 2.1 SDK 库模块（mars/build.gradle）
```gradle
buildscript {
    repositories { mavenLocal(); google(); mavenCentral() }
    dependencies {
        classpath 'com.android.tools.build:gradle:4.1.0'        // Android Gradle Plugin
        classpath 'digital.wup:android-maven-publish:3.6.2'     // 发布用
    }
}
```
- AGP：**4.1.0**
- 发布：digital.wup:android-maven-publish:3.6.2
- 仓库：google()、mavenCentral()、mavenLocal()
- ext：GROUP=com.tencent.mm.network，VERSION_NAME=0.2.0-SNAPSHOT

### 2.2 示例 App 的 buildscript（samples/android/marsSampleChat/build.gradle）
```gradle
classpath 'com.android.tools.build:gradle:3.4.1'
classpath "com.github.dcendents:android-maven-gradle-plugin:2.1"
classpath 'com.google.protobuf:protobuf-gradle-plugin:0.8.8'
classpath 'com.jfrog.bintray.gradle:gradle-bintray-plugin:1.+'
classpath "org.jfrog.buildinfo:build-info-extractor-gradle:3.1.1"
```
- 仓库额外含 jcenter()

---

## 3. Android 平台 / 编译版本

| 工程 | compileSdk | buildTools | minSdk | targetSdk | Java |
|---|---|---|---|---|---|
| mars_android_sdk / mars_xlog_sdk | 31 | 30.0.2 | 19 | 29 | 1.7 |
| marsSampleChat (app) | 26 | 28.0.3 | 19 | 26 | 1.7 |
| marsSampleChat flavor | - | - | 19 | 23 | - |

> 库模块 build.gradle 中 ndk { moduleName "mmnet"; abiFilter "armeabi"; abiFilter "armeabi-v7a" }，但底层 CMake 实际构建的 so 名为 marsxlog / marsstn（见 5），abiFilters 仅为遗留配置。

---

## 4. Java 层依赖（仅示例 App 显式声明）

mars_android_sdk / mars_xlog_sdk 两个库模块**没有 dependencies {} 块**，只 apply com.android.library + 发布脚本，原生代码走 jni sourceSet（ndk-build/CMake）。

示例 App samples/android/marsSampleChat/app/build.gradle 的依赖：
```gradle
dependencies {
    implementation fileTree(include: ['*.jar'], dir: 'libs')
    implementation 'com.android.support:appcompat-v7:26.1.0'
    implementation 'com.android.support:cardview-v7:26.1.0'
    implementation 'com.android.support:design:26.1.0'
    implementation 'com.android.support:support-v4:26.1.0'
    implementation 'com.google.protobuf:protobuf-lite:3.0.0'
    implementation 'com.google.code.gson:gson:2.8.5'
    implementation 'com.tencent.mars:mars-core:VERSION_NAME'   // Mars 发布产物（占位）
    implementation 'com.tencent.mars:mars-wrapper'             // 或 project(':wrapper')
    androidTestImplementation 'com.android.support.test:runner:0.5'
    androidTestImplementation 'com.android.support.test:rules:0.5'
}
```
- 启用 DataBinding 与 protobuf（protoc 3.0.0 + javalite 插件），说明示例协议用 protobuf。
- 发布产物坐标：com.tencent.mars:mars-core（含原生 so 的 AAR）、com.tencent.mars:mars-wrapper（Java 封装）。

---

## 5. 原生（C/C++）依赖 —— Android .so 实际链接了什么

来自 mars/CMakeLists.txt 的 if(ANDROID) 分支，构建两个共享库：

### 5.1 libmarsxlog.so
```cmake
target_link_libraries(marsxlog
    xlog            # Mars 自有
    mars-boost      # 仓库内置 Boost
    comm            # Mars 自有
    libzstd_static  # 仓库内置 Zstandard
    ${log-lib}      # Android NDK: liblog
    ${z-lib}        # Android NDK: libz (zlib)
)
```

### 5.2 libmarsstn.so
```cmake
find_library(CRYPT_LIB crypto PATHS openssl/openssl_lib_android/ANDROID_ABI ...)
find_library(SSL_LIB   ssl   PATHS openssl/openssl_lib_android/ANDROID_ABI ...)
target_link_libraries(marsstn
    ${log-lib}       # liblog
    stn sdt app baseevent comm boot   # Mars 自有模块
    mars-boost       # 内置 Boost
    marsxlog
    libzstd_static   # 内置 Zstandard
    ${SSL_LIB} ${CRYPT_LIB}            # 内置 OpenSSL: libssl / libcrypto
)
```

### 5.3 第三方原生库（仓库内置，随源码编译/预编译）

| 库 | 仓库路径 | 在 Android 中的形态 | 用途 |
|---|---|---|---|
| Boost | mars/boost | 编译为 mars-boost | 基础工具/asio 等 |
| OpenSSL | mars/openssl | 按 ABI 预编译于 openssl/openssl_lib_android/ABI（libssl.so / libcrypto.so） | TLS / 加密 |
| Zstandard | mars/zstd | 静态库 libzstd_static | 压缩 |
| RapidJSON | mars/rapidjson | 头文件库 | JSON 解析 |
| Google Test / Mock | mars/googletest, mars/googlemock | 仅测试 | 单元测试 |

### 5.4 Android NDK 系统库
- liblog（android/log.h，日志，必备）
- libz（zlib，压缩）
- libc++（C++ 运行时，C++ 代码默认依赖）
- 以及 libc / libm / libdl（默认链接）

> Mars 的 CMake 通过 find_library(log-lib log) / find_library(z-lib z) 取系统库，其余第三方库为仓库内置源码/预编译，因此 Android 端不需要在 Gradle 里额外声明 NDK 库依赖（与本项目 ndk-multi-api-test 显式 find_library(aaudio/android/binder_ndk/log) 的写法不同）。

---

## 6. 结论：Mars Android 依赖总览

| 类别 | 依赖 |
|---|---|
| Gradle 插件 | AGP 4.1.0（库）/ 3.4.1（示例）、android-maven-publish 3.6.2、protobuf-gradle-plugin 0.8.8、android-maven-gradle-plugin 2.1、bintray 插件 |
| Java/AAR（仅示例） | com.android.support:appcompat-v7/cardview-v7/design/support-v4:26.1.0、protobuf-lite:3.0.0、gson:2.8.5、com.tencent.mars:mars-core、com.tencent.mars:mars-wrapper、support-test:0.5（androidTest） |
| 原生第三方（内置） | Boost(mars-boost)、OpenSSL(libssl/libcrypto)、Zstandard(libzstd_static)、RapidJSON、GoogleTest/Mock(测试) |
| NDK 系统库 | liblog、libz、libc++（及 libc/libm/libdl） |
| Mars 自有模块 | comm、boot、app、baseevent、xlog、sdt、stn（编译进 libmarsxlog.so / libmarsstn.so） |

构建入口：mars/build_android.py（封装 Gradle + CMake 交叉编译）。产物为 libmarsxlog.so 与 libmarsstn.so，由 mars_android_sdk / mars_xlog_sdk 打包为 AAR 发布。
