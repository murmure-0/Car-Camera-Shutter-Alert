/**
 * @file mainwindow.cpp
 * @brief Main window implementation — layout, page routing, and theme coordination.
 */

#include "mainwindow.h"
#include "./ui_mainwindow.h"

/**
 * @brief MainWindow acts as the UI shell and page router.
 *
 * Builds the 800x480 design-resolution UI tree, applies uniform scaling
 * via QGraphicsView, manages sidebar navigation, page stack switching,
 * status bar, and coordinates theme/recording state.
 */

#include "pages/camerapage.h"
#include "pages/connectivitypage.h"
#include "pages/dashboardpage.h"
#include "pages/environmentpage.h"
#include "pages/gpspage.h"
#include "pages/motionpage.h"
#include "pages/settingspage.h"

#include <QButtonGroup>
#include <QFrame>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QNetworkInterface>
#include <QMainWindow>
#include <QProgressBar>
#include <QResizeEvent>
#include <QSerialPort>
#include <QStackedWidget>
#include <QStyle>
#include <QTime>
#include <QTimer>
#include <QToolButton>
#include <QVariant>
#include <QtMath>
#include <QVBoxLayout>
#include <limits>

namespace {
static QToolButton *makeNavButton(const QString &text, QWidget *parent)
{
    auto *b = new QToolButton(parent);
    b->setText(text);
    b->setCheckable(true);
    b->setAutoRaise(true);
    b->setProperty("nav", true);
    b->setFixedSize(40, 40);
    b->setToolButtonStyle(Qt::ToolButtonTextOnly);
    return b;
}
} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("Car UI");

    if (menuBar()) {
        menuBar()->hide();
    }
    if (QMainWindow::statusBar()) {
        QMainWindow::statusBar()->hide();
    }
    if (ui->mainToolBar) {
        ui->mainToolBar->hide();
    }

    resize(800, 480);

    m_view = new QGraphicsView(this);
    m_scene = new QGraphicsScene(this);
    m_view->setScene(m_scene);
    m_view->setFrameStyle(QFrame::NoFrame);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setAlignment(Qt::AlignCenter);
    m_view->setBackgroundBrush(QColor("#000000"));
    m_view->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    m_root = new QWidget;
    m_root->setObjectName("root");
    m_root->setFixedSize(800, 480);

    m_proxy = m_scene->addWidget(m_root);
    m_proxy->setPos(0, 0);
    m_scene->setSceneRect(0, 0, 800, 480);

    auto *outer = new QVBoxLayout(ui->centralWidget);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);
    outer->addWidget(m_view);

    auto *rootCol = new QVBoxLayout(m_root);
    rootCol->setContentsMargins(0, 0, 0, 0);
    rootCol->setSpacing(0);

    m_statusBar = new QFrame(m_root);
    m_statusBar->setObjectName("statusBar");
    m_statusBar->setFixedHeight(32);
    auto *statusRow = new QHBoxLayout(m_statusBar);
    statusRow->setContentsMargins(12, 0, 12, 0);
    statusRow->setSpacing(8);

    auto *leftWrap = new QWidget(m_statusBar);
    auto *leftRow = new QHBoxLayout(leftWrap);
    leftRow->setContentsMargins(0, 0, 0, 0);
    leftRow->setSpacing(8);
    m_statusLeftIcon = new QLabel(leftWrap);
    m_statusLeftIcon->setFixedSize(10, 10);
    m_statusLeftIcon->setObjectName("statusLeftIcon");
    m_statusLeftText = new QLabel(leftWrap);
    m_statusLeftText->setObjectName("statusLeftText");
    leftRow->addWidget(m_statusLeftIcon);
    leftRow->addWidget(m_statusLeftText);

    m_clockLabel = new QLabel(m_statusBar);
    m_clockLabel->setObjectName("clockLabel");
    m_clockLabel->setAlignment(Qt::AlignCenter);

    auto *rightWrap = new QWidget(m_statusBar);
    auto *rightRow = new QHBoxLayout(rightWrap);
    rightRow->setContentsMargins(0, 0, 0, 0);
    rightRow->setSpacing(10);
    m_statusRightText = new QLabel(rightWrap);
    m_statusRightText->setObjectName("statusRightText");
    m_battery = new QProgressBar(rightWrap);
    static_cast<QProgressBar *>(m_battery)->setRange(0, 100);
    static_cast<QProgressBar *>(m_battery)->setValue(75);
    static_cast<QProgressBar *>(m_battery)->setTextVisible(false);
    static_cast<QProgressBar *>(m_battery)->setFixedSize(46, 10);
    rightRow->addWidget(m_statusRightText);
    rightRow->addWidget(m_battery);

    statusRow->addWidget(leftWrap, 1, Qt::AlignLeft);
    statusRow->addWidget(m_clockLabel, 1);
    statusRow->addWidget(rightWrap, 1, Qt::AlignRight);

    rootCol->addWidget(m_statusBar);
    auto *contentRow = new QHBoxLayout;
    contentRow->setContentsMargins(0, 0, 0, 0);
    contentRow->setSpacing(0);

    m_sideBar = new QFrame(m_root);
    m_sideBar->setObjectName("sideBar");
    m_sideBar->setFixedWidth(64);
    auto *sideCol = new QVBoxLayout(m_sideBar);
    sideCol->setContentsMargins(0, 12, 0, 12);
    sideCol->setSpacing(14);
    sideCol->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    m_navGroup = new QButtonGroup(this);
    m_navGroup->setExclusive(true);

    m_btnHome = makeNavButton("H", m_sideBar);
    m_btnGps = makeNavButton("G", m_sideBar);
    m_btnEnv = makeNavButton("E", m_sideBar);
    m_btnCamera = makeNavButton("C", m_sideBar);
    m_btnMotion = makeNavButton("M", m_sideBar);
    m_btnNet = makeNavButton("N", m_sideBar);
    m_btnSettings = makeNavButton("S", m_sideBar);

    m_btnCamera->setProperty("cameraAccent", true);

    m_navGroup->addButton(m_btnHome, int(PageId::Dashboard));
    m_navGroup->addButton(m_btnGps, int(PageId::Gps));
    m_navGroup->addButton(m_btnEnv, int(PageId::Environment));
    m_navGroup->addButton(m_btnCamera, int(PageId::Camera));
    m_navGroup->addButton(m_btnMotion, int(PageId::Motion));
    m_navGroup->addButton(m_btnNet, int(PageId::Connectivity));
    m_navGroup->addButton(m_btnSettings, int(PageId::Settings));

    sideCol->addWidget(m_btnHome, 0, Qt::AlignHCenter);
    sideCol->addWidget(m_btnGps, 0, Qt::AlignHCenter);
    sideCol->addWidget(m_btnEnv, 0, Qt::AlignHCenter);
    sideCol->addWidget(m_btnCamera, 0, Qt::AlignHCenter);
    sideCol->addWidget(m_btnMotion, 0, Qt::AlignHCenter);
    sideCol->addWidget(m_btnNet, 0, Qt::AlignHCenter);
    sideCol->addStretch(1);
    sideCol->addWidget(m_btnSettings, 0, Qt::AlignHCenter);

    m_stack = new QStackedWidget(m_root);
    m_stack->setObjectName("stack");

    m_dashboardPage = new DashboardPage(m_stack);
    m_gpsPage = new GpsPage(m_stack);
    m_environmentPage = new EnvironmentPage(m_stack);
    m_cameraPage = new CameraPage(m_stack);
    m_motionPage = new MotionPage(m_stack);
    auto *net = new ConnectivityPage(m_stack);
    m_settingsPage = new SettingsPage(m_stack);

    m_stack->addWidget(m_dashboardPage);
    m_stack->addWidget(m_gpsPage);
    m_stack->addWidget(m_environmentPage);
    m_stack->addWidget(m_cameraPage);
    m_stack->addWidget(m_motionPage);
    m_stack->addWidget(net);
    m_stack->addWidget(m_settingsPage);

    connect(m_dashboardPage, &DashboardPage::requestGps, this, [this]() { setPage(PageId::Gps); });
    connect(m_dashboardPage, &DashboardPage::requestEnvironment, this, [this]() { setPage(PageId::Environment); });
    connect(m_cameraPage, &CameraPage::toggleRecordingRequested, this, [this]() { setRecording(!m_recording); });
    
    connect(m_cameraPage, &CameraPage::gestureDetected, this, [this](const QString &gesture) {
        if (!m_dashboardPage) return;
        
        QString action = "none";
        
        if (gesture == "Fist") {
            action = "pause";
            if (m_dashboardPage->isPlaying()) {
                m_dashboardPage->togglePlayPause();
            }
        } else if (gesture == "Five") {
            action = "play";
            if (!m_dashboardPage->isPlaying()) {
                m_dashboardPage->togglePlayPause();
            }
        } else if (gesture == "One") {
            action = "prev";
            m_dashboardPage->prevSong();
        } else if (gesture == "Two") {
            action = "next";
            m_dashboardPage->nextSong();
        }
        
        QJsonObject gestureData;
        gestureData["gesture"] = gesture;
        gestureData["action"] = action;
        
        QJsonObject root;
        root["type"] = "gesture";
        root["data"] = gestureData;
        
        sendSerialData(root);
    });
    
    connect(m_cameraPage, &CameraPage::fatigueDetected, this, [this](bool isTired) {
        if (!m_dashboardPage) return;
        m_dashboardPage->setFatigueWarning(isTired);
        
        if (isTired != m_lastFatigueState) {
            m_lastFatigueState = isTired;
            
            QJsonObject fatigueData;
            fatigueData["is_tired"] = isTired;
            
            QJsonObject root;
            root["type"] = "fatigue";
            root["data"] = fatigueData;
            
            sendSerialData(root);
        }
    });
    connect(m_settingsPage, &SettingsPage::darkModeToggled, this, [this](bool on) {
        m_darkMode = on;
        applyOverlayTheme(m_currentPage);
    });

    contentRow->addWidget(m_sideBar);
    contentRow->addWidget(m_stack, 1);
    rootCol->addLayout(contentRow, 1);

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(m_navGroup, &QButtonGroup::idClicked, this, [this](int id) { setPage(static_cast<PageId>(id)); });
#else
    connect(m_navGroup,
            static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked),
            this,
            [this](int id) { setPage(static_cast<PageId>(id)); });
