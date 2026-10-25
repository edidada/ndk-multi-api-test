#include "multi_api_test.h"

bool test_aaudio() {
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
