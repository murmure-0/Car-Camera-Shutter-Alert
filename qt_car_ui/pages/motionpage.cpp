#include "motionpage.h"

#include "hudwidgets.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProcess>
#include <QResizeEvent>
#include <QtMath>
#include <QTimer>
#include <QVBoxLayout>

MotionPage::MotionPage(QWidget *parent)
    : QWidget(parent)
{
    auto *grid = new QGridLayout(this);
    grid->setContentsMargins(16, 16, 16, 16);
    grid->setSpacing(16);

    auto *left = new QWidget(this);
    auto *leftCol = new QVBoxLayout(left);
    leftCol->setContentsMargins(0, 0, 0, 0);
    leftCol->setSpacing(12);
    leftCol->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    auto *horizonCard = makeGlassPanel(left);
    horizonCard->setFixedSize(300, 300);
    auto *horizonCol = new QVBoxLayout(horizonCard);
    horizonCol->setContentsMargins(18, 18, 18, 18);
    horizonCol->setSpacing(10);
    m_horizonWidget = new ArtificialHorizonWidget(horizonCard);
    horizonCol->addWidget(m_horizonWidget, 1, Qt::AlignCenter);

    auto *prRow = new QHBoxLayout;
    auto *pitchWrap = new QWidget(left);
    auto *pitchCol = new QVBoxLayout(pitchWrap);
    pitchCol->setContentsMargins(0, 0, 0, 0);
    pitchCol->setSpacing(2);
    auto *pitchK = new QLabel("Pitch", pitchWrap);
    pitchK->setStyleSheet("color: rgba(226,232,240,0.60); font-size: 11px; font-weight: 900;");
    m_pitchVal = new QLabel("-5°", pitchWrap);
    m_pitchVal->setStyleSheet("color: rgba(255,255,255,0.92); font-size: 20px; font-weight: 900;");
    pitchCol->addWidget(pitchK, 0, Qt::AlignHCenter);
    pitchCol->addWidget(m_pitchVal, 0, Qt::AlignHCenter);

    auto *rollWrap = new QWidget(left);
    auto *rollCol = new QVBoxLayout(rollWrap);
    rollCol->setContentsMargins(0, 0, 0, 0);
    rollCol->setSpacing(2);
    auto *rollK = new QLabel("Roll", rollWrap);
    rollK->setStyleSheet("color: rgba(226,232,240,0.60); font-size: 11px; font-weight: 900;");
    m_rollVal = new QLabel("12°", rollWrap);
    m_rollVal->setStyleSheet("color: rgba(255,255,255,0.92); font-size: 20px; font-weight: 900;");
    rollCol->addWidget(rollK, 0, Qt::AlignHCenter);
    rollCol->addWidget(m_rollVal, 0, Qt::AlignHCenter);

    prRow->addWidget(pitchWrap);
    prRow->addSpacing(24);
    prRow->addWidget(rollWrap);

    leftCol->addWidget(horizonCard, 0, Qt::AlignHCenter);
    leftCol->addLayout(prRow);

    auto *right = new QWidget(this);
    auto *rightCol = new QVBoxLayout(right);
    rightCol->setContentsMargins(0, 0, 0, 0);
    rightCol->setSpacing(16);

    auto *gCard = makeGlassPanel(right);
    auto *gCol = new QVBoxLayout(gCard);
    gCol->setContentsMargins(14, 14, 14, 14);
    gCol->setSpacing(10);
    auto *gTitle = new QLabel("G-Force", gCard);
    gTitle->setStyleSheet("color: rgba(226,232,240,0.65); font-size: 11px; font-weight: 900;");
    gCol->addWidget(gTitle);

    auto *gBody = new QWidget(gCard);
    auto *gRow = new QHBoxLayout(gBody);
    gRow->setContentsMargins(0, 0, 0, 0);
    gRow->setSpacing(12);
    auto *gMeter = new QFrame(gBody);
    gMeter->setFixedSize(96, 96);
    gMeter->setStyleSheet("border: 4px solid rgba(255,255,255,0.10); border-radius: 48px; background: rgba(0,0,0,0.12);");
    m_gDot = new QFrame(gMeter);
    m_gDot->setFixedSize(8, 8);
    m_gDot->setStyleSheet("background: #ef4444; border-radius: 4px;");
    m_gDot->move(48 + 10 - 4, 48 - 10 - 4);

    auto *gRead = new QWidget(gBody);
    auto *gReadCol = new QVBoxLayout(gRead);
    gReadCol->setContentsMargins(0, 0, 0, 0);
    gReadCol->setSpacing(6);

    auto makeKV = [gRead](const QString &k, const QString &v) {
        auto *row = new QWidget(gRead);
        auto *h = new QHBoxLayout(row);
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(8);
        auto *kk = new QLabel(k, row);
        kk->setStyleSheet("color: rgba(226,232,240,0.60); font-size: 12px; font-weight: 900;");
        auto *vv = new QLabel(v, row);
        vv->setStyleSheet("color: rgba(255,255,255,0.92); font-size: 12px; font-weight: 900;");
        h->addWidget(kk);
        h->addStretch(1);
        h->addWidget(vv);
        return QPair<QWidget *, QLabel *>(row, vv);
    };

    const auto xKV = makeKV("X:", "0.42");
    const auto yKV = makeKV("Y:", "-0.15");
    const auto zKV = makeKV("Z:", "0.98");
    m_gxVal = xKV.second;
    m_gyVal = yKV.second;
    m_gzVal = zKV.second;
    gReadCol->addWidget(xKV.first);
    gReadCol->addWidget(yKV.first);
    gReadCol->addWidget(zKV.first);
    gReadCol->addStretch(1);

    gRow->addWidget(gMeter);
    gRow->addWidget(gRead, 1);
    gCol->addWidget(gBody);

    auto *rotCard = makeGlassPanel(right);
    auto *rotCol = new QVBoxLayout(rotCard);
    rotCol->setContentsMargins(14, 14, 14, 14);
    rotCol->setSpacing(10);
    auto *rotTitle = new QLabel("Angular Vel (°/s)", rotCard);
    rotTitle->setStyleSheet("color: rgba(226,232,240,0.65); font-size: 11px; font-weight: 900;");
    rotCol->addWidget(rotTitle);

    auto *rotGrid = new QGridLayout;
    rotGrid->setContentsMargins(0, 0, 0, 0);
    rotGrid->setSpacing(10);
    auto makeSmall = [rotCard](const QString &axis, const QString &v, const QString &c) {
        auto *w = new QFrame(rotCard);
        w->setStyleSheet("background: rgba(0,0,0,0.20); border: 1px solid rgba(255,255,255,0.08); border-radius: 12px;");
        auto *col = new QVBoxLayout(w);
        col->setContentsMargins(10, 10, 10, 10);
        col->setSpacing(4);
        auto *a = new QLabel(axis, w);
        a->setStyleSheet(QString("color:%1; font-weight: 900; font-size: 12px;").arg(c));
        auto *vv = new QLabel(v, w);
        vv->setStyleSheet("color: rgba(255,255,255,0.92); font-weight: 900; font-size: 14px;");
        col->addWidget(a, 0, Qt::AlignHCenter);
        col->addWidget(vv, 0, Qt::AlignHCenter);
        return QPair<QWidget *, QLabel *>(w, vv);
    };
    const auto rotX = makeSmall("X", "4.2", "#22d3ee");
    const auto rotY = makeSmall("Y", "-1.2", "#a855f7");
    const auto rotZ = makeSmall("Z", "0.5", "#22c55e");
    m_rotXVal = rotX.second;
    m_rotYVal = rotY.second;
    m_rotZVal = rotZ.second;
    rotGrid->addWidget(rotX.first, 0, 0);
    rotGrid->addWidget(rotY.first, 0, 1);
    rotGrid->addWidget(rotZ.first, 0, 2);
    rotCol->addLayout(rotGrid);
    rotCol->addStretch(1);

    rightCol->addWidget(gCard, 1);
    rightCol->addWidget(rotCard, 0);

    grid->addWidget(left, 0, 0, 1, 1);
    grid->addWidget(right, 0, 1, 1, 1);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);

    m_warningOverlay = new QFrame(this);
    m_warningOverlay->setObjectName("rolloverWarningOverlay");
    m_warningOverlay->setStyleSheet(
        "QFrame#rolloverWarningOverlay {"
        "  background: transparent;"
        "  border: 6px solid rgba(239,68,68,0.90);"
        "  border-radius: 18px;"
        "}");
    m_warningOverlay->hide();

    auto *warnLayout = new QVBoxLayout(m_warningOverlay);
    warnLayout->setAlignment(Qt::AlignCenter);
    warnLayout->setSpacing(8);

    m_warningIcon = new QLabel("⚠", m_warningOverlay);
    m_warningIcon->setStyleSheet("color: rgba(239,68,68,0.95); font-size: 48px; border: none; background: transparent;");
    m_warningIcon->setAlignment(Qt::AlignCenter);
    warnLayout->addWidget(m_warningIcon);

    m_warningText = new QLabel("Rollover Warning", m_warningOverlay);
    m_warningText->setStyleSheet("color: rgba(239,68,68,0.95); font-size: 22px; font-weight: 900; border: none; background: transparent;");
    m_warningText->setAlignment(Qt::AlignCenter);
    warnLayout->addWidget(m_warningText);

    m_flashTimer = new QTimer(this);
    m_flashTimer->setInterval(400);
    connect(m_flashTimer, &QTimer::timeout, this, [this]() {
        m_flashState = !m_flashState;
        if (m_flashState) {
            m_warningOverlay->setStyleSheet(
                "QFrame#rolloverWarningOverlay {"
                "  background: rgba(239,68,68,0.15);"
                "  border: 6px solid rgba(239,68,68,0.95);"
                "  border-radius: 18px;"
                "}");
        } else {
            m_warningOverlay->setStyleSheet(
                "QFrame#rolloverWarningOverlay {"
                "  background: transparent;"
                "  border: 6px solid rgba(239,68,68,0.40);"
                "  border-radius: 18px;"
                "}");
        }
    });

    m_soundProcess = new QProcess(this);
    m_soundRepeatTimer = new QTimer(this);
    m_soundRepeatTimer->setInterval(3000);
    connect(m_soundRepeatTimer, &QTimer::timeout, this, &MotionPage::playWarningSound);
}

