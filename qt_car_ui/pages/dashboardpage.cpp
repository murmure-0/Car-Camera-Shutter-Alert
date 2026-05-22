#include "dashboardpage.h"

#include "hudwidgets.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProgressBar>
#include <QPushButton>
#include <QtGlobal>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QtMath>
#include <QVBoxLayout>
#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QSlider>
#include <QThread>

DashboardPage::DashboardPage(QWidget *parent)
    : QWidget(parent)
{
    auto *grid = new QGridLayout(this);
    grid->setContentsMargins(16, 16, 16, 16);
    grid->setSpacing(16);

    auto *mapCard = new QPushButton(this);
    mapCard->setProperty("glass", true);
    mapCard->setProperty("card", true);
    mapCard->setCursor(Qt::PointingHandCursor);
    mapCard->setFlat(true);
    mapCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mapCard->setStyleSheet("QPushButton { text-align: left; padding: 0px; }");

    auto *mapCol = new QVBoxLayout(mapCard);
    mapCol->setContentsMargins(14, 14, 14, 14);
    mapCol->setSpacing(8);

    auto *topRow = new QHBoxLayout;
    m_coordLabel = new QLabel("30°N, 120°E", mapCard);
    m_coordLabel->setStyleSheet("color: rgba(255,255,255,0.85); background: rgba(0,0,0,0.35); padding: 4px 8px; border-radius: 8px; font-size: 11px;");
    auto *badge = new QLabel("Navigating", mapCard);
    badge->setStyleSheet("color: rgba(255,255,255,0.92); background: rgba(8,145,178,0.92); padding: 4px 8px; border-radius: 8px; font-weight: 700; font-size: 11px;");
    topRow->addWidget(m_coordLabel, 0, Qt::AlignLeft);
    topRow->addStretch(1);
    topRow->addWidget(badge, 0, Qt::AlignRight);
    mapCol->addLayout(topRow);

    m_map = new MapBackgroundWidget(mapCard);
    m_map->setMinimumHeight(300);
    m_map->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mapCol->addWidget(m_map, 1);

    auto *title = new QLabel("Main St & 5th Ave", mapCard);
    title->setStyleSheet("color: rgba(255,255,255,0.95); font-weight: 800; font-size: 18px;");
    auto *sub = new QLabel("Turn in 200m", mapCard);
    sub->setStyleSheet("color: rgba(226,232,240,0.70); font-size: 12px;");
    mapCol->addWidget(title);
    mapCol->addWidget(sub);
    mapCol->setStretchFactor(m_map, 1);

    connect(mapCard, &QPushButton::clicked, this, &DashboardPage::requestGps);

    auto *envCard = new QPushButton(this);
    envCard->setProperty("glass", true);
    envCard->setProperty("card", true);
    envCard->setCursor(Qt::PointingHandCursor);
    envCard->setFlat(true);
    envCard->setStyleSheet("QPushButton { text-align: left; padding: 0px; }");
    auto *envCol = new QVBoxLayout(envCard);
    envCol->setContentsMargins(14, 14, 14, 14);
    envCol->setSpacing(10);
    auto *envHdr = new QHBoxLayout;
    auto *envName = new QLabel("Env", envCard);
    envName->setStyleSheet("color: rgba(34,197,94,0.90); font-weight: 700; font-size: 12px;");
    auto *envIcon = new QLabel("🌿", envCard);
    envIcon->setStyleSheet("color: #22c55e; font-weight: 800; font-size: 11px;");
    envHdr->addWidget(envName);
    envHdr->addStretch(1);
    envHdr->addWidget(envIcon);
    envCol->addLayout(envHdr);
    auto *envBottom = new QHBoxLayout;
    envBottom->setContentsMargins(0, 0, 0, 0);
    envBottom->setSpacing(0);
    auto *tempWrap = new QWidget(envCard);
    tempWrap->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    auto *tempCol = new QVBoxLayout(tempWrap);
    tempCol->setContentsMargins(0, 0, 0, 0);
    tempCol->setSpacing(2);
    m_tempVal = new QLabel("24°C", tempWrap);
    m_tempVal->setStyleSheet("color: rgba(255,255,255,0.95); font-weight: 900; font-size: 28px;");
    auto *tempLbl = new QLabel("Temp", tempWrap);
    tempLbl->setStyleSheet("color: rgba(226,232,240,0.60); font-size: 11px;");
    tempCol->addWidget(m_tempVal);
    tempCol->addWidget(tempLbl);
    auto *humWrap = new QWidget(envCard);
    humWrap->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    auto *humCol = new QVBoxLayout(humWrap);
    humCol->setContentsMargins(0, 0, 0, 0);
    humCol->setSpacing(2);
    m_humVal = new QLabel("45%", humWrap);
    m_humVal->setStyleSheet("color: #60a5fa; font-weight: 900; font-size: 20px;");
    auto *humLbl = new QLabel("Humid", humWrap);
    humLbl->setStyleSheet("color: rgba(226,232,240,0.60); font-size: 11px;");
    humCol->addWidget(m_humVal, 0, Qt::AlignRight);
    humCol->addWidget(humLbl, 0, Qt::AlignRight);
    envBottom->addWidget(tempWrap, 0, Qt::AlignLeft | Qt::AlignVCenter);
    envBottom->addStretch(1);
    envBottom->addWidget(humWrap, 0, Qt::AlignRight | Qt::AlignVCenter);
    envCol->addStretch(1);
    envCol->addLayout(envBottom);
    envCol->addStretch(0);

    connect(envCard, &QPushButton::clicked, this, &DashboardPage::requestEnvironment);

    auto *sysCard = new QPushButton(this);
    auto *sysCol = new QVBoxLayout(sysCard);
    sysCol->setContentsMargins(14, 14, 14, 14);
    sysCol->setSpacing(10);
    auto *sysHdr = new QHBoxLayout;
    auto *sysName = new QLabel("System", sysCard);
    sysName->setStyleSheet("color: rgba(226,232,240,0.65); font-size: 11px; font-weight: 800;");
    auto *sysIcon = new QLabel("CPU", sysCard);
    sysIcon->setStyleSheet("color: #a855f7; font-weight: 800; font-size: 11px;");
    sysHdr->addWidget(sysName);
    sysHdr->addStretch(1);
    sysHdr->addWidget(sysIcon);
    sysCol->addLayout(sysHdr);

    auto *cpuRow = new QHBoxLayout;
    auto *cpuLbl = new QLabel("CPU", cpuRow);
    cpuLbl->setStyleSheet("color: rgba(226,232,240,0.75); font-size: 11px;");
    m_cpuBar = new QProgressBar(cpuRow);
    m_cpuBar->setRange(0, 100);
    m_cpuBar->setValue(0);
    m_cpuBar->setTextVisible(false);
    m_cpuBar->setFixedHeight(8);
    m_cpuBar->setStyleSheet("QProgressBar::chunk{background:#a855f7;}");
    cpuLayout->addWidget(cpuLbl);
    cpuLayout->addStretch(1);
    cpuLayout->addWidget(m_cpuBar);
    sysCol->addWidget(cpuRow);

    auto *memRow = new QWidget(sysCard);
    auto *memLayout = new QHBoxLayout(memRow);
    memLayout->setContentsMargins(0, 0, 0, 0);
    memLayout->setSpacing(10);
    auto *memLbl = new QLabel("Mem", memRow);
    memLbl->setStyleSheet("color: rgba(226,232,240,0.75); font-size: 11px;");
    m_memBar = new QProgressBar(memRow);
    m_memBar->setRange(0, 100);
    m_memBar->setValue(0);
    m_memBar->setTextVisible(false);
    m_memBar->setFixedHeight(8);
    m_memBar->setStyleSheet("QProgressBar::chunk{background:#3b82f6;}");
    memLayout->addWidget(memLbl);
    memLayout->addStretch(1);
    memLayout->addWidget(m_memBar);
    sysCol->addWidget(memRow);

    auto *cpuTempRow = new QWidget(sysCard);
    auto *cpuTempLayout = new QHBoxLayout(cpuTempRow);
    cpuTempLayout->setContentsMargins(0, 0, 0, 0);
    cpuTempLayout->setSpacing(10);
    auto *cpuTempNameLbl = new QLabel("Temp", cpuTempRow);
    cpuTempNameLbl->setStyleSheet("color: rgba(226,232,240,0.75); font-size: 11px;");
    m_cpuTempLabel = new QLabel("--°C", cpuTempRow);
    m_cpuTempLabel->setStyleSheet("color: #22c55e; font-size: 11px; font-weight: 700;");
    cpuTempLayout->addWidget(cpuTempNameLbl);
    cpuTempLayout->addStretch(1);
    cpuTempLayout->addWidget(m_cpuTempLabel);
    sysCol->addWidget(cpuTempRow);

    sysCol->addStretch(1);

    auto *musicCard = new QPushButton(this);
    auto *musicCol = new QVBoxLayout(musicCard);
    musicCol->setContentsMargins(14, 12, 14, 12);
    musicCol->setSpacing(8);
    
    auto *musicHdr = new QHBoxLayout;
    auto *musicName = new QLabel("🎵 Music", musicCard);
    musicName->setStyleSheet("color: rgba(226,232,240,0.65); font-size: 11px; font-weight: 800;");
    m_songLabel = new QLabel("No song selected", musicCard);
    m_songLabel->setStyleSheet("color: rgba(255,255,255,0.85); font-size: 12px; font-weight: 700;");
    musicHdr->addWidget(musicName);
    musicHdr->addStretch(1);
    musicHdr->addWidget(m_songLabel);
    musicCol->addLayout(musicHdr);

    auto *ctrlRow = new QHBoxLayout;
    ctrlRow->setContentsMargins(0, 0, 0, 0);
    ctrlRow->setSpacing(10);

    m_prevBtn = new QPushButton("|<<", musicCard);
    m_prevBtn->setFixedSize(44, 36);
    m_prevBtn->setStyleSheet(
        "QPushButton { background: rgba(255,255,255,0.15); border-radius: 18px; color: white; font-size: 12px; font-weight: bold; }"
        "QPushButton:hover { background: rgba(255,255,255,0.25); }"
        "QPushButton:pressed { background: rgba(34,211,238,0.5); }");

    m_playBtn = new QPushButton(">", musicCard);
    m_playBtn->setFixedSize(50, 44);
    m_playBtn->setStyleSheet(
        "QPushButton { background: #22d3ee; border-radius: 22px; color: white; font-size: 18px; font-weight: bold; }"
        "QPushButton:hover { background: #38bdf8; }"
        "QPushButton:pressed { background: #0891b2; }");

    m_nextBtn = new QPushButton(">>|", musicCard);
    m_nextBtn->setFixedSize(44, 36);
    m_nextBtn->setStyleSheet(
        "QPushButton { background: rgba(255,255,255,0.15); border-radius: 18px; color: white; font-size: 12px; font-weight: bold; }"
        "QPushButton:hover { background: rgba(255,255,255,0.25); }"
        "QPushButton:pressed { background: rgba(34,211,238,0.5); }");

    ctrlRow->addStretch(1);
    ctrlRow->addWidget(m_prevBtn);
    ctrlRow->addSpacing(4);
    ctrlRow->addWidget(m_playBtn);
    ctrlRow->addSpacing(4);
    ctrlRow->addWidget(m_nextBtn);
    ctrlRow->addStretch(1);
    musicCol->addLayout(ctrlRow);

    auto *progressRow = new QHBoxLayout;
    progressRow->setContentsMargins(0, 0, 0, 0);
    progressRow->setSpacing(8);
    
    m_timeLabel = new QLabel("0:00 / 0:00", musicCard);
    m_timeLabel->setStyleSheet("color: rgba(226,232,240,0.70); font-size: 10px;");
    m_timeLabel->setFixedWidth(80);
    
    m_progressSlider = new QSlider(Qt::Horizontal, musicCard);
    m_progressSlider->setRange(0, 100);
    m_progressSlider->setValue(0);
    m_progressSlider->setStyleSheet(
        "QSlider::groove:horizontal { height: 4px; background: rgba(255,255,255,0.2); border-radius: 2px; }"
        "QSlider::sub-page:horizontal { background: #22d3ee; border-radius: 2px; }"
        "QSlider::handle:horizontal { width: 10px; height: 10px; margin: -3px 0; background: #22d3ee; border-radius: 5px; }"
        "QSlider::handle:horizontal:hover { background: #38bdf8; }");
    
    progressRow->addWidget(m_timeLabel);
    progressRow->addWidget(m_progressSlider, 1);
    musicCol->addLayout(progressRow);

    connect(m_playBtn, &QPushButton::clicked, this, &DashboardPage::togglePlayPause);
    connect(m_prevBtn, &QPushButton::clicked, this, &DashboardPage::prevSong);
    connect(m_nextBtn, &QPushButton::clicked, this, &DashboardPage::nextSong);
    connect(m_progressSlider, &QSlider::sliderReleased, this, [this]() {
        int position = m_progressSlider->value();
        seekToPosition(position);
    });

    grid->addWidget(mapCard, 0, 0, 2, 2);
    grid->addWidget(envCard, 0, 2, 1, 1);
    grid->addWidget(sysCard, 1, 2, 1, 1);
    grid->addWidget(musicCard, 2, 0, 1, 3);
    grid->setRowStretch(0, 1);
    grid->setRowStretch(1, 1);
    grid->setRowStretch(2, 0);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(2, 1);

    m_network = new QNetworkAccessManager(this);
    QTimer::singleShot(0, this, &DashboardPage::requestMap);

    m_musicProcess = new QProcess(this);
    connect(m_musicProcess, static_cast<void(QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished),
            this, [this](int code, QProcess::ExitStatus status) {
                Q_UNUSED(code)
                Q_UNUSED(status)
                if (m_isPlaying) {
                    QTimer::singleShot(100, this, [this]() {
                        if (m_isPlaying) {
                            int next = (m_currentIndex + 1) % m_musicFiles.size();
                            playMusic(next);
                        }
                    });
                }
            });
    scanMusicDir();
    
    m_soundProcess = new QProcess(this);
    
    m_fatigueWarningTimer = new QTimer(this);
    m_fatigueWarningTimer->setInterval(5000);
    connect(m_fatigueWarningTimer, &QTimer::timeout, this, [this]() {
        if (m_isFatigueWarningActive) {
            playSound("./Sound_Efftcts/Fatigue_warning.mp3");
        }
    });

    m_sysTimer = new QTimer(this);
    connect(m_sysTimer, &QTimer::timeout, this, &DashboardPage::updateSystemStats);
    m_sysTimer->start(2000);
    updateSystemStats();
}

