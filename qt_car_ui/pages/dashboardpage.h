#ifndef DASHBOARDPAGE_H
#define DASHBOARDPAGE_H

#include <QWidget>
#include <QStringList>

class QLabel;
class QNetworkAccessManager;
class MapBackgroundWidget;
class QProcess;
class QPushButton;
class QSlider;
class QTimer;
class QProgressBar;

class DashboardPage : public QWidget
{
    Q_OBJECT
public:
    explicit DashboardPage(QWidget *parent = nullptr);
    void updateEnvironmentSummary(double temp, double humi);
    void updateGpsSummary(double lat, double lon, int sat);
    
    void togglePlayPause();
    void nextSong();
    void prevSong();
    bool isPlaying() const { return m_isPlaying; }
    
    void setFatigueWarning(bool active);

signals:
    void requestGps();
    void requestEnvironment();

private:
    void requestMap();
    void scanMusicDir();
    void playMusic(int index);
    void stopMusic();
    void updateTimeLabel();
    void seekToPosition(int position);

    MapBackgroundWidget *m_map = nullptr;
    QNetworkAccessManager *m_network = nullptr;
    QLabel *m_coordLabel = nullptr;
    QLabel *m_tempVal = nullptr;
    QLabel *m_humVal = nullptr;
    QString m_center = QStringLiteral("Baidu Building");
    int m_zoom = 10;

    QProcess *m_musicProcess = nullptr;
    QStringList m_musicFiles;
    int m_currentIndex = -1;
    bool m_isPlaying = false;

    QPushButton *m_playBtn = nullptr;
    QPushButton *m_prevBtn = nullptr;
    QPushButton *m_nextBtn = nullptr;
    QLabel *m_songLabel = nullptr;
    QLabel *m_timeLabel = nullptr;
    QSlider *m_progressSlider = nullptr;
    QTimer *m_progressTimer = nullptr;
    
    int m_songDuration = 0;
    int m_currentPosition = 0;
    
    QProgressBar *m_cpuBar = nullptr;
    QProgressBar *m_memBar = nullptr;
    QLabel *m_cpuTempLabel = nullptr;
    QTimer *m_sysTimer = nullptr;
    
    void updateSystemStats();
    long long m_lastCpuTotal = 0;
    long long m_lastCpuIdle = 0;
    
    QProcess *m_soundProcess = nullptr;
    void playSound(const QString &soundFile);
    QTimer *m_fatigueWarningTimer = nullptr;
    bool m_isFatigueWarningActive = false;
};

#endif // DASHBOARDPAGE_H