#endif

    m_clockTimer = new QTimer(this);
    m_clockTimer->setInterval(1000);
    connect(m_clockTimer, &QTimer::timeout, this, &MainWindow::updateClock);
    m_clockTimer->start();
    updateClock();

    m_recTimer = new QTimer(this);
    m_recTimer->setInterval(1000);
    connect(m_recTimer, &QTimer::timeout, this, &MainWindow::updateRecText);

    m_dotTimer = new QTimer(this);
    m_dotTimer->setInterval(450);
    connect(m_dotTimer, &QTimer::timeout, this, &MainWindow::updateRecDot);

    m_networkCheckTimer = new QTimer(this);
    m_networkCheckTimer->setInterval(5000);
    connect(m_networkCheckTimer, &QTimer::timeout, this, [this]() {
        bool isConnected = false;
        QString connectionType = "none";
        
        QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
        for (const QNetworkInterface &iface : interfaces) {
            if (iface.flags() & QNetworkInterface::IsLoopBack) continue;
            if (!(iface.flags() & QNetworkInterface::IsUp)) continue;
            if (!(iface.flags() & QNetworkInterface::IsRunning)) continue;
            
            QList<QNetworkAddressEntry> entries = iface.addressEntries();
            for (const QNetworkAddressEntry &entry : entries) {
                QHostAddress ip = entry.ip();
                if (ip.protocol() == QAbstractSocket::IPv4Protocol && 
                    !ip.isLoopback() && 
                    !ip.isLinkLocal()) {
                    isConnected = true;
                    
                    QString name = iface.name().toLower();
                    if (name.contains("wlan") || name.contains("wifi") || name.contains("wl")) {
                        connectionType = "wifi";
                    } else if (name.contains("eth") || name.contains("en")) {
                        connectionType = "ethernet";
                    } else if (name.contains("ppp") || name.contains("rmnet") || name.contains("usb")) {
                        connectionType = "mobile";
                    } else {
                        connectionType = "other";
                    }
                    break;
                }
            }
            if (isConnected) break;
        }
        
        if (isConnected != m_lastNetworkConnected) {
            m_lastNetworkConnected = isConnected;
            
            QJsonObject networkData;
            networkData["connected"] = isConnected;
            networkData["type"] = connectionType;
            
            QJsonObject root;
            root["type"] = "network";
            root["data"] = networkData;
            
            sendSerialData(root);
        }
    });
    m_networkCheckTimer->start();
    QMetaObject::invokeMethod(m_networkCheckTimer, "timeout", Qt::QueuedConnection);

    setPage(PageId::Dashboard);

    QTimer::singleShot(0, this, [this]() { updateViewScale(); });
    setupSerial();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    updateViewScale();
}