void DashboardPage::updateEnvironmentSummary(double temp, double humi)
{
    if (m_tempVal) {
        m_tempVal->setText(QString::number(temp, 'f', 1) + "°C");
    }
    if (m_humVal) {
        m_humVal->setText(QString::number(int(qRound(humi))) + "%");
    }
}

void DashboardPage::updateGpsSummary(double lat, double lon, int sat)
{
    if (!m_coordLabel) {
        return;
    }
    QString coord = QString::number(lat, 'f', 6) + ", " + QString::number(lon, 'f', 6);
    if (sat >= 0) {
        coord += "  S:" + QString::number(sat);
    }
    m_coordLabel->setText(coord);
}

void DashboardPage::requestMap()
{
    if (!m_network || !m_map) {
        return;
    }

    const QSize fallbackSize(640, 360);
    const QSize targetSize = !m_map->size().isEmpty() ? m_map->size() : fallbackSize;
    const int minWidth = qMax(320, m_map->minimumWidth());
    const int minHeight = qMax(240, m_map->minimumHeight());
    const int width = qBound(1, qMax(targetSize.width(), minWidth), 1024);
    const int height = qBound(1, qMax(targetSize.height(), minHeight), 1024);

    QUrl url(QStringLiteral("http://api.map.baidu.com/staticimage"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("center"), m_center);
    query.addQueryItem(QStringLiteral("markers"), m_center);
    query.addQueryItem(QStringLiteral("width"), QString::number(width));
    query.addQueryItem(QStringLiteral("height"), QString::number(height));
    query.addQueryItem(QStringLiteral("zoom"), QString::number(m_zoom));
    url.setQuery(query);

    QNetworkRequest request(url);
    auto *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray payload = reply->readAll();
        QImage image;
        image.loadFromData(payload);
        if (!image.isNull() && m_map) {
            m_map->setMapImage(image);
        }
        reply->deleteLater();
    });
}

