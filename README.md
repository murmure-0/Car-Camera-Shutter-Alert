# 🚗 Car Camera Shutter Alert

> 基于瑞芯微 RV1106 + STM32L4 的智能车载安全驾驶监控系统 — 集疲劳检测、手势识别、多传感器融合与 HUD 显示于一体

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform: RV1106](https://img.shields.io/badge/Platform-RV1106-blue.svg)]()
[![MCU: STM32L4](https://img.shields.io/badge/MCU-STM32L4-green.svg)]()
[![Framework: Qt5](https://img.shields.io/badge/Framework-Qt5-green.svg)]()

---

## 📖 项目简介

Car Camera Shutter Alert 是一套面向嵌入式车载场景的智能安全驾驶辅助系统，运行在瑞芯微 RV1106 嵌入式平台与 STM32L431 微控制器之上。系统通过摄像头实时采集驾驶员面部图像，利用 NPU 加速的深度学习模型进行人脸检测与关键点提取，计算 EAR（眼睛纵横比）、MAR（嘴巴纵横比）、PERCLOS 等疲劳指标，实现实时疲劳驾驶预警。同时集成百度手势识别 API，支持通过手势控制音乐播放，提升驾驶安全性与便利性。

### ✨ 核心特性

- **🧠 实时疲劳检测** — 基于 RetinaFace + PFLD 的多指标疲劳判定（EAR/MAR/PERCLOS/连续闭眼/连续张嘴）
- **🤚 手势识别控制** — 百度云端手势识别 API，支持握拳暂停、张手播放、食指上一首、数字二下一首
- **📹 视频录制** — 摄像头实时预览与录制状态管理
- **🗺️ GPS 定位** — GPS 模块 NMEA 解析 + 百度静态地图展示
- **🌡️ 环境监测** — 温湿度（AHT30）、加速度/陀螺仪（MPU6050）、电压/电流（INA226）、电池/霍尔传感器
- **🎵 音乐播放** — 本地音乐播放器，支持手势控制
- **📊 HUD 仪表盘** — 800×480 暗色主题 UI，人工地平仪、折线图、地图背景等 HUD 组件
- **🔌 双 MCU 串口通信** — STM32 与 RV1106 通过 UART JSON 协议实时交互传感器数据与控制指令
- **⚙️ 步进电机控制** — 支持摄像头遮挡盖板自动开合

---

## 🏗️ 系统架构

```
┌─────────────────────────────────────────────────────────────────┐
│                    RV1106 主控板 (Linux)                         │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │              Qt5 车机 UI (qt_car_ui)                      │   │
│  │  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ │   │
│  │  │仪表盘│ │ GPS  │ │环境  │ │摄像头│ │运动  │ │设置  │ │   │
│  │  └──────┘ └──────┘ └──────┘ └──┬───┘ └──────┘ └──────┘ │   │
│  └──────────────────────────────┼───────────────────────────┘   │
│                                 │                                │
│  ┌──────────────────────────────┼───────────────────────────┐   │
│  │        AI 推理引擎           │                           │   │
│  │  ┌─────────────┐  ┌─────────┴────────┐                  │   │
│  │  │ RetinaFace  │  │ PFLD 关键点检测  │                  │   │
│  │  │ 人脸检测    │  │ 106点关键点      │                  │   │
│  │  └──────┬──────┘  └────────┬─────────┘                  │   │
│  │         └────────┬─────────┘                             │   │
│  │                  ▼                                        │   │
│  │         ┌─────────────────┐                              │   │
│  │         │  疲劳指标计算    │                              │   │
│  │         │  EAR/MAR/PERCLOS│                              │   │
│  │         └─────────────────┘                              │   │
│  └──────────────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  百度手势识别 API (BaiduGestureClient)                    │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                 │
│  UART (ttyS0/ttyS1, 115200bps) ◄──── JSON 协议 ────►          │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                  STM32L431 传感器控制板 (FreeRTOS)               │
│  ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐       │
│  │MPU6050 │ │ AHT30  │ │ INA226 │ │  GPS   │ │步进电机│       │
│  │加速度  │ │温湿度  │ │电压电流│ │定位    │ │遮挡盖  │       │
│  └────────┘ └────────┘ └────────┘ └────────┘ └────────┘       │
│  ┌────────┐ ┌────────┐ ┌────────┐                              │
│  │ADC采集 │ │按键    │ │LED指示 │                              │
│  │电池/霍尔│ │开关机  │ │状态    │                              │
│  └────────┘ └────────┘ └────────┘                              │
└─────────────────────────────────────────────────────────────────┘
```

---

## 📁 项目结构

```
Car-Camera-Shutter-Alert/
├── qt_car_ui/                      # RV1106 端 Qt5 车机 UI 应用
│   ├── main.cpp                    # 应用入口
│   ├── mainwindow.h/cpp/ui         # 主窗口（页面路由、缩放适配、全局状态）
│   ├── baidu_gesture_client.h/cpp  # 百度手势识别 API 客户端
│   ├── camera/
│   │   ├── camera_service.h        # 摄像头硬件服务（MPI/ISP）
│   │   └── luckfox_mpi.cc          # Luckfox Pico MPI 封装
│   ├── pages/
│   │   ├── camerapage.h/cpp        # 摄像头页面（预览、疲劳检测、手势识别）
│   │   ├── dashboardpage.h/cpp     # 仪表盘页面（地图、音乐、系统监控）
│   │   ├── gpspage.h/cpp           # GPS 定位页面
│   │   ├── environmentpage.h/cpp   # 环境监测页面
│   │   ├── motionpage.h/cpp        # 运动姿态页面
│   │   ├── connectivitypage.h/cpp  # 网络连接页面
│   │   ├── settingspage.h/cpp      # 系统设置页面
│   │   ├── hudwidgets.h            # HUD 自定义控件（地平仪、折线图、预览等）
│   │   └── camerahud.h/cpp         # 摄像头 HUD 叠加层
│   ├── luckfox_pico_yolov5/
│   │   ├── include/
│   │   │   ├── yolov5.h            # YOLOv5 RKNN 模型上下文
│   │   │   ├── postprocess.h       # YOLOv5 后处理
│   │   │   ├── fatigue_runner.h    # 疲劳检测推理引擎
│   │   │   └── handyolo_runner.h   # 手部 YOLO 检测器
│   │   └── src/
│   │       ├── yolov5_runner.cpp
│   │       ├── fatigue_runner.cpp
│   │       └── handyolo_runner.cpp
│   ├── include/                    # 第三方库与平台头文件
│   │   ├── 3rdparty/               # RKNPU2、librga、DMA/DRM 分配器
│   │   ├── common/                 # ISP、sample_comm 公共代码
│   │   └── include/                # Rockchip MPI、RGA、RKAIQ 等平台头文件
│   ├── docs/
│   │   ├── camera_page_flow.md     # 摄像头页面运行流程分析
│   │   └── thread_resource_analysis.md  # 线程资源管理分析
│   └── qt_car_ui.pro               # Qt 项目文件
│
├── rv1106_sensor/                  # STM32L431 传感器控制板固件
│   ├── Core/
│   │   ├── Inc/                    # 头文件
│   │   │   ├── main.h
│   │   │   ├── bsp_power.h         # 电源管理
│   │   │   ├── mpu6050.h           # MPU6050 IMU 驱动
│   │   │   ├── aht30.h             # AHT30 温湿度驱动
│   │   │   ├── ina226.h            # INA226 功率监测驱动
│   │   │   ├── gps_module.h        # GPS 模块驱动
│   │   │   ├── stepper.h           # 步进电机驱动
│   │   │   ├── soft_i2c.h          # 软件 I2C 驱动
│   │   │   ├── key_handler.h       # 按键处理
│   │   │   ├── task_queue.h        # FreeRTOS 消息队列
│   │   │   ├── business_handler.h  # 业务逻辑处理
│   │   │   ├── protocol.h          # 串口通信协议
│   │   │   └── json_parser.h       # JSON 解析器
│   │   └── Src/                    # 源文件
│   ├── Drivers/                    # STM32 HAL/CMSIS 库
│   ├── MDK-ARM/                    # Keil MDK 工程文件
│   ├── README.md                   # STM32 子项目说明
│   └── PROTOCOL.md                 # STM32-RV1106 通信协议文档
│
├── 3d_model_assets/                # SolidWorks 3D 外壳模型
├── PCB/                            # PCB 工程文件
├── LICENSE                         # MIT 许可证
├── CONTRIBUTING.md                 # 贡献指南
└── README.md                       # 本文件
```

---

## 🚀 快速开始

### 前置条件

#### RV1106 端 (Qt5 UI)

- **硬件**: Luckfox Pico RV1106 开发板
- **交叉编译工具链**: arm-linux-gnueabihf-gcc
- **Qt5**: 交叉编译的 Qt5 for ARM (含 network、serialport 模块)
- **RKNN SDK**: rknpu2 运行时库 (armhf-uclibc)
- **Rockchip MPI**: librockit、librockchip_mpp、librga、librkaiq
- **OpenCV**: 交叉编译的 OpenCV for ARM
- **百度智能云 API Key**: 用于手势识别服务

#### STM32 端 (传感器固件)

- **硬件**: STM32L431RCT6 最小系统板
- **IDE**: Keil MDK-ARM v5
- **SDK**: STM32L4xx HAL Driver + CMSIS
- **RTOS**: FreeRTOS

### 编译与部署

#### 1. Qt5 车机 UI 编译

```bash
# 克隆仓库
git clone https://github.com/<your-username>/Car-Camera-Shutter-Alert.git
cd Car-Camera-Shutter-Alert/qt_car_ui

# 配置百度 API 凭证（环境变量方式）
export BAIDU_API_KEY="your_api_key"
export BAIDU_SECRET_KEY="your_secret_key"

# 如需使用自定义代理服务器（可选，默认直连百度云）
# export BAIDU_PROXY_BASE="http://your-proxy-server:8081"

# 使用交叉编译工具链构建
mkdir build && cd build
<qt-install-path>/bin/qmake ../qt_car_ui.pro -spec linux-arm-gnueabi-g++
make -j$(nproc)

# 部署到 RV1106 开发板
scp qt_car_ui root@<device-ip>:/opt/qt_car_ui/bin/
```

#### 2. STM32 固件编译

1. 使用 Keil MDK-ARM 打开 `rv1106_sensor/MDK-ARM/rv1106_sensor.uvprojx`
2. 选择目标芯片 STM32L431RCTx
3. 编译并下载到开发板

### 运行

在 RV1106 开发板上：

```bash
# 确保 RKNN 模型文件已部署到 ./model/ 目录
ls ./model/
# RetinaFace640.rknn  pfld_106.rknn

# 运行车机 UI
./qt_car_ui
```

---

## 🔧 功能详解

### 疲劳检测

系统采用多指标融合的疲劳判定策略：

| 指标 | 含义 | 阈值 | 判定条件 |
|------|------|------|---------|
| **EAR** | 眼睛纵横比 | < 0.22 | 单眼闭合 |
| **MAR** | 嘴巴纵横比 | > 0.60 | 打哈欠 |
| **PERCLOS** | 滑动窗口闭眼占比 | > 0.50 | 持续疲劳（需≥100帧数据） |
| **连续闭眼帧** | 持续闭眼帧计数 | ≥ 15 帧 | 瞌睡 |
| **连续张嘴帧** | 持续张嘴帧计数 | ≥ 20 帧 | 打哈欠 |

任一指标触发即判定为疲劳状态，系统将：
- 在摄像头预览界面显示红色警告框和疲劳指标
- 通过串口发送疲劳状态到 STM32
- 在仪表盘页面触发疲劳警告音

### 手势识别

通过百度云端手势识别 API 实现，支持以下手势映射：

| 手势 | 动作 | 说明 |
|------|------|------|
| ✊ Fist (握拳) | ⏸ 暂停 | 暂停音乐播放 |
| 🖐 Five (张手) | ▶ 播放 | 恢复音乐播放 |
| ☝️ One (食指) | ⏮ 上一首 | 切换到上一首歌曲 |
| ✌️ Two (数字二) | ⏭ 下一首 | 切换到下一首歌曲 |

手势识别结果同时通过串口发送到 STM32，JSON 格式：
```json
{"type":"gesture","data":{"gesture":"Fist","action":"pause"}}
```

### 串口通信协议

STM32 与 RV1106 之间通过 UART (115200bps, 8N1) 进行 JSON 文本行通信：

- **STM32 → RV1106**: 周期性上报传感器数据（温度、湿度、加速度、陀螺仪、电压、电流、GPS、电池、霍尔、电机角度）
- **RV1106 → STM32**: 发送疲劳状态、网络状态、手势识别结果

详细协议规范见 [rv1106_sensor/PROTOCOL.md](rv1106_sensor/PROTOCOL.md)

---

## 🖼️ 界面展示

系统采用 800×480 暗色主题 UI，包含 7 个功能页面：

| 页面 | 导航键 | 功能 |
|------|--------|------|
| 仪表盘 | H | 百度地图背景、音乐播放器、CPU/内存/温度监控 |
| GPS | G | GPS 坐标显示、卫星数量、百度静态地图 |
| 环境 | E | 温湿度、电池电压、功率监测 |
| 摄像头 | C | 实时预览、人脸检测框、疲劳指标叠加、手势识别 |
| 运动 | M | 人工地平仪、加速度/陀螺仪折线图、倾覆警告 |
| 网络 | N | 网络连接状态、IP 地址 |
| 设置 | S | 暗色/亮色主题切换、音量调节 |

---

## 🛠️ 技术栈

| 组件 | 技术 |
|------|------|
| 主控芯片 | 瑞芯微 RV1106 (ARM Cortex-A7 + NPU) |
| 传感器 MCU | STM32L431RCT6 (ARM Cortex-M4) |
| UI 框架 | Qt 5 (C++17) |
| AI 推理 | RKNPU2 (RKNN 零拷贝 API) |
| 人脸检测 | RetinaFace640 (RKNN 量化模型) |
| 关键点检测 | PFLD 106点 (RKNN 量化模型) |
| 手势识别 | 百度智能云手势识别 API |
| 地图服务 | 百度地图静态图 API |
| RTOS | FreeRTOS |
| 3D 建模 | SolidWorks |
| PCB 设计 | 立创 EDA |

---

## 📚 文档

- [摄像头页面运行流程分析](qt_car_ui/docs/camera_page_flow.md) — 详细的数据流、线程架构与生命周期
- [线程资源管理分析](qt_car_ui/docs/thread_resource_analysis.md) — 线程阻塞点排查与优化建议
- [STM32 模块说明](rv1106_sensor/项目模块说明.md) — 传感器驱动与 FreeRTOS 任务架构
- [通信协议文档](rv1106_sensor/PROTOCOL.md) — STM32-RV1106 JSON 串口协议
- [GPS 串口问题排查](rv1106_sensor/gps_uart_issue_record.md) — FreeRTOS 堆栈溢出排查案例

---

## 🤝 贡献指南

欢迎对本项目做出贡献！请阅读 [CONTRIBUTING.md](CONTRIBUTING.md) 了解详情。

### 快速贡献流程

1. Fork 本仓库
2. 创建功能分支 (`git checkout -b feature/amazing-feature`)
3. 提交更改 (`git commit -m 'Add some amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 创建 Pull Request

---

## 📄 许可证

本项目基于 [MIT License](LICENSE) 开源。

### 第三方库许可证

| 库 | 许可证 |
|----|--------|
| Qt5 | LGPL v3 |
| RKNPU2 | Apache 2.0 |
| OpenCV | Apache 2.0 |
| STM32 HAL | BSD-3-Clause |
| FreeRTOS | MIT |

---

## 🙏 致谢

- [瑞芯微电子](https://www.rock-chips.com/) — RV1106 芯片与 RKNN SDK
- [百度智能云](https://cloud.baidu.com/) — 手势识别与地图 API
- [Luckfox Pico](https://www.luckfox.com/) — RV1106 开发板
- [STMicroelectronics](https://www.st.com/) — STM32 HAL 库
- [FreeRTOS](https://www.freertos.org/) — 实时操作系统
