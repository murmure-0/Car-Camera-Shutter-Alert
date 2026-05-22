# 摄像头页面运行流程分析

## 1. 系统启动与初始化

### 1.1 启动入口

`MainWindow` 构造函数中创建 `CameraPage`：

```
MainWindow::MainWindow()
  └─ m_cameraPage = new CameraPage(m_stack)    // mainwindow.cpp:L221
```

### 1.2 CameraPage 构造函数初始化流程

`CameraPage::CameraPage()` (camerapage.cpp:L145) 按以下顺序初始化：

```
CameraPage::CameraPage()
  ├─ 1. qRegisterMetaType<QVector<CameraDetectionOverlay>>()   // 注册跨线程信号类型
  ├─ 2. qRegisterMetaType<QStringList>()
  ├─ 3. 创建 CameraPreviewWidget（预览画面 + 检测框绘制）
  ├─ 4. 创建右侧信息面板（疲劳状态/EAR/MAR/PERCLOS/手势标签）
  ├─ 5. 创建 CameraCaptureWorker + QThread（采集线程）
  ├─ 6. 连接信号槽（帧数据 → UI更新）
  ├─ 7. 启动采集线程 thread->start()
  ├─ 8. 创建 BaiduGestureClient（手势识别客户端）
  ├─ 9. 创建 QTimer（1秒间隔，触发手势识别）
  └─ 10. 启动手势识别定时器 m_cloudTimer->start()
```

### 1.3 采集线程初始化

线程启动后，`CameraCaptureWorker::start()` 执行：

```
CameraCaptureWorker::start()                    // camerapage.cpp:L46
  ├─ m_service.init(640, 480)                   // 初始化摄像头硬件
  │   ├─ SAMPLE_COMM_ISP_Init(0, ...)           // ISP 初始化
  │   ├─ SAMPLE_COMM_ISP_Run(0)                 // ISP 运行
  │   ├─ RK_MPI_SYS_Init()                      // MPI 系统初始化
  │   ├─ vi_dev_init()                          // VI 设备初始化
  │   └─ vi_chn_init(0, 640, 480)              // VI 通道初始化
  │
  ├─ m_runner.init("./model/RetinaFace640.rknn", // 初始化人脸检测模型
  │                 "./model/pfld_106.rknn",      // 初始化关键点检测模型
  │                 112)                          // 关键点模型输入尺寸
  │   ├─ init_retinaface_model()                 // 加载 RetinaFace RKNN 模型
  │   │   ├─ rknn_init()                         // 创建 RKNN 上下文
  │   │   ├─ rknn_query(IN_OUT_NUM)              // 查询输入输出数量
  │   │   ├─ rknn_query(NATIVE_INPUT_ATTR)       // 查询输入属性
  │   │   └─ rknn_query(NATIVE_NHWC_OUTPUT_ATTR) // 查询输出属性
  │   │
  │   └─ m_pfldModel->init(pfld_rknn_path)       // 加载 PFLD 关键点模型
  │       ├─ rknn_init()
  │       ├─ rknn_query(...)
  │       └─ rknn_create_mem()                    // 分配零拷贝内存
  │
  └─ 进入采集循环 while(m_running)
```

---

## 2. 线程架构

### 2.1 线程总览

| 线程 | 类型 | 创建时机 | 主要职责 | 生命周期 |
|------|------|---------|---------|---------|
| **UI 主线程** | Qt GUI 线程 | 程序启动 | 界面渲染、事件处理、手势API调用 | 程序全程 |
| **采集线程** | QThread | CameraPage构造时 | 摄像头抓帧 + 人脸检测 + 疲劳推理 | CameraPage存在期间 |
| **百度API网络线程** | Qt网络内部线程 | 首次HTTP请求时 | 发送图像到百度服务器，接收手势结果 | 异步，请求完成即结束 |

### 2.2 线程关系图

