#include "camerapage.h"

#include "hudwidgets.h"
#include "baidu_gesture_client.h"
#include "camera/camera_service.h"
#include "fatigue_runner.h"

#include <QImage>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMetaType>
#include <QPushButton>
#include <QSlider>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
#include <cstdio>
#include <opencv2/opencv.hpp>

class CameraCaptureWorker final : public QObject
{
    Q_OBJECT
public:
    explicit CameraCaptureWorker(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

signals:
    void frameReady(const QImage &img, const QVector<CameraDetectionOverlay> &detections, const QStringList &fatigueInfo);

public slots:
    void start()
    {
        if (m_running) {
            return;
        }
        printf("[CameraCaptureWorker] start\n");
        m_running = true;

        m_width = 640;
        m_height = 480;

        if (!m_service.init(m_width, m_height)) {
            emit finished();
            return;
        }
        if (!m_runner.init("./model/RetinaFace640.rknn",
                           "./model/pfld_106.rknn",
                           112)) {
            m_service.shutdown();
            emit finished();
            return;
        }
        printf("[CameraCaptureWorker] enter capture loop\n");
        const bool enable_face = true;
        while (m_running) {
            if (!m_active) {
                QThread::msleep(100);
                continue;
            }
            QImage img;
            if (!m_service.grabFrameRGB(img, 1000)) {
                continue;
            }
            QVector<CameraDetectionOverlay> detections;
            QStringList fatigueInfo;
            if (m_runner.isReady() && enable_face) {
                QImage rgb = img.convertToFormat(QImage::Format_RGB888);
                
                const int targetSize = 480;
                const int x = (rgb.width() - targetSize) / 2;
                const int y = (rgb.height() - targetSize) / 2;
                QImage cropped = rgb.copy(x, y, targetSize, targetSize);
                
                QImage rotated = cropped.transformed(QTransform().rotate(-90));
                
                cv::Mat rgbMat(rotated.height(), rotated.width(), CV_8UC3,
                               const_cast<uchar *>(rotated.bits()), rotated.bytesPerLine());
                cv::Mat bgr;
                cv::cvtColor(rgbMat, bgr, cv::COLOR_RGB2BGR);
                FatigueResult fatigue;
                cv::Mat annotated;
                if (m_runner.run(bgr, &fatigue, &annotated) == 0 && !annotated.empty()) {
                    cv::Mat annotatedRgb;
                    cv::cvtColor(annotated, annotatedRgb, cv::COLOR_BGR2RGB);
                    QImage annotatedImg(annotatedRgb.data, annotatedRgb.cols, annotatedRgb.rows,
                                        annotatedRgb.step, QImage::Format_RGB888);
                    img = annotatedImg.copy();
                    
                    if (!fatigue.faces.empty()) {
                        const auto& face = fatigue.faces[0];
                        const auto& m = face.metrics;
                        QString status = face.is_tired ? "Fatigued" : "Normal";
                        fatigueInfo << QString("Status: %1").arg(status);
                        fatigueInfo << QString("EAR: %1").arg(QString::number(m.ear, 'f', 2));
                        fatigueInfo << QString("MAR: %1").arg(QString::number(m.mar, 'f', 2));
                        fatigueInfo << QString("PERCLOS: %1%").arg(QString::number(m.perclos * 100, 'f', 1));
                        if (m.consecutive_eye_closed > 0) {
                            fatigueInfo << QString("Eyes closed: %1 frames").arg(m.consecutive_eye_closed);
                        }
                        if (m.consecutive_mouth_open > 0) {
                            fatigueInfo << QString("Mouth open: %1 frames").arg(m.consecutive_mouth_open);
                        }

                        for (const auto& f : fatigue.faces) {
                            CameraDetectionOverlay det;
                            det.rect = QRectF(f.face_rect.x, f.face_rect.y,
                                              f.face_rect.width, f.face_rect.height);
                            det.label = f.is_tired ? "TIRED" : "FACE";
                            det.score = f.is_tired ? 0.0f : 1.0f;
                            detections.append(det);
                        }
                    }
                }
            }
            emit frameReady(std::move(img), detections, fatigueInfo);
        }

        m_runner.deinit();
        m_service.shutdown();


        printf("[CameraCaptureWorker] finished\n");
        emit finished();
    }

    void stop()
    {
        printf("[CameraCaptureWorker] stop\n");
        m_running = false;
    }

    void setActive(bool active)
    {
        m_active = active;
    }

signals:
    void finished();

private:
    bool m_running = false;
    volatile bool m_active = true;
    int m_width = 0;
    int m_height = 0;
    CameraService m_service;
    FatigueDetectorRunner m_runner;
};

CameraPage::CameraPage(QWidget *parent)
    : QWidget(parent)
{
    qRegisterMetaType<QVector<CameraDetectionOverlay>>("QVector<CameraDetectionOverlay>");
    qRegisterMetaType<QStringList>("QStringList");
    m_preview = new CameraPreviewWidget(this);
    auto *root = new QGridLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->addWidget(m_preview);

    auto *grid = new QGridLayout(m_preview);
    grid->setContentsMargins(8, 8, 8, 8);
    grid->setSpacing(0);

    auto *rightBar = new QFrame(m_preview);
    rightBar->setProperty("glass", true);
    rightBar->setFixedWidth(110);
    rightBar->setStyleSheet("border-radius: 14px;");
    auto *rightCol = new QVBoxLayout(rightBar);
    rightCol->setContentsMargins(8, 10, 8, 10);
    rightCol->setSpacing(6);
    rightCol->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    auto *faceTitle = new QLabel("Face Detection", rightBar);
    faceTitle->setStyleSheet("color: rgba(34,197,94,0.90); font-weight: 700; font-size: 11px; border: none;");
    faceTitle->setAlignment(Qt::AlignCenter);
    rightCol->addWidget(faceTitle);

    m_fatigueLabel = new QLabel("Status: --", rightBar);
    m_fatigueLabel->setStyleSheet("color: rgba(255,255,255,0.85); font-weight: 600; font-size: 10px; border: none;");
    m_fatigueLabel->setAlignment(Qt::AlignCenter);
    m_fatigueLabel->setWordWrap(true);
    rightCol->addWidget(m_fatigueLabel);

    m_earLabel = new QLabel("EAR: --", rightBar);
    m_earLabel->setStyleSheet("color: rgba(255,255,255,0.75); font-weight: 500; font-size: 10px; border: none;");
    m_earLabel->setAlignment(Qt::AlignCenter);
    rightCol->addWidget(m_earLabel);

    m_marLabel = new QLabel("MAR: --", rightBar);
    m_marLabel->setStyleSheet("color: rgba(255,255,255,0.75); font-weight: 500; font-size: 10px; border: none;");
    m_marLabel->setAlignment(Qt::AlignCenter);
    rightCol->addWidget(m_marLabel);

    m_perclosLabel = new QLabel("PERCLOS: --", rightBar);
    m_perclosLabel->setStyleSheet("color: rgba(255,255,255,0.75); font-weight: 500; font-size: 10px; border: none;");
    m_perclosLabel->setAlignment(Qt::AlignCenter);
    rightCol->addWidget(m_perclosLabel);

    auto *sep1 = new QFrame(rightBar);
    sep1->setFixedHeight(1);
    sep1->setStyleSheet("background: rgba(255,255,255,0.15); border: none;");
    rightCol->addWidget(sep1);

    auto *gestureTitle = new QLabel("Gesture", rightBar);
    gestureTitle->setStyleSheet("color: rgba(59,130,246,0.90); font-weight: 700; font-size: 11px; border: none;");
    gestureTitle->setAlignment(Qt::AlignCenter);
    rightCol->addWidget(gestureTitle);

    m_gestureLabel = new QLabel("--", rightBar);
    m_gestureLabel->setStyleSheet("color: rgba(255,255,255,0.85); font-weight: 600; font-size: 12px; border: none;");
    m_gestureLabel->setAlignment(Qt::AlignCenter);
    rightCol->addWidget(m_gestureLabel);

    rightCol->addStretch();

    grid->addWidget(rightBar, 0, 2, 3, 1, Qt::AlignRight | Qt::AlignVCenter);
    grid->setColumnStretch(1, 1);
    grid->setRowStretch(1, 1);

    auto *worker = new CameraCaptureWorker();
    auto *thread = new QThread(this);
    worker->moveToThread(thread);
    connect(thread, &QThread::started, worker, &CameraCaptureWorker::start);
    connect(this, &CameraPage::destroyed, worker, &CameraCaptureWorker::stop);
    connect(this, &CameraPage::destroyed, thread, &QThread::quit);
    connect(worker, &CameraCaptureWorker::finished, thread, &QThread::quit);
    connect(worker, &CameraCaptureWorker::finished, worker, &CameraCaptureWorker::deleteLater);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    connect(worker, &CameraCaptureWorker::frameReady, m_preview,
            [this](const QImage &img, const QVector<CameraDetectionOverlay> &detections, const QStringList &fatigueInfo) {
                if (m_preview) {
                    m_preview->setFrame(img);
                    m_preview->setDetections(detections);
                    m_preview->setFatigueInfo(QStringList());
                }
                m_lastFrame = img;

                bool isFatigue = false;
                for (const QString &line : fatigueInfo) {
                    if (line.contains("Fatigued")) {
                        isFatigue = true;
                    }
                    if (line.startsWith("Status:")) {
                        if (m_fatigueLabel) {
                            m_fatigueLabel->setText(line);
                            m_fatigueLabel->setStyleSheet(line.contains("Fatigued")
                                ? "color: rgba(239,68,68,0.95); font-weight: 700; font-size: 10px; border: none;"
                                : "color: rgba(34,197,94,0.90); font-weight: 600; font-size: 10px; border: none;");
                        }
                    } else if (line.startsWith("EAR:")) {
                        if (m_earLabel) m_earLabel->setText(line);
                    } else if (line.startsWith("MAR:")) {
                        if (m_marLabel) m_marLabel->setText(line);
                    } else if (line.startsWith("PERCLOS:")) {
                        if (m_perclosLabel) m_perclosLabel->setText(line);
                    }
                }
                if (fatigueInfo.isEmpty()) {
                    if (m_fatigueLabel) {
                        m_fatigueLabel->setText("Status: --");
                        m_fatigueLabel->setStyleSheet("color: rgba(255,255,255,0.85); font-weight: 600; font-size: 10px; border: none;");
                    }
                    if (m_earLabel) m_earLabel->setText("EAR: --");
                    if (m_marLabel) m_marLabel->setText("MAR: --");
                    if (m_perclosLabel) m_perclosLabel->setText("PERCLOS: --");
                }
                emit fatigueDetected(isFatigue);
            });
    thread->start();

    m_worker = worker;

    m_cloudClient = new BaiduGestureClient(this);
    connect(m_cloudClient, &BaiduGestureClient::inferenceFinished, this,
            [this](bool, const QStringList &lines) {
                m_cloudInFlight = false;
                if (m_preview) {
                    m_preview->setOverlayText(QStringList());
                }
                processGestureResult(lines);
            });

    m_cloudTimer = new QTimer(this);
    m_cloudTimer->setInterval(1000);
    connect(m_cloudTimer, &QTimer::timeout, this, [this]() {
        if (!m_cloudClient || !m_preview) {
            return;
        }
        if (m_cloudInFlight) {
            return;
        }
        if (m_lastFrame.isNull()) {
            return;
        }
        m_cloudInFlight = true;
        if (m_gestureLabel) m_gestureLabel->setText("Recognizing...");
        m_cloudClient->infer(m_lastFrame);
    });
    m_cloudTimer->start();
}

CameraPage::~CameraPage()
{
    if (m_worker) {
        m_worker->stop();
    }
}

void CameraPage::setRecording(bool recording)
{
}

void CameraPage::setActive(bool active)
{
    if (m_worker) {
        m_worker->setActive(active);
    }
    if (m_cloudTimer) {
        if (active) {
            m_cloudTimer->start();
        } else {
            m_cloudTimer->stop();
            m_cloudInFlight = false;
        }
    }
}

void CameraPage::processGestureResult(const QStringList &lines)
{
    QString detectedGesture;
    for (const QString &line : lines) {
        QString upperLine = line.toUpper();
        if (upperLine.contains("FIST")) {
            detectedGesture = "Fist";
            break;
        } else if (upperLine.contains("FIVE") || upperLine.contains("PALM")) {
            detectedGesture = "Five";
            break;
        } else if (upperLine.contains("ONE") || upperLine.contains("INDEX")) {
            detectedGesture = "One";
            break;
        } else if (upperLine.contains("TWO")) {
            detectedGesture = "Two";
            break;
        } else if (upperLine.contains("OK")) {
            detectedGesture = "OK";
            break;
        } else if (upperLine.contains("PRAYER")) {
            detectedGesture = "Prayer";
            break;
        } else if (upperLine.contains("CONGRATULATION")) {
            detectedGesture = "Congratulation";
            break;
        } else if (upperLine.contains("HONOUR")) {
            detectedGesture = "Honour";
            break;
        } else if (upperLine.contains("HEART_SINGLE")) {
            detectedGesture = "Heart_single";
            break;
        } else if (upperLine.contains("THUMB_UP")) {
            detectedGesture = "Thumb_up";
            break;
        } else if (upperLine.contains("THUMB_DOWN")) {
            detectedGesture = "Thumb_down";
            break;
        } else if (upperLine.contains("ILY")) {
            detectedGesture = "ILY";
            break;
        } else if (upperLine.contains("PALM_UP")) {
            detectedGesture = "Palm_up";
            break;
        } else if (upperLine.contains("HEART_1")) {
            detectedGesture = "Heart_1";
            break;
        } else if (upperLine.contains("HEART_2")) {
            detectedGesture = "Heart_2";
            break;
        } else if (upperLine.contains("HEART_3")) {
            detectedGesture = "Heart_3";
            break;
        } else if (upperLine.contains("THREE")) {
            detectedGesture = "Three";
            break;
        } else if (upperLine.contains("FOUR")) {
            detectedGesture = "Four";
            break;
        } else if (upperLine.contains("SIX")) {
            detectedGesture = "Six";
            break;
        } else if (upperLine.contains("SEVEN")) {
            detectedGesture = "Seven";
            break;
        } else if (upperLine.contains("EIGHT")) {
            detectedGesture = "Eight";
            break;
        } else if (upperLine.contains("NINE")) {
            detectedGesture = "Nine";
            break;
        } else if (upperLine.contains("ROCK")) {
            detectedGesture = "Rock";
            break;
        } else if (upperLine.contains("INSULT")) {
            detectedGesture = "Insult";
            break;
        }
    }
    
    if (detectedGesture.isEmpty()) {
          if (m_gestureLabel) m_gestureLabel->setText("--");
          return;
      }
      
      if (m_gestureLabel) m_gestureLabel->setText(detectedGesture);
      emit gestureDetected(detectedGesture);
}

#include "camerapage.moc"
