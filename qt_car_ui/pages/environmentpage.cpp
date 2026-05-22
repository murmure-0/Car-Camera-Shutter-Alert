#include "environmentpage.h"

#include "hudwidgets.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRandomGenerator>
#include <QtMath>
#include <QVBoxLayout>

EnvironmentPage::EnvironmentPage(QWidget *parent)
    : QWidget(parent)
{
    auto *grid = new QGridLayout(this);
    grid->setContentsMargins(16, 16, 16, 16);
    grid->setSpacing(16);

    auto *tempCard = makeGlassPanel(this);
    auto *humCard = makeGlassPanel(this);
    auto *bottom = makeGlassPanel(this);
    bottom->setFixedHeight(100);

    auto *tempCol = new QVBoxLayout(tempCard);
    tempCol->setContentsMargins(14, 14, 14, 14);
    tempCol->setSpacing(10);
    auto *tempHdr = new QHBoxLayout;
    auto *tempTitle = new QLabel("Temp", tempCard);
    tempTitle->setStyleSheet("color: rgba(226,232,240,0.65); font-size: 12px; font-weight: 900;");
    auto *tempIcon = new QLabel("℃", tempCard);
    tempIcon->setFixedSize(38, 38);
    tempIcon->setAlignment(Qt::AlignCenter);
    tempIcon->setStyleSheet("background: rgba(249,115,22,0.18); color: #fb923c; border-radius: 19px; font-weight: 900; font-size: 16px;");
    tempHdr->addWidget(tempTitle);
    tempHdr->addStretch(1);
    tempHdr->addWidget(tempIcon);
    tempCol->addLayout(tempHdr);
    m_tempVal = new QLabel("24.5°C", tempCard);
    m_tempVal->setStyleSheet("color: rgba(255,255,255,0.95); font-size: 44px; font-weight: 900;");
    tempCol->addWidget(m_tempVal);
    m_tempChart = new LineChartWidget(tempCard);
    m_tempSeries = {22, 22.5, 23, 23.8, 24.2, 24.5};
    m_tempChart->setSeries(m_tempSeries, QColor("#f97316"));
    tempCol->addWidget(m_tempChart, 1);

    auto *humCol = new QVBoxLayout(humCard);
    humCol->setContentsMargins(14, 14, 14, 14);
    humCol->setSpacing(10);
    auto *humHdr = new QHBoxLayout;
    auto *humTitle = new QLabel("Humidity", humCard);
    humTitle->setStyleSheet("color: rgba(226,232,240,0.65); font-size: 12px; font-weight: 900;");
    auto *humIcon = new QLabel("%", humCard);
    humIcon->setFixedSize(38, 38);
    humIcon->setAlignment(Qt::AlignCenter);
    humIcon->setStyleSheet("background: rgba(59,130,246,0.18); color: #60a5fa; border-radius: 19px; font-weight: 900; font-size: 16px;");
    humHdr->addWidget(humTitle);
    humHdr->addStretch(1);
    humHdr->addWidget(humIcon);
    humCol->addLayout(humHdr);
    m_humVal = new QLabel("48%", humCard);
    m_humVal->setStyleSheet("color: rgba(255,255,255,0.95); font-size: 44px; font-weight: 900;");
    humCol->addWidget(m_humVal);
    m_humChart = new LineChartWidget(humCard);
    m_humSeries = {55, 53, 50, 49, 48, 48};
    m_humChart->setSeries(m_humSeries, QColor("#3b82f6"));
    humCol->addWidget(m_humChart, 1);

    auto *bottomRow = new QHBoxLayout(bottom);
    bottomRow->setContentsMargins(14, 14, 14, 14);
    bottomRow->setSpacing(14);
    auto *stats = new QWidget(bottom);
    auto *statsRow = new QHBoxLayout(stats);
    statsRow->setContentsMargins(0, 0, 0, 0);
    statsRow->setSpacing(18);
    auto makeStat = [stats](const QString &k, const QString &v, const QString &vStyle) {
        auto *w = new QWidget(stats);
        auto *col = new QVBoxLayout(w);
        col->setContentsMargins(0, 0, 0, 0);
        col->setSpacing(4);
        auto *kk = new QLabel(k, w);
        kk->setStyleSheet("color: rgba(226,232,240,0.60); font-size: 11px; font-weight: 800;");
        auto *vv = new QLabel(v, w);
        vv->setStyleSheet(vStyle);
        col->addWidget(kk);
        col->addWidget(vv);
        return w;
    };
    statsRow->addWidget(makeStat("AQI", "Good (42)", "color:#22c55e; font-size: 22px; font-weight: 900;"));
    auto *sep1 = new QFrame(stats);
    sep1->setFixedWidth(1);
    sep1->setStyleSheet("background: rgba(255,255,255,0.10);");
    statsRow->addWidget(sep1);
    statsRow->addWidget(makeStat("Dew Point", "12°C", "color: rgba(255,255,255,0.92); font-size: 22px; font-weight: 900;"));
    auto *sep2 = new QFrame(stats);
    sep2->setFixedWidth(1);
    sep2->setStyleSheet("background: rgba(255,255,255,0.10);");
    statsRow->addWidget(sep2);
    statsRow->addWidget(makeStat("Pressure", "1013 hPa", "color: rgba(255,255,255,0.92); font-size: 22px; font-weight: 900;"));

    auto *refresh = new QPushButton("Refresh Sensors", bottom);
    refresh->setCursor(Qt::PointingHandCursor);
    refresh->setStyleSheet("background: rgba(17,24,39,0.55); border: 1px solid rgba(255,255,255,0.10); padding: 8px 12px; border-radius: 10px; font-weight: 800; color: rgba(255,255,255,0.90);");

    connect(refresh, &QPushButton::clicked, this, [this]() {
        const double baseT = 22.0 + QRandomGenerator::global()->bounded(40) / 10.0;
        const double baseH = 40.0 + QRandomGenerator::global()->bounded(30);
        const int seriesSize = 100;
        QVector<double> tv;
        QVector<double> hv;
        tv.reserve(seriesSize);
        hv.reserve(seriesSize);
        for (int i = 0; i < seriesSize; ++i) {
            tv.push_back(baseT + (QRandomGenerator::global()->bounded(30) - 15) / 10.0);
            hv.push_back(baseH + (QRandomGenerator::global()->bounded(20) - 10));
        }
        if (m_tempChart) {
            m_tempSeries = tv;
            m_tempChart->setSeries(m_tempSeries, QColor("#f97316"));
        }
        if (m_humChart) {
            m_humSeries = hv;
            m_humChart->setSeries(m_humSeries, QColor("#3b82f6"));
        }
        if (m_tempVal) {
            m_tempVal->setText(QString::number(tv.last(), 'f', 1) + "°C");
        }
        if (m_humVal) {
            m_humVal->setText(QString::number(int(hv.last())) + "%");
        }
    });

    bottomRow->addWidget(stats, 1);
    bottomRow->addWidget(refresh, 0, Qt::AlignRight | Qt::AlignVCenter);

    grid->addWidget(tempCard, 0, 0, 1, 1);
    grid->addWidget(humCard, 0, 1, 1, 1);
    grid->addWidget(bottom, 1, 0, 1, 2);
    grid->setRowStretch(0, 1);
    grid->setRowStretch(1, 0);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
}

void EnvironmentPage::updateEnvironment(double temp, double humi)
{
    if (m_tempVal) {
        m_tempVal->setText(QString::number(temp, 'f', 1) + "°C");
    }
    if (m_humVal) {
        m_humVal->setText(QString::number(int(qRound(humi))) + "%");
    }
    const int seriesSize = 100;
    if (m_tempChart) {
        if (m_tempSeries.isEmpty()) {
            m_tempSeries = QVector<double>(seriesSize, temp);
        } else {
            m_tempSeries.push_back(temp);
            while (m_tempSeries.size() > seriesSize) {
                m_tempSeries.removeFirst();
            }
        }
        m_tempChart->setSeries(m_tempSeries, QColor("#f97316"));
    }
    if (m_humChart) {
        if (m_humSeries.isEmpty()) {
            m_humSeries = QVector<double>(seriesSize, humi);
        } else {
            m_humSeries.push_back(humi);
            while (m_humSeries.size() > seriesSize) {
                m_humSeries.removeFirst();
            }
        }
        m_humChart->setSeries(m_humSeries, QColor("#3b82f6"));
    }
}
