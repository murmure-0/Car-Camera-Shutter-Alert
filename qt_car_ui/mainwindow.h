/**
 * @file mainwindow.h
 * @brief Main window with page routing, scaling adaptation, and global state management.
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QElapsedTimer>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QButtonGroup;
class QGraphicsProxyWidget;
class QGraphicsScene;
class QGraphicsView;
class QLabel;
class QSerialPort;
class QStackedWidget;
class QTimer;
class QToolButton;
class QWidget;

class CameraPage;
class DashboardPage;
class EnvironmentPage;
class GpsPage;
class MotionPage;
class SettingsPage;

/**
 * @brief Main window and global UI framework.
 *
 * Manages the 800x480 design-resolution UI tree, QGraphicsView scaling,
 * sidebar navigation, page stack switching, status bar, and unified
 * theme/recording state coordination.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    enum class PageId {
        Dashboard = 0,
        Gps,
        Environment,
        Camera,
        Motion,
        Connectivity,
        Settings,
    };

    void updateViewScale();
    void setPage(PageId page);
    void applyOverlayTheme(PageId page);
    void updateClock();
    void setRecording(bool recording);
    void updateRecText();
    void updateRecDot();
    void setupSerial();
    void handleSerialReadyRead();
    void processSensorLine(const QByteArray &line);
    void sendSerialData(const QJsonObject &jsonData);

    QWidget *m_root = nullptr;
    QStackedWidget *m_stack = nullptr;
    QGraphicsView *m_view = nullptr;
    QGraphicsScene *m_scene = nullptr;
    QGraphicsProxyWidget *m_proxy = nullptr;

    QWidget *m_statusBar = nullptr;
    QLabel *m_statusLeftIcon = nullptr;
    QLabel *m_statusLeftText = nullptr;
    QLabel *m_clockLabel = nullptr;
    QLabel *m_statusRightText = nullptr;
    QWidget *m_battery = nullptr;

    QWidget *m_sideBar = nullptr;
    QButtonGroup *m_navGroup = nullptr;
    QToolButton *m_btnHome = nullptr;
    QToolButton *m_btnGps = nullptr;
    QToolButton *m_btnEnv = nullptr;
    QToolButton *m_btnCamera = nullptr;
    QToolButton *m_btnMotion = nullptr;
    QToolButton *m_btnNet = nullptr;
    QToolButton *m_btnSettings = nullptr;

    PageId m_currentPage = PageId::Dashboard;

    bool m_darkMode = true;
    bool m_recording = true;
    int m_recSeconds = (4 * 60) + 23;
    bool m_lastOverlay = false;
    QString m_lastQss;

    QTimer *m_clockTimer = nullptr;
    QTimer *m_recTimer = nullptr;
    QTimer *m_dotTimer = nullptr;
    QElapsedTimer m_dotPhaseTimer;

    QTimer *m_networkCheckTimer = nullptr;
    bool m_lastNetworkConnected = false;
    bool m_lastFatigueState = false;

    CameraPage *m_cameraPage = nullptr;
    DashboardPage *m_dashboardPage = nullptr;
    EnvironmentPage *m_environmentPage = nullptr;
    GpsPage *m_gpsPage = nullptr;
    MotionPage *m_motionPage = nullptr;
    SettingsPage *m_settingsPage = nullptr;
    QSerialPort *m_serialRx = nullptr;
    QSerialPort *m_serialTx = nullptr;
    QByteArray m_serialBuffer;

    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
