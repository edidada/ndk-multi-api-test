cmake_minimum_required(VERSION 3.18)
project(ndk_multi_api_test)

find_library(AAUDIO_LIB aaudio)
find_library(ANDROID_LIB android)
find_library(BINDER_NDK_LIB binder_ndk)
find_library(LOG_LIB log)

add_executable(ndk_multi_api_test main.cpp)

target_link_libraries(ndk_multi_api_test
    ${AAUDIO_LIB}
    ${ANDROID_LIB}
    ${BINDER_NDK_LIB}
    ${LOG_LIB}
    # 故意不链接 pthread / rt，验证 Bionic 已整合
)
