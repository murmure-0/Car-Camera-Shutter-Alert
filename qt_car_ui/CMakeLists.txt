# 最低 CMake 版本要求
cmake_minimum_required(VERSION 3.16)

# 工程名与版本、使用的语言
project(car_ui VERSION 0.1 LANGUAGES CXX)

# Qt 的自动处理选项：自动 UIC/MOC/RCC
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# C++ 标准设置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 从环境变量读取 SDK 路径（用于交叉编译）
set(SDK_PATH "$ENV{LUCKFOX_SDK_PATH}")
if(NOT SDK_PATH)
    message(WARNING "Please Set Luckfox-pico SDK Path. Such as:export LUCKFOX_SDK_PATH=/home/user/luckfox-pico")
    return()
endif()
# 指定交叉编译工具链
set(CMAKE_C_COMPILER "${SDK_PATH}/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-gcc")
set(CMAKE_CXX_COMPILER "${SDK_PATH}/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++")


# 链接器库搜索目录：本工程 lib 目录
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/lib")
    # 也支持 include/lib 目录，并设置运行时 rpath
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/lib)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,${CMAKE_CURRENT_SOURCE_DIR}/include/lib")
endif()
set(CMAKE_INSTALL_RPATH "$ORIGIN/lib") # 安装后运行时 rpath
add_definitions(-DRV1106_1103) # 平台宏：启用 RV1106/1103 相关代码
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/include/3rdparty 3rdparty.out)


# 查找 Qt5 Widgets
find_package(Qt5 REQUIRED COMPONENTS Widgets Network)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/lib/cmake/opencv4")
    set(OpenCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/lib/cmake/opencv4")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lib/cmake/opencv4")
    set(OpenCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/cmake/opencv4")
endif()
find_package(OpenCV REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED) # 线程库
add_compile_options(-g -Wall -DISP_HW_V30 -DRKPLATFORM=ON -DARCH64=OFF -DROCKIVA -DUAPI2 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64) # 常用编译宏与调试
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath-link,${CMAKE_CURRENT_SOURCE_DIR}/lib:/usr/lib") # 交叉链接依赖查找

set(SDK_LIB_DIRS "")

# 源文件清单（UI 与业务逻辑）
set(PROJECT_SOURCES
        main.cpp
        camerahud.cpp
        camerahud.h
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        baidu_gesture_client.h
        baidu_gesture_client.cpp
        pages/hudwidgets.h
        pages/dashboardpage.h
        pages/dashboardpage.cpp
        pages/gpspage.h
        pages/gpspage.cpp
        pages/environmentpage.h
        pages/environmentpage.cpp
        pages/camerapage.h
        pages/camerapage.cpp
        pages/motionpage.h
        pages/motionpage.cpp
        pages/connectivitypage.h
        pages/connectivitypage.cpp
        pages/settingspage.h
        pages/settingspage.cpp
        camera/luckfox_mpi.cc 
        camera/camera_service.h
        luckfox_pico_yolov5/include/fatigue_runner.h
        luckfox_pico_yolov5/src/fatigue_runner.cpp
        luckfox_pico_yolov5/include/handyolo_runner.h
        luckfox_pico_yolov5/src/handyolo_runner.cpp
        luckfox_pico_retinaface_facenet/src/retinaface.cc
)
# 头文件搜索路径（BEFORE 确保本地包优先）
include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/include/opencv4
    ${CMAKE_CURRENT_SOURCE_DIR}/include/include/rkaiq
    ${CMAKE_CURRENT_SOURCE_DIR}/include/include/rkaiq/uAPI2
    ${CMAKE_CURRENT_SOURCE_DIR}/include/include/rkaiq/common
    ${CMAKE_CURRENT_SOURCE_DIR}/include/include/rkaiq/xcore
    ${CMAKE_CURRENT_SOURCE_DIR}/include/include/rkaiq/algos
    ${CMAKE_CURRENT_SOURCE_DIR}/include/include/rkaiq/iq_parser
    ${CMAKE_CURRENT_SOURCE_DIR}/include/include/rkaiq/iq_parser_v2
    ${CMAKE_CURRENT_SOURCE_DIR}/include/include/rkaiq/smartIr
    ${CMAKE_CURRENT_SOURCE_DIR}/include/common
    ${CMAKE_CURRENT_SOURCE_DIR}/include/common/isp3.x
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/luckfox_pico_yolov5/include
    ${CMAKE_CURRENT_SOURCE_DIR}/luckfox_pico_retinaface_facenet/include
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/rknpu2/include
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/common/isp3.x
)

# 生成可执行文件目标
add_executable(car_ui
    ${PROJECT_SOURCES}
)

# 基础链接库：Qt/线程/数学/动态加载
target_link_libraries(car_ui PRIVATE
    ${LIBRKNNRT}
    Qt5::Widgets
    Qt5::Network
    ${OpenCV_LIBS}
    Threads::Threads
    pthread
    m
    dl
)

# rkaiq：ISP/AIQ 核心库
target_link_libraries(car_ui PRIVATE rkaiq)

# rockchip_mpp：硬件编解码与缓冲管理
target_link_libraries(car_ui PRIVATE rockchip_mpp)

# rockiva：AI/视觉算法库
target_link_libraries(car_ui PRIVATE rockiva)

# sample_comm：ISP/VI 示例封装依赖
target_link_libraries(car_ui PRIVATE sample_comm)
target_compile_definitions(car_ui PRIVATE HAVE_SAMPLE_COMM)

# rga：图像加速与缩放
target_link_libraries(car_ui PRIVATE rga)

# rtsp：推流协议库
target_link_libraries(car_ui PRIVATE rtsp)

# rockit：多媒体框架库
target_link_libraries(car_ui PRIVATE rockit)

# 目标属性：Bundle/版本信息等
set_target_properties(car_ui PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.car_ui
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# 安装目录设置与安装规则
include(GNUInstallDirs)
install(TARGETS car_ui
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