```
┌─────────────────────────────────────────────────────────────┐
│                     UI 主线程 (Main Thread)                   │
│                                                               │
│  ┌──────────────┐  ┌──────────────────┐  ┌───────────────┐  │
│  │CameraPreview │  │BaiduGestureClient│  │  QLabel(s)    │  │
│  │   Widget     │  │  (网络请求)       │  │ 疲劳/手势显示  │  │
│  └──────┬───────┘  └────────┬─────────┘  └───────┬───────┘  │
│         │                   │                     │           │
│         │  frameReady信号   │  inferenceFinished  │  更新文本  │
│         │  (跨线程)         │  信号               │           │
└─────────┼───────────────────┼─────────────────────┼───────────┘
          │                   │                     │
          ▲                   ▲                     ▲
          │                   │                     │
┌─────────┼───────────────────┼─────────────────────┼───────────┐
│         │    采集线程 (Capture QThread)             │           │
│         │                                               │       │
│  ┌──────┴───────┐                                       │       │
│  │CameraCapture │                                       │       │
│  │   Worker     │─── frameReady(img, detections, ──────┘       │
│  │              │               fatigueInfo)                    │
│  │  ┌─────────┐│                                              │
│  │  │Camera   ││  YUV→RGB 抓帧                                │
│  │  │Service  ││                                              │
│  │  └─────────┘│                                              │
│  │  ┌─────────┐│                                              │
│  │  │Fatigue  ││  人脸检测 + 关键点 + 疲劳判定                  │
│  │  │Detector ││                                              │
│  │  │Runner   ││                                              │
│  │  └─────────┘│                                              │
│  └──────────────┘                                              │
└─────────────────────────────────────────────────────────────────┘
```

---

## 3. 主循环数据流

### 3.1 采集线程主循环

```
while (m_running) {                                    // camerapage.cpp:L68
    │
    ├─ 1. 抓取原始帧
    │   m_service.grabFrameRGB(img, 1000)              // camera_service.h:L49
    │   输入: 无
    │   输出: QImage(640x480, RGB888)
    │   数据流: VI通道 → YUV420SP → 软件转RGB888 → QImage
    │
    ├─ 2. 图像预处理
    │   ├─ convertToFormat(Format_RGB888)              // 确保格式一致
    │   ├─ copy(x, y, 480, 480)                       // 中心裁剪 640x480→480x480
    │   └─ transformed(rotate(-90))                   // 逆时针旋转90°
    │   输出: QImage(480x480, RGB888) → cv::Mat(BGR)
    │
    ├─ 3. RetinaFace 人脸检测
    │   inference_retinaface_model(&m_retinafaceCtx,   // retinaface.cc
    │       frame.cols, frame.rows,
    │       rgb_data, step, &retina_result)
    │   输入: cv::Mat(BGR, 480x480)
    │   输出: retinaface_result { count, object[128] }
    │       每个 object: { cls, box{left,top,right,bottom}, score, ponit[5] }
    │
    ├─ 4. PFLD 关键点检测（对每张人脸）
    │   m_pfldModel->run(face_roi)                     // fatigue_runner.cpp
    │   输入: 人脸ROI区域 (cv::Mat)
    │   输出: 106个关键点坐标 (cv::Point2f)
    │
    ├─ 5. 疲劳指标计算
    │   ├─ EAR (Eye Aspect Ratio) 计算                 // 双眼纵横比
    │   ├─ MAR (Mouth Aspect Ratio) 计算               // 嘴巴纵横比
    │   ├─ PERCLOS 计算                                // 滑动窗口闭眼占比
    │   ├─ 连续闭眼帧计数
    │   └─ 连续张嘴帧计数
    │
    ├─ 6. 疲劳判定
    │   eye_fatigue = consecutiveEyeClosed >= 15
    │   mouth_fatigue = consecutiveMouthOpen >= 20
    │   perclos_fatigue = perclos > 0.5 (需≥100帧数据)
    │   is_tired = eye_fatigue || mouth_fatigue || perclos_fatigue
    │
    ├─ 7. 绘制标注图
    │   ├─ 人脸检测框（绿色正常/红色疲劳）
    │   ├─ 关键点连线
    │   └─ 疲劳指标文字
    │
    └─ 8. 发射信号
        emit frameReady(img, detections, fatigueInfo)  // 跨线程到UI
}
```