void MainWindow::updateViewScale()
{
    if (!m_view || !m_proxy) {
        return;
    }

    const QSizeF viewSize = m_view->viewport()->size();
    if (viewSize.width() < 1.0 || viewSize.height() < 1.0) {
        return;
    }

    const double sx = viewSize.width() / 800.0;
    const double sy = viewSize.height() / 480.0;
    const double s = qMax(0.1, qMin(sx, sy));

    QTransform t;
    t.scale(s, s);
    m_view->setTransform(t);
    m_view->centerOn(m_proxy);
}

void MainWindow::setupSerial()
{
    if (!m_serialRx) {
        m_serialRx = new QSerialPort(this);
        m_serialRx->setPortName(QStringLiteral("ttyS1"));
        m_serialRx->setBaudRate(QSerialPort::Baud115200);
        m_serialRx->setDataBits(QSerialPort::Data8);
        m_serialRx->setParity(QSerialPort::NoParity);
        m_serialRx->setStopBits(QSerialPort::OneStop);
        m_serialRx->setFlowControl(QSerialPort::NoFlowControl);
        if (!m_serialRx->open(QIODevice::ReadOnly)) {
            qDebug() << "Failed to open ttyS1 for reading:" << m_serialRx->errorString();
        } else {
            qDebug() << "Successfully opened ttyS1 for reading";
            connect(m_serialRx, &QSerialPort::readyRead, this, &MainWindow::handleSerialReadyRead);
        }
    }
    
    if (!m_serialTx) {
        m_serialTx = new QSerialPort(this);
        m_serialTx->setPortName(QStringLiteral("ttyS0"));
        m_serialTx->setBaudRate(QSerialPort::Baud115200);
        m_serialTx->setDataBits(QSerialPort::Data8);
        m_serialTx->setParity(QSerialPort::NoParity);
        m_serialTx->setStopBits(QSerialPort::OneStop);
        m_serialTx->setFlowControl(QSerialPort::NoFlowControl);
        if (!m_serialTx->open(QIODevice::WriteOnly)) {
            qDebug() << "Failed to open ttyS0 for writing:" << m_serialTx->errorString();
        } else {
            qDebug() << "Successfully opened ttyS0 for writing";
        }
    }
}

