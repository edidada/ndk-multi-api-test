#include "multi_api_test.h"

bool test_native_window() {
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