### 3.2 帧数据传输详情

| 阶段 | 数据类型 | 格式/尺寸 | 传输方向 |
|------|---------|----------|---------|
| 摄像头输出 | YUV420SP | 640x480, stride对齐 | 硬件→CameraService |
| grabFrameRGB | QImage | 640x480, RGB888 | CameraService→Worker |
| 裁剪旋转后 | QImage | 480x480, RGB888 | Worker内部 |
| OpenCV转换 | cv::Mat | 480x480, CV_8UC3(BGR) | Worker→FatigueRunner |
| RetinaFace输出 | retinaface_result | 最多128个检测框 | FatigueRunner内部 |
| PFLD输出 | vector<Point2f> | 106个关键点 | FatigueRunner内部 |
| 标注图 | QImage | 480x480, RGB888 | FatigueRunner→Worker |
| frameReady信号 | QImage + QVector + QStringList | 跨线程 | Worker→UI主线程 |

---

## 4. 线程间通信机制

### 4.1 采集线程 → UI主线程

**信号**: `CameraCaptureWorker::frameReady`

```
信号定义 (camerapage.cpp:L36):
    void frameReady(const QImage &img,
                    const QVector<CameraDetectionOverlay> &detections,
                    const QStringList &fatigueInfo);

连接方式 (camerapage.cpp:L219):
    connect(worker, &CameraCaptureWorker::frameReady, m_preview, [...]);

数据传输:
    QImage img          → 预览画面（含标注）
    QVector<CameraDetectionOverlay> detections → 检测框信息
    QStringList fatigueInfo → 疲劳指标文本列表
```

由于信号从子线程发射到主线程，Qt 使用**队列连接（QueuedConnection）** 自动处理线程安全。这就是为什么需要在构造函数中 `qRegisterMetaType` 注册自定义类型。

### 4.2 UI主线程 → 百度API → UI主线程

**信号**: `BaiduGestureClient::inferenceFinished`

```
触发流程:
    m_cloudTimer (1秒间隔)
      → m_cloudClient->infer(m_lastFrame)     // 发送当前帧
        → requestToken()                       // 获取/刷新 access_token
          → HTTP POST → 百度OAuth2服务
          ← JSON {access_token, expires_in}
        → requestGesture(jpegBytes)            // 发送图像识别请求
          → HTTP POST → 百度手势识别API
          ← JSON {result: [{classname, probability}]}
      ← emit inferenceFinished(ok, lines)      // 返回识别结果

连接方式 (camerapage.cpp:L272):
    connect(m_cloudClient, &BaiduGestureClient::inferenceFinished, this, [...]);
```

### 4.3 CameraPage → MainWindow 信号

| 信号 | 触发条件 | 数据 | 接收处理 |
|------|---------|------|---------|
| `gestureDetected(QString)` | 百度API返回有效手势 | 手势名称如"Fist" | 音乐控制 + 串口发送JSON |
| `fatigueDetected(bool)` | 每帧疲劳判定结果 | true/false | 警告音控制 + 串口发送JSON |
| `toggleRecordingRequested()` | 录制按钮点击 | 无 | 切换录制状态 |

---

## 5. 关键数据结构

### 5.1 CameraDetectionOverlay

```cpp
// hudwidgets.h:L156
struct CameraDetectionOverlay {
    QRectF rect;      // 检测框在图像中的归一化坐标
    QString label;    // "FACE" 或 "TIRED"
    float score;      // 置信度
};
```

### 5.2 FatigueResult

```cpp
// fatigue_runner.h:L29
struct FatigueResult {
    std::vector<FatigueFaceResult> faces;
};

struct FatigueFaceResult {
    cv::Rect face_rect;                    // 人脸矩形
    bool is_tired;                         // 疲劳判定
    FatigueMetrics metrics;                // 疲劳指标
    std::vector<cv::Point2f> landmarks;    // 106个关键点
};

struct FatigueMetrics {
    float ear;                     // 眼睛纵横比
    float mar;                     // 嘴巴纵横比
    float perclos;                 // 闭眼时间占比
    int consecutive_eye_closed;    // 连续闭眼帧数
    int consecutive_mouth_open;    // 连续张嘴帧数
};
```

