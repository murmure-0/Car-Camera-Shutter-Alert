/**
 * @file camerahud.cpp
 * @brief Standalone camera HUD demo window implementation.
 */

#include "camerahud.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QSlider>
#include <QStyle>
#include <QTimer>
#include <QVariant>
#include <QWidget>

#include <QtMath>

namespace {

class RuleOfThirdsGrid final : public QWidget
{
public:
    explicit RuleOfThirdsGrid(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, false);
        p.setPen(QPen(QColor(255, 255, 255, 76), 1.0));

        const int w = width();
        const int h = height();
        const int x1 = w / 3;
        const int x2 = (w * 2) / 3;
        const int y1 = h / 3;
        const int y2 = (h * 2) / 3;

        p.drawLine(QPointF(x1, 0), QPointF(x1, h));
        p.drawLine(QPointF(x2, 0), QPointF(x2, h));
        p.drawLine(QPointF(0, y1), QPointF(w, y1));
        p.drawLine(QPointF(0, y2), QPointF(w, y2));
    }
};

static QString formatHms(int totalSeconds)
{
    totalSeconds = qMax(0, totalSeconds);
    const int h = totalSeconds / 3600;
    const int m = (totalSeconds % 3600) / 60;
    const int s = totalSeconds % 60;
    return QString("%1:%2:%3")
        .arg(h, 2, 10, QLatin1Char('0'))
        .arg(m, 2, 10, QLatin1Char('0'))
        .arg(s, 2, 10, QLatin1Char('0'));
}

static QPushButton *makeNavButton(const QString &text, QWidget *parent)
{
    auto *b = new QPushButton(text, parent);
    b->setCursor(Qt::PointingHandCursor);
    b->setFocusPolicy(Qt::NoFocus);
    b->setCheckable(true);
    b->setAutoExclusive(true);
    b->setFixedSize(44, 44);
    b->setProperty("nav", true);
    b->setProperty("active", false);
    return b;
}

static QWidget *makeBatteryWidget(QWidget *parent)
{
    auto *battery = new QWidget(parent);
    battery->setFixedSize(34, 14);

    auto *body = new QFrame(battery);
    body->setObjectName("batteryBody");
    body->setGeometry(0, 0, 30, 14);

    auto *level = new QFrame(body);
    level->setObjectName("batteryLevel");
    level->setGeometry(2, 2, 20, 10);

    auto *cap = new QFrame(battery);
    cap->setObjectName("batteryCap");
    cap->setGeometry(30, 4, 4, 6);

    return battery;
}

static QWidget *makeShutterInner(QWidget *parent)
{
    auto *w = new QWidget(parent);
    w->setObjectName("shutterInner");
    w->setFixedSize(38, 38);
    return w;
}

} // namespace