void DashboardPage::scanMusicDir()
{
    m_musicFiles.clear();
    QDir musicDir("./music");
    if (!musicDir.exists()) {
        if (m_songLabel) {
            m_songLabel->setText("Music directory not found");
        }
        return;
    }
    QStringList filters;
    filters << "*.mp3" << "*.wav" << "*.ogg" << "*.flac" << "*.m4a";
    m_musicFiles = musicDir.entryList(filters, QDir::Files | QDir::NoDotAndDotDot);
    for (int i = 0; i < m_musicFiles.size(); ++i) {
        m_musicFiles[i] = musicDir.absoluteFilePath(m_musicFiles[i]);
    }
    if (m_musicFiles.isEmpty()) {
        if (m_songLabel) {
            m_songLabel->setText("No music files found");
        }
    } else {
        if (m_songLabel) {
            QFileInfo fi(m_musicFiles[0]);
            m_songLabel->setText(fi.baseName());
        }
    }
}

void DashboardPage::playSound(const QString &soundFile)
{
    if (!m_soundProcess) return;
    
    if (m_soundProcess->state() == QProcess::Running) {
        return;
    }
    
    QStringList args;
    args << "-q" << soundFile;
    m_soundProcess->start("play", args);
}

void DashboardPage::setFatigueWarning(bool active)
{
    m_isFatigueWarningActive = active;
    
    if (active) {
        playSound("./Sound_Efftcts/Fatigue_warning.mp3");
        if (m_fatigueWarningTimer && !m_fatigueWarningTimer->isActive()) {
            m_fatigueWarningTimer->start();
        }
    } else {
        if (m_fatigueWarningTimer && m_fatigueWarningTimer->isActive()) {
            m_fatigueWarningTimer->stop();
        }
        if (m_soundProcess && m_soundProcess->state() == QProcess::Running) {
            m_soundProcess->kill();
            m_soundProcess->waitForFinished(500);
        }
    }
}