### 5.3 retinaface_result

```cpp
// retinaface_facenet.h:L37
typedef struct {
    int count;                              // 检测到的人脸数量
    retinaface_object_t object[128];        // 最多128个检测结果
} retinaface_result;

typedef struct retinaface_object_t {
    int cls;                    // 类别
    box_rect_t box;             // 边界框 {left, top, right, bottom}
    float score;                // 置信度
    ponit_t ponit[5];           // 5个关键点
} retinaface_object_t;
```

---

## 6. 疲劳检测详细流程

```
RetinaFace 人脸检测
  │
  ├─ letterbox 预处理（保持宽高比缩放 + 填充）
  ├─ RKNN 推理（零拷贝输入输出）
  ├─ 后处理：解码框 + 置信度过滤 + NMS
  └─ 输出：人脸边界框 + 5个关键点
      │
      ▼
PFLD 关键点检测（对每张人脸）
  │
  ├─ 根据检测框裁剪人脸 ROI
  ├─ 缩放到 112x112
  ├─ RKNN 推理
  └─ 输出：106个关键点坐标
      │
      ▼
疲劳指标计算
  │
  ├─ EAR 计算（左右眼各6个关键点）
  │   EAR = (||p2-p6|| + ||p3-p5||) / (2 * ||p1-p4||)
  │   阈值: < 0.22 判定闭眼
  │
  ├─ MAR 计算（嘴巴6个关键点）
  │   MAR = (||p2-p8|| + ||p3-p7|| + ||p4-p6||) / (2 * ||p1-p9||)
  │   阈值: > 0.60 判定张嘴
  │
  ├─ PERCLOS 计算（滑动窗口200帧）
  │   PERCLOS = 闭眼帧数 / 总帧数
  │   阈值: > 0.5 判定疲劳
  │   保护: 缓冲区 < 100帧时不计算
  │
  ├─ 连续闭眼计数
  │   EAR < 0.22 → +1
  │   EAR ≥ 0.22 → max(0, -1)
  │   阈值: ≥ 15帧判定疲劳
  │
  └─ 连续张嘴计数
      MAR > 0.60 → +1
      MAR ≤ 0.60 → max(0, -1)
      阈值: ≥ 20帧判定疲劳
```

---

## 7. 手势识别流程

```
QTimer (1秒间隔)                           // camerapage.cpp:L281
  │
  ├─ 检查: m_cloudInFlight == false?       // 上一次是否完成
  ├─ 检查: m_lastFrame.isNull() == false?  // 是否有图像
  │
  ▼
BaiduGestureClient::infer(m_lastFrame)     // baidu_gesture_client.cpp
  │
  ├─ QImage → JPEG 编码 (质量75)
  ├─ 检查 access_token 是否有效
  │   ├─ 无效 → requestToken()
  │   │   ├─ HTTP POST → 百度OAuth2
  │   │   └─ 保存 token + 过期时间
  │   └─ 有效 → requestGesture(jpegBytes)
  │       ├─ JPEG → Base64 → URL编码
  │       ├─ HTTP POST → 百度手势API
  │       └─ 解析 JSON 响应
  │           └─ 提取 classname + probability
  │
  ▼
emit inferenceFinished(ok, lines)          // 返回识别结果
  │
  ▼
processGestureResult(lines)                // camerapage.cpp:L312
  │
  ├─ 关键词匹配（FIST/FIVE/ONE/TWO/OK/...）
  ├─ 无匹配 → 不发射信号
  └─ 有匹配 → emit gestureDetected(gestureName)
      │
      ▼
MainWindow 接收                            // mainwindow.cpp:L239
  │
  ├─ 手势→动作映射:
  │   Fist → pause (暂停)
  │   Five → play  (播放)
  │   One  → prev  (上一首)
  │   Two  → next  (下一首)
  │
  ├─ 执行音乐控制
  └─ 组装JSON发送串口
      {"type":"gesture","data":{"gesture":"Fist","action":"pause"}}
```