void MainWindow::handleSerialReadyRead()
{
    if (!m_serialRx) {
        return;
    }
    m_serialBuffer.append(m_serialRx->readAll());
    while (true) {
        const int newlineIndex = m_serialBuffer.indexOf('\n');
        if (newlineIndex < 0) {
            break;
        }
        const QByteArray line = m_serialBuffer.left(newlineIndex).trimmed();
        m_serialBuffer.remove(0, newlineIndex + 1);
        if (!line.isEmpty()) {
            processSensorLine(line);
        }
    }
}

void MainWindow::processSensorLine(const QByteArray &line)
{
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(line, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return;
    }
    const QJsonObject root = doc.object();
    const bool hasTemp = root.contains("temp");
    const bool hasHumi = root.contains("humi");
    if (hasTemp || hasHumi) {
        const double temp = root.value("temp").toDouble();
        const double humi = root.value("humi").toDouble();
        if (m_environmentPage) {
            m_environmentPage->updateEnvironment(temp, humi);
        }
        if (m_dashboardPage) {
            m_dashboardPage->updateEnvironmentSummary(temp, humi);
        }
    }

    if (root.contains("mpu") && root.value("mpu").isObject()) {
        const QJsonObject mpu = root.value("mpu").toObject();
        const double ax = mpu.value("ax").toDouble();
        const double ay = mpu.value("ay").toDouble();
        const double az = mpu.value("az").toDouble();
        const double gx = mpu.value("gx").toDouble();
        const double gy = mpu.value("gy").toDouble();
        const double gz = mpu.value("gz").toDouble();
        double pitch = std::numeric_limits<double>::quiet_NaN();
        double roll = std::numeric_limits<double>::quiet_NaN();
        double yaw = std::numeric_limits<double>::quiet_NaN();
        if (mpu.contains("ang") && mpu.value("ang").isObject()) {
            const QJsonObject ang = mpu.value("ang").toObject();
            pitch = ang.value("pitch").toDouble(pitch);
            roll = ang.value("roll").toDouble(roll);
            yaw = ang.value("yaw").toDouble(yaw);
        }
        if (m_motionPage) {
            m_motionPage->updateMotion(ax, ay, az, gx, gy, gz, pitch, roll, yaw);
        }
    }

    if (root.contains("gps") && root.value("gps").isObject()) {
        const QJsonObject gps = root.value("gps").toObject();
        const double lat = gps.value("lat").toDouble();
        const double lon = gps.value("lon").toDouble();
        const int sat = gps.value("sat").toInt(-1);
        if (m_gpsPage) {
            m_gpsPage->updateGps(lat, lon, sat);
        }
        if (m_dashboardPage) {
            m_dashboardPage->updateGpsSummary(lat, lon, sat);
        }
    }

    if (root.contains("adc") && root.value("adc").isObject()) {
        const QJsonObject adc = root.value("adc").toObject();
        if (adc.contains("bat") && m_battery) {
            const double batV = adc.value("bat").toDouble();
            const double pct = (batV - 3.0) / 1.2 * 100.0;
            const int value = qBound(0, int(qRound(pct)), 100);
            static_cast<QProgressBar *>(m_battery)->setValue(value);
        }
    }
}

