#-------------------------------------------------
#
# Project created by QtCreator 2026-01-29T11:04:51
#
#-------------------------------------------------

QT       += core gui network serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qt_car_ui
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += RV1106_1103 ISP_HW_V30 RKPLATFORM=ON ARCH64=OFF ROCKIVA UAPI2 _LARGEFILE_SOURCE _LARGEFILE64_SOURCE _FILE_OFFSET_BITS=64 HAVE_SAMPLE_COMM

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++17
QMAKE_CXXFLAGS += -g -Wall

INCLUDEPATH += \
        include \
        include/include \
        include/include/opencv4 \
        include/3rdparty/rknpu2/include \
        include/include/rkaiq \
        include/include/rkaiq/uAPI2 \
        include/include/rkaiq/common \
        include/include/rkaiq/xcore \
        include/include/rkaiq/algos \
        include/include/rkaiq/iq_parser \
        include/include/rkaiq/iq_parser_v2 \
        include/include/rkaiq/smartIr \
        include/common \
        include/common/isp3.x \
        luckfox_pico_yolov5/include \
        luckfox_pico_retinaface_facenet/include \
        . \
        $${OpenCV_INCLUDE_DIRS} \
        3rdparty/rknpu2/include \
        common \
        common/isp3.x

LIBS += -L$$PWD/lib \
        -L$$PWD/include/lib \
        -L$$PWD/include/3rdparty/rknpu2/Linux/armhf-uclibc \
        $$PWD/include/3rdparty/rknpu2/Linux/armhf-uclibc/librknnmrt.so \
        -lrkaiq \
        -lrockchip_mpp \
        -lrockiva \
        -lsample_comm \
        -lrga \
        -lrtsp \
        -lrockit \
        -lopencv_imgproc \
        -lopencv_photo \
        -lopencv_video \
        -lopencv_features2d \
        -lopencv_highgui \
        -lopencv_core \
        -lpthread \
        -lm \
        -ldl \
        -lrt

SOURCES += \
        main.cpp \
        mainwindow.cpp \
        baidu_gesture_client.cpp \
        pages/camerapage.cpp \
        pages/connectivitypage.cpp \
        pages/dashboardpage.cpp \
        pages/environmentpage.cpp \
        pages/gpspage.cpp \
        pages/motionpage.cpp \
        pages/settingspage.cpp \
        camera/luckfox_mpi.cc \
        luckfox_pico_yolov5/src/yolov5_runner.cpp \
        luckfox_pico_yolov5/src/fatigue_runner.cpp \
        luckfox_pico_yolov5/src/handyolo_runner.cpp \
        luckfox_pico_retinaface_facenet/src/retinaface.cc

HEADERS += \
        mainwindow.h \
        baidu_gesture_client.h \
        pages/camerapage.h \
        pages/connectivitypage.h \
        pages/dashboardpage.h \
        pages/environmentpage.h \
        pages/gpspage.h \
        pages/hudwidgets.h \
        pages/motionpage.h \
        pages/settingspage.h \
        camera/camera_service.h \
        luckfox_pico_yolov5/include/yolov5_runner.h \
        luckfox_pico_yolov5/include/fatigue_runner.h \
        luckfox_pico_yolov5/include/handyolo_runner.h

FORMS += \
        mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