void MotionPage::updateMotion(double ax, double ay, double az, double gx, double gy, double gz, double pitch, double roll, double yaw)
{
    const double gxClamped = qBound(-1.0, ax, 1.0);
    const double gyClamped = qBound(-1.0, ay, 1.0);
    if (m_gDot) {
        const int cx = 48 + int(gxClamped * 18) - 4;
        const int cy = 48 - int(gyClamped * 18) - 4;
        m_gDot->move(cx, cy);
    }
    if (m_gxVal) {
        m_gxVal->setText(QString::number(ax, 'f', 2));
    }
    if (m_gyVal) {
        m_gyVal->setText(QString::number(ay, 'f', 2));
    }
    if (m_gzVal) {
        m_gzVal->setText(QString::number(az, 'f', 2));
    }
    if (m_rotXVal) {
        m_rotXVal->setText(QString::number(gx, 'f', 2));
    }
    if (m_rotYVal) {
        m_rotYVal->setText(QString::number(gy, 'f', 2));
    }
    if (m_rotZVal) {
        m_rotZVal->setText(QString::number(gz, 'f', 2));
    }
    if (!qIsNaN(pitch) && m_pitchVal) {
        m_pitchVal->setText(QString::number(int(qRound(pitch))) + "°");
    }
    if (!qIsNaN(roll) && m_rollVal) {
        m_rollVal->setText(QString::number(int(qRound(roll))) + "°");
        if (qAbs(roll) > 5.0) {
            if (!m_isRolloverWarning) {
                startRolloverWarning();
            }
        } else {
            if (m_isRolloverWarning) {
                stopRolloverWarning();
            }
        }
    }
    if (!qIsNaN(pitch) && !qIsNaN(roll) && m_horizonWidget) {
        m_horizonWidget->setPitchRoll(pitch, roll);
    }
    (void)yaw;
}

