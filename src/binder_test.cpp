#include "multi_api_test.h"

bool test_binder() {
    LOGI("========== Binder (libbinder_ndk) 测试 ==========");

    // 验证关键 Binder 函数可链接（libbinder_ndk 公开 API）
    auto* incStrong_fn     = &AIBinder_incStrong;
    auto* decStrong_fn     = &AIBinder_decStrong;
    auto* getCallingPid_fn = &AIBinder_getCallingPid;

    LOGI("[Binder] AIBinder 符号链接成功: incStrong=%p decStrong=%p getCallingPid=%p",
         (void*)incStrong_fn, (void*)decStrong_fn, (void*)getCallingPid_fn);

    // 说明：NDK 的 libbinder_ndk 仅暴露 AIBinder 等底层公开 API；
    // AServiceManager_getService / addService 并非公开 NDK 符号（仅出现在头文件注释示例中），
    // 且普通进程无法与 servicemanager 交互（需要系统权限或特定 context）。
    // 因此此处验证 binder NDK 库可正确链接、关键符号可用，即证明 NDK 环境配置正确。
    LOGI("[Binder] 注意：AServiceManager 非公开 NDK 符号，addService 还需系统级权限");
    return true;
}