void DashboardPage::playMusic(int index)
{
    if (index < 0 || index >= m_musicFiles.size()) return;
    
    if (m_musicProcess && m_musicProcess->state() == QProcess::Running) {
        m_musicProcess->terminate();
        m_musicProcess->waitForFinished(1000);
    }
    
    stopMusic();
    m_currentIndex = index;
    m_currentPosition = 0;
    
    m_songDuration = 180;
    
    if (!m_musicProcess) return;
    
    m_musicProcess->start("play", QStringList() << m_musicFiles[index]);
    m_isPlaying = true;
    
    if (m_playBtn) {
        m_playBtn->setText("||");
    }
    if (m_songLabel) {
        QFileInfo fi(m_musicFiles[index]);
        m_songLabel->setText(fi.baseName());
    }
    
    if (m_progressSlider) {
        m_progressSlider->setRange(0, m_songDuration);
        m_progressSlider->setValue(0);
    }
    updateTimeLabel();
    
    if (!m_progressTimer) {
        m_progressTimer = new QTimer(this);
        connect(m_progressTimer, &QTimer::timeout, this, [this]() {
            if (m_isPlaying && m_currentPosition < m_songDuration) {
                m_currentPosition++;
                if (m_progressSlider) {
                    m_progressSlider->setValue(m_currentPosition);
                }
                updateTimeLabel();
            }
        });
    }
    m_progressTimer->start(1000);
}

