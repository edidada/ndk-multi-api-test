cmake_minimum_required(VERSION 3.18)
project(ndk_multi_api_test)

find_library(AAUDIO_LIB aaudio)
find_library(ANDROID_LIB android)
find_library(BINDER_NDK_LIB binder_ndk)
find_library(LOG_LIB log)

# 按功能拆分的测试模块，编译为静态库
add_library(multi_api_test STATIC
    src/bionic_test.cpp
    src/aaudio_test.cpp
    src/hardware_buffer_test.cpp
    src/native_window_test.cpp
    src/binder_test.cpp
)
target_include_directories(multi_api_test PUBLIC include)

# 入口：仅包含 main()，链接上述库与各 NDK 库
add_executable(ndk_multi_api_test main.cpp)
target_link_libraries(ndk_multi_api_test
    multi_api_test
    ${AAUDIO_LIB}
    ${ANDROID_LIB}
    ${BINDER_NDK_LIB}
    ${LOG_LIB}
    # 故意不链接 pthread / rt，验证 Bionic 已整合
)