CameraHud::CameraHud(QWidget *parent)
    : QMainWindow(parent)
{
    setFixedSize(800, 480);
    setWindowTitle("Camera HUD");

    auto *root = new QWidget(this);
    root->setObjectName("root");
    setCentralWidget(root);
    root->setGeometry(0, 0, 800, 480);

    auto *bg = new QLabel(root);
    bg->setObjectName("bg");
    bg->setGeometry(0, 0, 800, 480);
    bg->setAlignment(Qt::AlignCenter);
    bg->setText("CAMERA PREVIEW");

    auto *grid = new RuleOfThirdsGrid(root);
    grid->setGeometry(0, 0, 800, 480);

    auto *topBar = new QWidget(root);
    topBar->setObjectName("topBar");
    topBar->setGeometry(0, 0, 800, 40);

    m_recDot = new QLabel(topBar);
    m_recDot->setObjectName("recDot");
    m_recDot->setGeometry(16, 12, 10, 10);

    m_recLabel = new QLabel(topBar);
    m_recLabel->setObjectName("recLabel");
    m_recLabel->setGeometry(32, 9, 200, 22);
    m_recLabel->setText("REC 00:00:00");

    auto *qualityLabel = new QLabel(topBar);
    qualityLabel->setObjectName("qualityLabel");
    qualityLabel->setGeometry(800 - 190, 9, 110, 22);
    qualityLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    qualityLabel->setText("1080p 60fps");

    auto *battery = makeBatteryWidget(topBar);
    battery->setObjectName("battery");
    battery->setGeometry(800 - 68, 13, 34, 14);

    auto *leftBar = new QWidget(root);
    leftBar->setObjectName("leftBar");
    leftBar->setGeometry(0, 0, 64, 480);

    auto *navGroup = new QButtonGroup(leftBar);
    navGroup->setExclusive(true);

    auto *btnHome = makeNavButton("H", leftBar);
    auto *btnMap = makeNavButton("M", leftBar);
    auto *btnTemp = makeNavButton("T", leftBar);
    auto *btnCamera = makeNavButton("C", leftBar);
    auto *btnMotion = makeNavButton("MO", leftBar);
    auto *btnNet = makeNavButton("N", leftBar);
    auto *btnSettings = makeNavButton("S", leftBar);

    btnHome->setProperty("navRole", "home");
    btnMap->setProperty("navRole", "map");
    btnTemp->setProperty("navRole", "temp");
    btnCamera->setProperty("navRole", "camera");
    btnMotion->setProperty("navRole", "motion");
    btnNet->setProperty("navRole", "network");
    btnSettings->setProperty("navRole", "settings");

    btnHome->setGeometry(10, 64, 44, 44);
    btnMap->setGeometry(10, 118, 44, 44);
    btnTemp->setGeometry(10, 172, 44, 44);
    btnCamera->setGeometry(10, 226, 44, 44);
    btnMotion->setGeometry(10, 280, 44, 44);
    btnNet->setGeometry(10, 334, 44, 44);
    btnSettings->setGeometry(10, 480 - 10 - 44, 44, 44);

    navGroup->addButton(btnHome);
    navGroup->addButton(btnMap);
    navGroup->addButton(btnTemp);
    navGroup->addButton(btnCamera);
    navGroup->addButton(btnMotion);
    navGroup->addButton(btnNet);
    navGroup->addButton(btnSettings);
    btnCamera->setChecked(true);
    btnCamera->setProperty("active", true);
    for (auto *btn : navGroup->buttons()) {
        btn->style()->unpolish(btn);
        btn->style()->polish(btn);
    }

    connect(navGroup,
            static_cast<void (QButtonGroup::*)(QAbstractButton *, bool)>(&QButtonGroup::buttonToggled),
            this,
            [=](QAbstractButton *b, bool on) {
        if (!on) {
            b->setProperty("active", false);
            b->style()->unpolish(b);
            b->style()->polish(b);
            return;
        }

        for (auto *btn : navGroup->buttons()) {
            btn->setProperty("active", btn == b);
            btn->style()->unpolish(btn);
            btn->style()->polish(btn);
        }
    });

    auto *rightBar = new QWidget(root);
    rightBar->setObjectName("rightBar");
    rightBar->setGeometry(800 - 100, 0, 100, 480);

    auto *modeGroup = new QButtonGroup(rightBar);
    modeGroup->setExclusive(true);

    m_modeVideo = new QPushButton("VIDEO", rightBar);
    m_modePhoto = new QPushButton("PHOTO", rightBar);
    m_modeSlow = new QPushButton("SLOW", rightBar);

    for (auto *b : {m_modeVideo, m_modePhoto, m_modeSlow}) {
        b->setFocusPolicy(Qt::NoFocus);
        b->setCursor(Qt::PointingHandCursor);
        b->setCheckable(true);
        b->setFlat(true);
        b->setFixedHeight(22);
        b->setProperty("mode", true);
        modeGroup->addButton(b);
    }

    m_modeVideo->setProperty("modeRole", "video");
    m_modePhoto->setProperty("modeRole", "photo");
    m_modeSlow->setProperty("modeRole", "slow");

    m_modeVideo->setGeometry(10, 18, 80, 22);
    m_modePhoto->setGeometry(10, 44, 80, 22);
    m_modeSlow->setGeometry(10, 70, 80, 22);
    m_modeVideo->setChecked(true);

    m_shutterButton = new QPushButton(rightBar);
    m_shutterButton->setObjectName("shutterButton");
    m_shutterButton->setFocusPolicy(Qt::NoFocus);
    m_shutterButton->setCursor(Qt::PointingHandCursor);
    m_shutterButton->setGeometry(18, 208, 64, 64);

    m_shutterInner = makeShutterInner(m_shutterButton);
    m_shutterInner->move((64 - m_shutterInner->width()) / 2, (64 - m_shutterInner->height()) / 2);

    connect(m_shutterButton, &QPushButton::clicked, this, [this] {
        setRecording(!m_recording);
    });

    auto *thumb = new QLabel(rightBar);
    thumb->setObjectName("thumb");
    thumb->setGeometry(14, 480 - 14 - 54, 72, 54);
    thumb->setText("");

    auto *zoomPill = new QWidget(root);
    zoomPill->setObjectName("zoomPill");
    zoomPill->setGeometry((800 - 360) / 2, 480 - 18 - 42, 360, 42);

    auto *zoomLayout = new QHBoxLayout(zoomPill);
    zoomLayout->setContentsMargins(16, 8, 16, 8);
    zoomLayout->setSpacing(12);

    auto *zoomMin = new QLabel("1x", zoomPill);
    zoomMin->setObjectName("zoomLabel");
    auto *zoomMax = new QLabel("5x", zoomPill);
    zoomMax->setObjectName("zoomLabel");

    m_zoomSlider = new QSlider(Qt::Horizontal, zoomPill);
    m_zoomSlider->setObjectName("zoomSlider");
    m_zoomSlider->setRange(0, 100);
    m_zoomSlider->setValue(0);
    m_zoomSlider->setFixedHeight(18);

    zoomLayout->addWidget(zoomMin);
    zoomLayout->addWidget(m_zoomSlider, 1);
    zoomLayout->addWidget(zoomMax);

    root->setStyleSheet(QStringLiteral(R"QSS(
QWidget#root {
    background: #0B0F15;
}

QLabel#bg {
    color: rgba(255,255,255,140);
    font-size: 18px;
    letter-spacing: 2px;
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
        stop:0 #1a2433,
        stop:0.45 #0f1724,
        stop:1 #06080d);
}

QWidget#topBar {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
        stop:0 rgba(0,0,0,204),
        stop:1 rgba(0,0,0,0));
}

