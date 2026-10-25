#pragma once

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

// pthread 测试（不显式链接 libpthread）
#include <pthread.h>
#include <ctime>

// 各模块测试入口，具体实现位于 src/ 下对应文件
bool test_bionic();
bool test_aaudio();
bool test_hardware_buffer();
bool test_native_window();
bool test_binder();