void DashboardPage::togglePlayPause()
{
    if (m_musicFiles.isEmpty()) {
        scanMusicDir();
        if (m_musicFiles.isEmpty()) return;
    }
    
    if (m_isPlaying) {
        stopMusic();
    } else {
        if (m_currentIndex < 0) {
            playMusic(0);
        } else {
            playMusic(m_currentIndex);
        }
    }
}

void DashboardPage::stopMusic()
{
    m_isPlaying = false;
    if (m_progressTimer) {
        m_progressTimer->stop();
    }
    if (m_musicProcess && m_musicProcess->state() == QProcess::Running) {
        m_musicProcess->terminate();
        m_musicProcess->waitForFinished(1000);
    }
    if (m_playBtn) {
        m_playBtn->setText(">");
    }
}

void DashboardPage::nextSong()
{
    playSound("./Sound_Efftcts/Next_track.mp3");
    
    if (m_musicFiles.isEmpty()) return;
    int next = (m_currentIndex + 1) % m_musicFiles.size();
    playMusic(next);
}

void DashboardPage::prevSong()
{
    playSound("./Sound_Efftcts/Previous_track.mp3");
    
    if (m_musicFiles.isEmpty()) return;
    int prev = (m_currentIndex - 1 + m_musicFiles.size()) % m_musicFiles.size();
    playMusic(prev);
}

