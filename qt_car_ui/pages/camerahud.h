/**
 * @file camerahud.h
 * @brief Standalone camera HUD demo window declaration.
 */

#pragma once

#include <QElapsedTimer>
#include <QMainWindow>

class QLabel;
class QPushButton;
class QSlider;
class QTimer;

class CameraHud : public QMainWindow
{
    Q_OBJECT

public:
    explicit CameraHud(QWidget *parent = nullptr);

private:
    void setRecording(bool recording);
    void updateRecText();
    void updateRecDot();

    bool m_recording = true;
    int m_elapsedSeconds = (4 * 60) + 23;

    QTimer *m_recTimer = nullptr;
    QTimer *m_dotTimer = nullptr;
    QElapsedTimer m_dotPhaseTimer;

    QLabel *m_recDot = nullptr;
    QLabel *m_recLabel = nullptr;

    QPushButton *m_modeVideo = nullptr;
    QPushButton *m_modePhoto = nullptr;
    QPushButton *m_modeSlow = nullptr;

    QPushButton *m_shutterButton = nullptr;
    QWidget *m_shutterInner = nullptr;

    QSlider *m_zoomSlider = nullptr;
};