---

## 8. 生命周期与资源释放

### 8.1 正常退出

```
CameraPage::~CameraPage()                   // camerapage.cpp:L300
  └─ m_worker->stop()                       // 设置 m_running = false
      │
      ▼ (采集线程循环退出)
      │
  m_runner.deinit()                         // 释放 RKNN 模型
  │   ├─ release_retinaface_model()         // 释放 RetinaFace
  │   └─ m_pfldModel->deinit()             // 释放 PFLD
  │
  m_service.shutdown()                      // 关闭摄像头
  │   ├─ RK_MPI_VI_DisableChn(0, 0)
  │   ├─ RK_MPI_VI_DisableDev(0)
  │   ├─ SAMPLE_COMM_ISP_Stop(0)
  │   └─ RK_MPI_SYS_Exit()
  │
  emit finished()                           // 通知线程可退出
      │
      ▼
  thread->quit()                            // QThread 退出事件循环
  worker->deleteLater()                     // 延迟删除 Worker
  thread->deleteLater()                     // 延迟删除 Thread
```

### 8.2 信号连接与自动清理

```cpp
// camerapage.cpp:L219-225
connect(thread, &QThread::started, worker, &CameraCaptureWorker::start);
connect(this, &CameraPage::destroyed, worker, &CameraCaptureWorker::stop);
connect(this, &CameraPage::destroyed, thread, &QThread::quit);
connect(worker, &CameraCaptureWorker::finished, thread, &QThread::quit);
connect(worker, &CameraCaptureWorker::finished, worker, &CameraCaptureWorker::deleteLater);
connect(thread, &QThread::finished, thread, &QThread::deleteLater);
```

---

## 9. 关键技术细节

### 9.1 跨线程信号注册

```cpp
// camerapage.cpp:L146-147
qRegisterMetaType<QVector<CameraDetectionOverlay>>("QVector<CameraDetectionOverlay>");
qRegisterMetaType<QStringList>("QStringList");
```

由于 `frameReady` 信号携带自定义类型参数，从子线程发射到主线程时使用 `QueuedConnection`，Qt 需要在元类型系统中注册这些类型才能正确序列化和反序列化。

### 9.2 零拷贝 RKNN 推理

RetinaFace 模型使用 RV1106 的零拷贝 API：

```cpp
// retinaface_facenet.h
rknn_tensor_mem *input_mems[1];    // 输入内存（DMA映射）
rknn_tensor_mem *output_mems[3];   // 输出内存（DMA映射）
```

通过 `rknn_create_mem()` 分配的内存与 NPU 共享物理地址，避免数据拷贝。

### 9.3 Letterbox 预处理

RetinaFace 推理前对输入图像进行 letterbox 处理：

```cpp
// retinaface_facenet.h
typedef struct {
    float scale;    // 缩放比例
    int x_pad;      // 水平填充像素
    int y_pad;      // 垂直填充像素
} letterbox_t;
```

推理后需要用 `scale`、`x_pad`、`y_pad` 将检测框坐标映射回原图。

### 9.4 百度API限流机制

```cpp
// camerapage.cpp:L287-289
if (m_cloudInFlight) {
    return;    // 上一次请求未完成，跳过本次
}
```

通过 `m_cloudInFlight` 标志确保同一时间只有一个手势识别请求在飞行中，避免并发请求导致 API 限流或 token 冲突。

### 9.5 PERCLOS 保护机制

```cpp
// fatigue_runner.cpp:L1562
m_perclos = m_eyeStateBuffer.size() < static_cast<size_t>(m_perclosWindow / 2)
                ? 0.0f
                : static_cast<float>(closed_count) / m_eyeStateBuffer.size();
```

缓冲区帧数未达到窗口大小一半（100帧）时，PERCLOS 强制为 0，防止启动初期少量闭眼帧导致误判。