QLabel#recDot {
    border-radius: 5px;
    background: rgba(255, 40, 40, 220);
}

QLabel#recLabel, QLabel#qualityLabel {
    color: rgba(255,255,255,220);
    font-size: 12px;
    font-weight: 600;
}

QWidget#batteryBody {
    border: 1px solid rgba(255,255,255,170);
    border-radius: 3px;
    background: rgba(0,0,0,60);
}

QWidget#batteryLevel {
    border-radius: 2px;
    background: rgba(90, 255, 140, 210);
}

QWidget#batteryCap {
    border: 1px solid rgba(255,255,255,170);
    border-left: 0px;
    border-radius: 1px;
    background: rgba(255,255,255,70);
}

QWidget#leftBar {
    background: rgba(0,0,0,100);
}

QWidget#rightBar {
    background: rgba(0,0,0,110);
}

QPushButton {
    font-family: "Segoe UI";
}

QPushButton[nav="true"][active="true"] {
    background: rgba(255, 50, 60, 210);
    border: 1px solid rgba(255, 110, 110, 230);
    color: rgba(255,255,255,235);
}

QPushButton[nav="true"][active="false"] {
    background: rgba(255,255,255,18);
    border: 1px solid rgba(255,255,255,35);
    color: rgba(255,255,255,210);
}

QPushButton[nav="true"][active="false"]:pressed {
    background: rgba(255,255,255,26);
}

QPushButton#shutterButton {
    border-radius: 32px;
    border: 4px solid rgba(255,255,255,235);
    background: rgba(0,0,0,0);
}

QWidget#shutterInner {
    border-radius: 19px;
    background: rgba(255, 30, 30, 230);
}

QPushButton#shutterButton[recording="false"] QWidget#shutterInner {
    background: rgba(255, 30, 30, 0);
}

QPushButton#shutterButton[recording="false"] {
    border: 4px solid rgba(255,255,255,220);
}

