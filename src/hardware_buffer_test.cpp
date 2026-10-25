#include "multi_api_test.h"

bool test_hardware_buffer() {
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
