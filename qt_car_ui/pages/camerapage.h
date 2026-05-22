#ifndef CAMERAPAGE_H
#define CAMERAPAGE_H

#include <QImage>
#include <QWidget>

class QSlider;
class QPushButton;
class QLabel;
class QWidget;
class CameraCaptureWorker;
class CameraPreviewWidget;
class QTimer;
class BaiduGestureClient;

class CameraPage : public QWidget
{
    Q_OBJECT
public:
    explicit CameraPage(QWidget *parent = nullptr);
    ~CameraPage();

    void setRecording(bool recording);
    void setActive(bool active);

signals:
    void toggleRecordingRequested();
    void gestureDetected(const QString &gesture);
    void fatigueDetected(bool isTired);

private:
    CameraCaptureWorker *m_worker = nullptr;
    CameraPreviewWidget *m_preview = nullptr;
    BaiduGestureClient *m_cloudClient = nullptr;
    QTimer *m_cloudTimer = nullptr;
    QImage m_lastFrame;
    bool m_cloudInFlight = false;

    QLabel *m_fatigueLabel = nullptr;
    QLabel *m_earLabel = nullptr;
    QLabel *m_marLabel = nullptr;
    QLabel *m_perclosLabel = nullptr;
    QLabel *m_gestureLabel = nullptr;

    void processGestureResult(const QStringList &lines);
    QString m_lastGesture;
    int m_gestureFrameCount = 0;
    QTimer *m_gestureCooldownTimer = nullptr;
    bool m_gestureOnCooldown = false;
};

#endif