void MotionPage::startRolloverWarning()
{
    m_isRolloverWarning = true;
    m_flashState = true;

    m_warningOverlay->setGeometry(this->rect());
    m_warningOverlay->raise();
    m_warningOverlay->show();

    m_flashTimer->start();

    if (m_rollVal) {
        m_rollVal->setStyleSheet("color: rgba(239,68,68,0.95); font-size: 20px; font-weight: 900;");
    }

    playWarningSound();
    m_soundRepeatTimer->start();

    emit rolloverWarning(true);
}

void MotionPage::stopRolloverWarning()
{
    m_isRolloverWarning = false;
    m_flashState = false;

    m_flashTimer->stop();
    m_warningOverlay->hide();

    if (m_rollVal) {
        m_rollVal->setStyleSheet("color: rgba(255,255,255,0.92); font-size: 20px; font-weight: 900;");
    }

    stopWarningSound();

    emit rolloverWarning(false);
}

void MotionPage::playWarningSound()
{
    if (!m_soundProcess) return;
    if (m_soundProcess->state() == QProcess::Running) return;
    m_soundProcess->start("play", QStringList() << "-q" << "./Sound_Efftcts/Rollover_warning_alert.mp3");
}

void MotionPage::stopWarningSound()
{
    m_soundRepeatTimer->stop();
    if (m_soundProcess && m_soundProcess->state() == QProcess::Running) {
        m_soundProcess->kill();
        m_soundProcess->waitForFinished(500);
    }
    m_isSoundActive = false;
}

void MotionPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_warningOverlay) {
        m_warningOverlay->setGeometry(this->rect());
    }
}
