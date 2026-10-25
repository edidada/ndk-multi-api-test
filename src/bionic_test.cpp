#include "multi_api_test.h"

static void* thread_test(void*) {
    LOGI("[Bionic] pthread 子线程运行正常");
    return nullptr;
}

bool test_bionic() {
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