void MainWindow::sendSerialData(const QJsonObject &jsonData)
{
    if (!m_serialTx || !m_serialTx->isOpen()) {
        return;
    }
    
    QJsonDocument doc(jsonData);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    data.append('\n');
    
    m_serialTx->write(data);
    m_serialTx->flush();
}

void MainWindow::setPage(PageId page)
{
    m_currentPage = page;
    if (m_stack) {
        m_stack->setCurrentIndex(int(page));
    }

    if (m_navGroup) {
        if (auto *btn = m_navGroup->button(int(page))) {
            btn->setChecked(true);
        }
    }

    setRecording(page == PageId::Camera);
    if (m_cameraPage) {
        m_cameraPage->setActive(page == PageId::Camera);
    }
    applyOverlayTheme(page);
    updateRecText();
    updateRecDot();
}

void MainWindow::applyOverlayTheme(PageId page)
{
    const bool overlay = (page == PageId::Camera);
    m_root->setProperty("overlay", overlay);

    QString leftText;
    QString rightText;
    switch (page) {
    case PageId::Dashboard:
        leftText = "4G LTE";
        rightText = "WiFi  BT";
        break;
    case PageId::Gps:
        leftText = "GPS: 3D Fix (8 sats)";
        rightText = "WiFi  BT";
        break;
    case PageId::Environment:
        leftText = "Env sensors active";
        rightText = "WiFi  BT";
        break;
    case PageId::Camera:
        rightText = "1080p 60fps";
        break;
    case PageId::Motion:
        leftText = "MPU-6050 IMU";
        rightText = "WiFi  BT";
        break;
    case PageId::Connectivity:
        leftText = "Network";
        rightText = "WiFi  BT";
        break;
    case PageId::Settings:
        leftText = "Settings";
        rightText = "WiFi  BT";
        break;
    }

    if (page != PageId::Camera) {
        m_statusLeftText->setText(leftText);
    }
    m_statusRightText->setText(rightText);

    const QString baseBg = m_darkMode ? "#0b1220" : "#111827";
    const QString baseBg2 = m_darkMode ? "#1f2937" : "#0b1220";

    const QString qss = QString(R"(
QWidget#root {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 %2);
}
QWidget#root[overlay="true"] {
    background: #000000;
}

QFrame#statusBar {
    background: %1;
    border-bottom: 1px solid rgba(255,255,255,0.06);
}
QWidget#root[overlay="true"] QFrame#statusBar {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 rgba(0,0,0,200), stop:1 rgba(0,0,0,0));
    border-bottom: 0px;
}

QLabel#statusLeftText, QLabel#statusRightText {
    color: rgba(226,232,240,0.70);
    font-size: 11px;
}
QLabel#clockLabel {
    color: rgba(255,255,255,0.92);
    font-weight: 700;
    font-size: 12px;
}
QLabel#statusLeftIcon {
    background: rgba(255,255,255,0.22);
    border-radius: 5px;
}
QWidget#root[overlay="true"] QLabel#statusLeftIcon {
    background: rgba(239,68,68,0.95);
}

QFrame#sideBar {
    background: #1f2937;
    border-right: 1px solid rgba(255,255,255,0.06);
}
QWidget#root[overlay="true"] QFrame#sideBar {
    background: rgba(0,0,0,110);
    border-right: 1px solid rgba(255,255,255,0.10);
}

QToolButton[nav="true"] {
    color: rgba(226,232,240,0.55);
    background: transparent;
    border-radius: 12px;
    font-weight: 700;
    font-size: 12px;
}
QToolButton[nav="true"]:hover {
    background: rgba(255,255,255,0.10);
    color: rgba(255,255,255,0.92);
}
QToolButton[nav="true"]:checked {
    background: #0891b2;
    color: rgba(255,255,255,0.95);
}
QToolButton[cameraAccent="true"]:checked {
    background: rgba(220,38,38,0.85);
}

