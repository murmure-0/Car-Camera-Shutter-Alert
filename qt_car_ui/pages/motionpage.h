#ifndef MOTIONPAGE_H
#define MOTIONPAGE_H

#include <QWidget>

class ArtificialHorizonWidget;
class QLabel;
class QFrame;
class QTimer;
class QProcess;

class MotionPage : public QWidget
{
    Q_OBJECT
public:
    explicit MotionPage(QWidget *parent = nullptr);
    void updateMotion(double ax, double ay, double az, double gx, double gy, double gz, double pitch, double roll, double yaw);

signals:
    void rolloverWarning(bool active);

private:
    ArtificialHorizonWidget *m_horizonWidget = nullptr;
    QLabel *m_pitchVal = nullptr;
    QLabel *m_rollVal = nullptr;
    QFrame *m_gDot = nullptr;
    QLabel *m_gxVal = nullptr;
    QLabel *m_gyVal = nullptr;
    QLabel *m_gzVal = nullptr;
    QLabel *m_rotXVal = nullptr;
    QLabel *m_rotYVal = nullptr;
    QLabel *m_rotZVal = nullptr;

    QFrame *m_warningOverlay = nullptr;
    QLabel *m_warningIcon = nullptr;
    QLabel *m_warningText = nullptr;
    QTimer *m_flashTimer = nullptr;
    bool m_flashState = false;
    bool m_isRolloverWarning = false;

    QProcess *m_soundProcess = nullptr;
    QTimer *m_soundRepeatTimer = nullptr;
    bool m_isSoundActive = false;

    void startRolloverWarning();
    void stopRolloverWarning();
    void playWarningSound();
    void stopWarningSound();

protected:
    void resizeEvent(QResizeEvent *event) override;
};

#endif