QPushButton#shutterButton[recording="true"] {
    border: 4px solid rgba(255,255,255,250);
}

QPushButton#shutterButton:pressed {
    background: rgba(255,255,255,25);
}

QPushButton#shutterButton QWidget#shutterInner {
    background: rgba(255, 30, 30, 230);
}

QPushButton#shutterButton[recording="false"] QWidget#shutterInner {
    background: rgba(255, 30, 30, 0);
}

QPushButton {
    border-radius: 10px;
}

QPushButton[navRole="motion"] {
    font-size: 10px;
}

QPushButton[navRole="settings"] {
    font-size: 12px;
}

QPushButton[nav="true"] {
    font-size: 13px;
    font-weight: 700;
}

QPushButton#shutterButton {
    font-size: 0px;
}

QPushButton[mode="true"] {
    border: 0px;
    background: rgba(0,0,0,0);
    color: rgba(255,255,255,140);
    font-size: 12px;
    font-weight: 600;
    letter-spacing: 1px;
}

QPushButton[mode="true"]:checked {
    color: rgba(255,255,255,235);
    font-weight: 800;
}

QLabel#thumb {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
        stop:0 rgba(255,255,255,22),
        stop:1 rgba(255,255,255,6));
    border: 1px solid rgba(255,255,255,70);
    border-radius: 10px;
}

QWidget#zoomPill {
    background: rgba(0,0,0,140);
    border: 1px solid rgba(255,255,255,35);
    border-radius: 21px;
}

QLabel#zoomLabel {
    color: rgba(255,255,255,220);
    font-size: 12px;
    font-weight: 700;
}

QSlider#zoomSlider::groove:horizontal {
    height: 4px;
    border-radius: 2px;
    background: rgba(255,255,255,50);
}

QSlider#zoomSlider::sub-page:horizontal {
    height: 4px;
    border-radius: 2px;
    background: rgba(255,255,255,140);
}

QSlider#zoomSlider::add-page:horizontal {
    height: 4px;
    border-radius: 2px;
    background: rgba(255,255,255,35);
}

QSlider#zoomSlider::handle:horizontal {
    width: 14px;
    height: 14px;
    margin: -6px 0px;
    border-radius: 7px;
    background: rgba(255,255,255,235);
    border: 1px solid rgba(0,0,0,160);
}
    )QSS"));

    m_shutterButton->setProperty("recording", true);
    m_shutterButton->style()->unpolish(m_shutterButton);
    m_shutterButton->style()->polish(m_shutterButton);

    m_recTimer = new QTimer(this);
    connect(m_recTimer, &QTimer::timeout, this, [this] {
        if (!m_recording) {
            return;
        }
        ++m_elapsedSeconds;
        updateRecText();
    });
    m_recTimer->start(1000);

    m_dotTimer = new QTimer(this);
    connect(m_dotTimer, &QTimer::timeout, this, [this] { updateRecDot(); });
    m_dotTimer->start(33);
    m_dotPhaseTimer.start();

    updateRecText();
    updateRecDot();
}

void CameraHud::setRecording(bool recording)
{
    if (m_recording == recording) {
        return;
    }

    m_recording = recording;

    if (m_recording) {
        m_dotPhaseTimer.restart();
    } else {
        m_recDot->setStyleSheet(QStringLiteral("border-radius:5px; background: rgba(255, 40, 40, 0);"));
    }

    m_shutterButton->setProperty("recording", m_recording);
    m_shutterButton->style()->unpolish(m_shutterButton);
    m_shutterButton->style()->polish(m_shutterButton);
}

void CameraHud::updateRecText()
{
    m_recLabel->setText(QStringLiteral("REC %1").arg(formatHms(m_elapsedSeconds)));
}

void CameraHud::updateRecDot()
{
    if (!m_recording) {
        return;
    }

    const double t = m_dotPhaseTimer.elapsed() / 1000.0;
    const double wave = (qSin(t * 2.0 * 3.14159265358979323846 * 1.2) + 1.0) * 0.5;
    const int alpha = 110 + qRound(wave * 145.0);

    m_recDot->setStyleSheet(QStringLiteral("border-radius:5px; background: rgba(255, 40, 40, %1);").arg(alpha));
}