QWidget:focus {
    outline: none;
}
QPushButton:focus {
    border: 0px;
    outline: none;
}
QToolButton:focus {
    border: 0px;
    outline: none;
}
QLineEdit:focus {
    border: 0px;
    outline: none;
}
QCheckBox:focus {
    outline: none;
}

QProgressBar {
    border: 1px solid rgba(255,255,255,0.10);
    background: rgba(255,255,255,0.08);
    border-radius: 5px;
}
QProgressBar::chunk {
    background: #22c55e;
    border-radius: 5px;
}

QWidget[glass="true"] {
    background: rgba(31, 41, 55, 0.72);
    border: 1px solid rgba(255,255,255,0.10);
    border-radius: 16px;
}
QWidget#root[overlay="true"] QWidget[glass="true"] {
    background: rgba(0, 0, 0, 0.45);
    border: 1px solid rgba(255,255,255,0.10);
}

QPushButton[card="true"] {
    color: rgba(255,255,255,0.92);
    border: 0px;
    text-align: left;
}

QLineEdit {
    background: transparent;
    border: 0px;
    color: rgba(255,255,255,0.92);
    font-size: 12px;
}

QSlider::groove:horizontal {
    height: 6px;
    background: rgba(255,255,255,0.16);
    border-radius: 3px;
}
QSlider::handle:horizontal {
    width: 14px;
    margin: -5px 0;
    border-radius: 7px;
    background: rgba(255,255,255,0.90);
}

QCheckBox {
    color: rgba(255,255,255,0.88);
    font-size: 12px;
}
QCheckBox::indicator {
    width: 44px;
    height: 22px;
    border-radius: 11px;
    background: rgba(255,255,255,0.14);
    border: 1px solid rgba(255,255,255,0.10);
}
QCheckBox::indicator:checked {
    background: #0891b2;
}

QListWidget {
    background: transparent;
    border: 0px;
    color: rgba(255,255,255,0.88);
    font-size: 12px;
}
QListWidget::item {
    padding: 8px 10px;
    border-radius: 12px;
}
QListWidget::item:selected {
    background: rgba(8,145,178,0.22);
}
)").arg(baseBg, baseBg2);

    bool needsPolish = false;
    if (qss != m_lastQss) {
        m_root->setStyleSheet(qss);
        m_lastQss = qss;
        needsPolish = true;
    }
    if (overlay != m_lastOverlay) {
        m_lastOverlay = overlay;
        needsPolish = true;
    }
    if (needsPolish) {
        m_root->style()->unpolish(m_root);
        m_root->style()->polish(m_root);
    }
}

void MainWindow::updateClock()
{
    m_clockLabel->setText(QTime::currentTime().toString("HH:mm"));
}

void MainWindow::setRecording(bool recording)
{
    if (m_recording == recording) {
        return;
    }
    m_recording = recording;

    if (m_cameraPage) {
        m_cameraPage->setRecording(m_recording);
    }

    if (m_recording) {
        if (!m_recTimer->isActive()) {
            m_recTimer->start();
        }
        if (!m_dotTimer->isActive()) {
            m_dotPhaseTimer.restart();
            m_dotTimer->start();
        }
    } else {
        m_recTimer->stop();
        m_dotTimer->stop();
    }
}

void MainWindow::updateRecText()
{
    if (m_currentPage != PageId::Camera) {
        return;
    }

    const int mm = m_recSeconds / 60;
    const int ss = m_recSeconds % 60;
    m_statusLeftText->setText(QString("REC %1:%2").arg(mm, 2, 10, QChar('0')).arg(ss, 2, 10, QChar('0')));
    if (m_recording) {
        ++m_recSeconds;
    }
}


void MainWindow::updateRecDot()
{
    if (m_currentPage != PageId::Camera) {
        m_statusLeftIcon->setStyleSheet(QString());
        return;
    }
    const bool on = (m_dotPhaseTimer.isValid() && (m_dotPhaseTimer.elapsed() / 450) % 2 == 0);
    if (on) {
        m_statusLeftIcon->setStyleSheet(QString());
    } else {
        m_statusLeftIcon->setStyleSheet("background: rgba(239,68,68,0.20); border-radius: 5px;");
    }
}
