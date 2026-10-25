#include "multi_api_test.h"

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
