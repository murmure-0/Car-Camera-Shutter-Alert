#include "settingspage.h"

#include "hudwidgets.h"

#include <QCheckBox>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>
#include <QtMath>

#define SETTINGS_DEBUG_LOG_ENABLED 1
#if SETTINGS_DEBUG_LOG_ENABLED
#define SETTINGS_DEBUG_LOG(...) qDebug() << __VA_ARGS__
#else
#define SETTINGS_DEBUG_LOG(...) do { } while (false)
#endif

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent)
{
    auto *grid = new QGridLayout(this);
    grid->setContentsMargins(16, 16, 16, 16);
    grid->setSpacing(16);

    auto *left = makeGlassPanel(this);
    auto *right = makeGlassPanel(this);

    auto *leftCol = new QVBoxLayout(left);
    leftCol->setContentsMargins(14, 14, 14, 14);
    leftCol->setSpacing(14);
    auto *leftTitle = new QLabel("Display & Sound", left);
    leftTitle->setStyleSheet("color: rgba(226,232,240,0.65); font-size: 11px; font-weight: 900;");
    leftCol->addWidget(leftTitle);

    auto makeSliderRow = [left](const QString &name, int init) {
        auto *wrap = new QWidget(left);
        auto *col = new QVBoxLayout(wrap);
        col->setContentsMargins(0, 0, 0, 0);
        col->setSpacing(6);
        auto *top = new QHBoxLayout;
        auto *lbl = new QLabel(name, wrap);
        lbl->setStyleSheet("color: rgba(255,255,255,0.90); font-size: 12px; font-weight: 800;");
        auto *val = new QLabel(QString::number(init) + "%", wrap);
        val->setStyleSheet("color: rgba(226,232,240,0.60); font-size: 12px;");
        top->addWidget(lbl);
        top->addStretch(1);
        top->addWidget(val);
        auto *sliderRow = new QHBoxLayout;
        auto *minusBtn = new QPushButton("−", wrap);
        minusBtn->setFixedSize(36, 36);
        minusBtn->setStyleSheet("QPushButton { background: rgba(255,255,255,0.08); border-radius: 18px; color: rgba(255,255,255,0.85); font-size: 18px; font-weight: 700; }"
                                "QPushButton:pressed { background: rgba(255,255,255,0.18); }");
        auto *s = new QSlider(Qt::Horizontal, wrap);
        s->setRange(0, 100);
        s->setValue(init);
        auto *plusBtn = new QPushButton("+", wrap);
        plusBtn->setFixedSize(36, 36);
        plusBtn->setStyleSheet("QPushButton { background: rgba(255,255,255,0.08); border-radius: 18px; color: rgba(255,255,255,0.85); font-size: 18px; font-weight: 700; }"
                               "QPushButton:pressed { background: rgba(255,255,255,0.18); }");
        sliderRow->addWidget(minusBtn);
        sliderRow->addWidget(s, 1);
        sliderRow->addWidget(plusBtn);
        col->addLayout(top);
        col->addLayout(sliderRow);
        QObject::connect(minusBtn, &QPushButton::clicked, s, [s]() {
            s->setValue(qMax(0, s->value() - 10));
        });
        QObject::connect(plusBtn, &QPushButton::clicked, s, [s]() {
            s->setValue(qMin(100, s->value() + 10));
        });
        return std::tuple<QWidget *, QSlider *, QLabel *>(wrap, s, val);
    };

    QWidget *brightWrap;
    QSlider *bright;
    QLabel *brightVal;
    std::tie(brightWrap, bright, brightVal) = makeSliderRow("Brightness", 80);
    bright->setTracking(false);
    leftCol->addWidget(brightWrap);

    QWidget *volWrap;
    QSlider *vol;
    QLabel *volVal;
    std::tie(volWrap, vol, volVal) = makeSliderRow("Volume", 60);
    vol->setTracking(false);
    leftCol->addWidget(volWrap);

    auto *darkRow = new QHBoxLayout;
    auto *darkLbl = new QLabel("Dark Mode", left);
    darkLbl->setStyleSheet("color: rgba(255,255,255,0.90); font-size: 12px; font-weight: 800;");
    auto *darkToggle = new QCheckBox(left);
    darkToggle->setChecked(true);
    darkRow->addWidget(darkLbl);
    darkRow->addStretch(1);
    darkRow->addWidget(darkToggle);
    leftCol->addLayout(darkRow);
    leftCol->addStretch(1);

    const QString brightnessPath = QStringLiteral("/sys/class/backlight/backlight/brightness");
    const QString maxBrightnessPath = QStringLiteral("/sys/class/backlight/backlight/max_brightness");
    auto applyBrightness = [brightnessPath, maxBrightnessPath](int v) {
        QFileInfo info(brightnessPath);
        if (info.exists()) {
            int maxValue = 100;
            QFile maxFile(maxBrightnessPath);
            if (maxFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                bool ok = false;
                const int parsed = QString::fromUtf8(maxFile.readAll()).trimmed().toInt(&ok);
                if (ok && parsed > 0) {
                    maxValue = parsed;
                }
                else
                {
                    SETTINGS_DEBUG_LOG(QString("max brightness parse failed: %1").arg(maxBrightnessPath));
                }
            }
            if(v < 10)
            {
                v = 10;
            }
            const int scaled = (v * maxValue) / 100;
            QFile brightFile(brightnessPath);
            if (brightFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
                brightFile.write(QString::number(scaled).toUtf8());
                return;
            }
            SETTINGS_DEBUG_LOG(QString("brightness write failed: %1").arg(brightnessPath));
        }
        SETTINGS_DEBUG_LOG(QString("brightness fallback (no sysfs): %1%").arg(v));
    };

    connect(bright, &QSlider::valueChanged, this, [brightVal, applyBrightness](int v) {
        brightVal->setText(QString::number(v) + "%");
        SETTINGS_DEBUG_LOG(QString("brightness %1%").arg(v));
        applyBrightness(v);
    });
    
    auto applyVolume = [](int v) {
        int mixerValue = (v * 30) / 100;
        if (mixerValue < 0) mixerValue = 0;
        if (mixerValue > 30) mixerValue = 30;
        
        QString cmd = QString("amixer set 'DAC LINEOUT' %1").arg(mixerValue);
        int result = system(cmd.toUtf8().constData());
        if (result != 0) {
            SETTINGS_DEBUG_LOG(QString("volume set failed: %1").arg(cmd));
        }
    };
    
    connect(vol, &QSlider::valueChanged, this, [volVal, applyVolume](int v) {
        volVal->setText(QString::number(v) + "%");
        SETTINGS_DEBUG_LOG(QString("volume %1%").arg(v));
        applyVolume(v);
    });
    connect(darkToggle, &QCheckBox::toggled, this, &SettingsPage::darkModeToggled);

    auto *rightTitle = new QLabel("Device Info", right);
    rightTitle->setStyleSheet("color: rgba(226,232,240,0.65); font-size: 11px; font-weight: 900;");
    rightCol->addWidget(rightTitle);

    auto makeInfo = [right](const QString &k, const QString &v) {
        auto *row = new QWidget(right);
        auto *h = new QHBoxLayout(row);
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(10);
        auto *kk = new QLabel(k, row);
        kk->setStyleSheet("color: rgba(226,232,240,0.55); font-size: 12px;");
        auto *vv = new QLabel(v, row);
        vv->setStyleSheet("color: rgba(255,255,255,0.92); font-size: 12px; font-weight: 800;");
        h->addWidget(kk);
        h->addStretch(1);
        h->addWidget(vv);
        return row;
    };

    rightCol->addWidget(makeInfo("Model", "ProtoBoard V2"));
    rightCol->addWidget(makeInfo("OS Version", "Build 2023.10.05"));
    rightCol->addWidget(makeInfo("Storage", "12GB / 64GB"));
    rightCol->addStretch(1);

    auto *reset = new QPushButton("Factory Reset", right);
    reset->setCursor(Qt::PointingHandCursor);
    reset->setStyleSheet("background: rgba(127,29,29,0.35); border: 1px solid rgba(248,113,113,0.35); color: #f87171; padding: 10px 12px; border-radius: 10px; font-weight: 900;");
    rightCol->addWidget(reset);

    connect(reset, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this, "Factory Reset", "Factory reset requested (prototype demo).");
    });

    grid->addWidget(left, 0, 0, 1, 1);
    grid->addWidget(right, 0, 1, 1, 1);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
}
