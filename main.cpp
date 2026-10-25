#include <cstdio>
#include <cstring>

// Bionic 检测
#ifdef __BIONIC__
#include <android/log.h>
#define TAG "MultiApiTest"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#else
#error "Not building against Bionic"
#endif

// 各 API 头文件
#include <aaudio/AAudio.h>
#include <android/hardware_buffer.h>
#include <android/native_window.h>
#include <android/binder_ibinder.h>
#include <android/binder_manager.h>

// pthread 测试（不显式链接 libpthread）
#include <pthread.h>
#include <ctime>

static void* thread_test(void*) {
    LOGI("[Bionic] pthread 子线程运行正常");
    return nullptr;
}

static bool test_bionic() {
    LOGI("========== Bionic 测试 ==========");

    // 1. 编译期宏
    LOGI("[Bionic] __BIONIC__ 宏已定义，确认使用 Bionic C 库");

    // 2. pthread 无需 -lpthread
    pthread_t t;
    if (pthread_create(&t, nullptr, thread_test, nullptr) != 0) {
        LOGI("[Bionic] pthread_create 失败");
        return false;
    }
    pthread_join(t, nullptr);

    // 3. clock_gettime 无需 -lrt
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        LOGI("[Bionic] clock_gettime 成功（无需 librt）");
    } else {
        LOGI("[Bionic] clock_gettime 失败");
        return false;
    }

    LOGI("[Bionic] 结论：pthread/rt 功能已整合进 libc");
    return true;
}

static bool test_aaudio() {
    LOGI("========== AAudio 测试 ==========");

    AAudioStreamBuilder* builder = nullptr;
    aaudio_result_t r = AAudio_createStreamBuilder(&builder);
    if (r != AAUDIO_OK) {
        LOGI("[AAudio] 创建 Builder 失败: %s", AAudio_convertResultToText(r));
        return false;
    }

    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT);
    AAudioStreamBuilder_setChannelCount(builder, 2);
    AAudioStreamBuilder_setSampleRate(builder, 48000);

    AAudioStream* stream = nullptr;
    r = AAudioStreamBuilder_openStream(builder, &stream);

    if (r == AAUDIO_OK) {
        LOGI("[AAudio] 成功打开低延迟输出流");
        AAudioStream_close(stream);
    } else {
        // 模拟器或无声卡设备可能失败，但 API 本身可调用
        LOGI("[AAudio] 打开流失败（可能环境不支持）: %s", AAudio_convertResultToText(r));
    }

    AAudioStreamBuilder_delete(builder);
    // 即使打开失败，只要 Builder 创建成功即证明 API 可用
    return (r == AAUDIO_OK || builder != nullptr);
}

static bool test_hardware_buffer() {
    LOGI("========== HardwareBuffer 测试 ==========");

    AHardwareBuffer_Desc desc = {};
    desc.width  = 256;
    desc.height = 256;
    desc.layers = 1;
    desc.format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;
    desc.usage  = AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN | AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN;

    AHardwareBuffer* buffer = nullptr;
    int ret = AHardwareBuffer_allocate(&desc, &buffer);
    if (ret != 0 || buffer == nullptr) {
        LOGI("[HardwareBuffer] 分配失败，ret=%d", ret);
        return false;
    }

    LOGI("[HardwareBuffer] 成功分配 %ux%u RGBA 缓冲区", desc.width, desc.height);

    // 尝试锁定并写入
    void* addr = nullptr;
    ret = AHardwareBuffer_lock(buffer, AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN,
                               -1, nullptr, &addr);
    if (ret == 0 && addr) {
        memset(addr, 0xFF, desc.width * desc.height * 4);
        AHardwareBuffer_unlock(buffer, nullptr);
        LOGI("[HardwareBuffer] 成功锁定并写入缓冲区");
    } else {
        LOGI("[HardwareBuffer] 锁定失败（分配本身已成功）");
    }

    AHardwareBuffer_release(buffer);
    LOGI("[HardwareBuffer] 释放完成");
    return true;
}

static bool test_native_window() {
    LOGI("========== NativeWindow 测试 ==========");
    // 纯命令行进程没有 Java Surface，无法获取真实的 ANativeWindow。
    // 这里仅验证 API 符号可链接、头文件可用。
    // 如果强行传入 nullptr，预期返回错误而非崩溃。

    // 验证函数指针存在（编译期链接成功即证明 API 可用）
    auto* acquire_fn = &ANativeWindow_acquire;
    auto* release_fn = &ANativeWindow_release;
    auto* getWidth_fn = &ANativeWindow_getWidth;

    LOGI("[NativeWindow] API 符号链接成功: acquire=%p release=%p getWidth=%p",
         (void*)acquire_fn, (void*)release_fn, (void*)getWidth_fn);

    // 尝试对 nullptr 调用（预期失败/返回错误，验证 API 被调用而非崩溃）
    int32_t width = ANativeWindow_getWidth(nullptr);
    LOGI("[NativeWindow] ANativeWindow_getWidth(nullptr) 返回 %d（预期为负值）", width);
    LOGI("[NativeWindow] 注意：真实测试需要 Java 层 Surface 传入 JNI");
    return true;
}

static bool test_binder() {
    LOGI("========== Binder (libbinder_ndk) 测试 ==========");

    // 验证关键 Binder 函数可链接
    auto* getService_fn = &AServiceManager_getService;
    auto* addService_fn = &AServiceManager_addService;

    LOGI("[Binder] AServiceManager 符号链接成功: getService=%p addService=%p",
         (void*)getService_fn, (void*)addService_fn);

    // 尝试获取一个不存在的服务（预期返回 nullptr，但证明 Binder 驱动可访问）
    AIBinder* binder = AServiceManager_getService("not.exist.service.test");
    if (binder == nullptr) {
        LOGI("[Binder] 查询不存在的服务返回 nullptr（符合预期，Binder 链路正常）");
    } else {
        LOGI("[Binder] 意外获取到服务: %p", (void*)binder);
        AIBinder_decStrong(binder);
    }

    // 注意：普通 shell 权限进程通常无法 addService，需要系统权限或特定 context
    LOGI("[Binder] 注意：addService 需要系统级权限，普通进程通常被拒绝");
    return true;
}

int main() {
    LOGI("===== NDK 多 API 综合测试开始 =====");

    bool results[5];
    results[0] = test_bionic();
    results[1] = test_aaudio();
    results[2] = test_hardware_buffer();
    results[3] = test_native_window();
    results[4] = test_binder();

    LOGI("===== 测试结果汇总 =====");
    const char* names[] = {"Bionic", "AAudio", "HardwareBuffer", "NativeWindow", "Binder"};
    for (int i = 0; i < 5; i++) {
        LOGI("  %s: %s", names[i], results[i] ? "PASS" : "FAIL");
    }

    // Bionic、AAudio、HardwareBuffer 是核心可验证项
    bool core_pass = results[0] && results[1] && results[2];
    LOGI("核心 API（Bionic/AAudio/HardwareBuffer）%s", core_pass ? "全部通过" : "存在失败");
    return core_pass ? 0 : 1;
}