void DashboardPage::updateTimeLabel()
{
    if (!m_timeLabel) return;
    
    auto formatTime = [](int seconds) -> QString {
        int min = seconds / 60;
        int sec = seconds % 60;
        return QString("%1:%2").arg(min).arg(sec, 2, 10, QChar('0'));
    };
    
    QString current = formatTime(m_currentPosition);
    QString total = formatTime(m_songDuration);
    m_timeLabel->setText(current + " / " + total);
}

void DashboardPage::seekToPosition(int position)
{
    if (position < 0 || position > m_songDuration) return;
    m_currentPosition = position;
    updateTimeLabel();
}

void DashboardPage::updateSystemStats()
{
    QFile statFile("/proc/stat");
    if (statFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray line = statFile.readLine();
        statFile.close();
        
        QList<QByteArray> parts = line.split(' ');
        if (parts.size() >= 5) {
            long long user = parts[1].toLongLong();
            long long nice = parts[2].toLongLong();
            long long system = parts[3].toLongLong();
            long long idle = parts[4].toLongLong();
            long long iowait = parts.size() > 5 ? parts[5].toLongLong() : 0;
            long long irq = parts.size() > 6 ? parts[6].toLongLong() : 0;
            long long softirq = parts.size() > 7 ? parts[7].toLongLong() : 0;
            
            long long total = user + nice + system + idle + iowait + irq + softirq;
            
            if (m_lastCpuTotal > 0) {
                long long totalDiff = total - m_lastCpuTotal;
                long long idleDiff = idle - m_lastCpuIdle;
                if (totalDiff > 0) {
                    int cpuUsage = 100 * (totalDiff - idleDiff) / totalDiff;
                    cpuUsage = qBound(0, cpuUsage, 100);
                    if (m_cpuBar) {
                        m_cpuBar->setValue(cpuUsage);
                    }
                }
            }
            
            m_lastCpuTotal = total;
            m_lastCpuIdle = idle;
        }
    }
    
    QFile memFile("/proc/meminfo");
    if (memFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        long long totalMem = 0;
        long long availableMem = 0;
        
        while (!memFile.atEnd()) {
            QByteArray line = memFile.readLine();
            if (line.startsWith("MemTotal:")) {
                totalMem = line.split(':')[1].trimmed().split(' ')[0].toLongLong();
            } else if (line.startsWith("MemAvailable:")) {
                availableMem = line.split(':')[1].trimmed().split(' ')[0].toLongLong();
            }
        }
        memFile.close();
        
        if (totalMem > 0) {
            int memUsage = 100 * (totalMem - availableMem) / totalMem;
            memUsage = qBound(0, memUsage, 100);
            if (m_memBar) {
                m_memBar->setValue(memUsage);
            }
        }
    }
    
    double maxTempC = 0;
    bool tempReadSuccess = false;
    
    for (int i = 0; i < 10; i++) {
        QFile tempFile("/sys/class/thermal/thermal_zone0/temp");
        if (tempFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray data = tempFile.readAll().trimmed();
            tempFile.close();
            
            int tempMilli = data.toInt();
            if (tempMilli > 0) {
                double tempC = tempMilli / 1000.0;
                if (tempC > maxTempC) {
                    maxTempC = tempC;
                }
                tempReadSuccess = true;
            }
        }
        if (i < 9) {
            QThread::msleep(1);
        }
    }
    
    if (tempReadSuccess && maxTempC > 0) {
        static double lastValidTemp = 0;
        if (lastValidTemp > 0 && qAbs(maxTempC - lastValidTemp) > 15) {
            maxTempC = lastValidTemp;
        } else {
            lastValidTemp = maxTempC;
        }
        
        if (m_cpuTempLabel) {
            m_cpuTempLabel->setText(QString::number(maxTempC, 'f', 1) + "°C");
            if (maxTempC > 70) {
                m_cpuTempLabel->setStyleSheet("color: #ef4444; font-size: 11px; font-weight: 700;");
            } else if (maxTempC > 50) {
                m_cpuTempLabel->setStyleSheet("color: #f59e0b; font-size: 11px; font-weight: 700;");
            } else {
                m_cpuTempLabel->setStyleSheet("color: #22c55e; font-size: 11px; font-weight: 700;");
            }
        }
    }
}
