#include "connectivitypage.h"

#include "hudwidgets.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

ConnectivityPage::ConnectivityPage(QWidget *parent)
    : QWidget(parent)
{
    auto *grid = new QGridLayout(this);
    grid->setContentsMargins(16, 16, 16, 16);
    grid->setSpacing(16);

    auto *wifi = makeGlassPanel(this);
    auto *bt = makeGlassPanel(this);

    auto *wifiCol = new QVBoxLayout(wifi);
    wifiCol->setContentsMargins(14, 14, 14, 14);
    wifiCol->setSpacing(12);

    auto *wifiTop = new QHBoxLayout;
    auto *wifiTitle = new QLabel("Wi-Fi", wifi);
    wifiTitle->setStyleSheet("color: rgba(255,255,255,0.92); font-size: 18px; font-weight: 900;");
    auto *wifiToggle = new QCheckBox(wifi);
    wifiToggle->setChecked(true);
    wifiTop->addWidget(wifiTitle);
    wifiTop->addStretch(1);
    wifiTop->addWidget(wifiToggle);
    wifiCol->addLayout(wifiTop);

    auto *wifiStatus = new QLabel("Connected to \"Home_5G\"", wifi);
    wifiStatus->setStyleSheet("color: rgba(226,232,240,0.65); font-size: 11px;");
    wifiCol->addWidget(wifiStatus);

    auto *wifiList = new QListWidget(wifi);
    wifiList->addItem("Home_5G  ✓");
    wifiList->addItem("Office_Guest  🔒");
    wifiList->addItem("Starbucks_Free");
    wifiList->addItem("iPhone Hotspot  🔒");
    wifiCol->addWidget(wifiList, 1);

    connect(wifiToggle, &QCheckBox::toggled, this, [wifiStatus, wifiList](bool on) {
        if (on) {
            wifiStatus->setText("Connected to \"Home_5G\"");
            wifiStatus->setStyleSheet("color: rgba(226,232,240,0.65); font-size: 11px;");
            wifiList->setEnabled(true);
        } else {
            wifiStatus->setText("Wi-Fi Off");
            wifiStatus->setStyleSheet("color: #f87171; font-size: 11px; font-weight: 800;");
            wifiList->setEnabled(false);
        }
    });

    auto *btCol = new QVBoxLayout(bt);
    btCol->setContentsMargins(14, 14, 14, 14);
    btCol->setSpacing(12);

    auto *btTop = new QHBoxLayout;
    auto *btTitle = new QLabel("Bluetooth", bt);
    btTitle->setStyleSheet("color: rgba(255,255,255,0.92); font-size: 18px; font-weight: 900;");
    auto *btToggle = new QCheckBox(bt);
    btToggle->setChecked(true);
    btTop->addWidget(btTitle);
    btTop->addStretch(1);
    btTop->addWidget(btToggle);
    btCol->addLayout(btTop);

    auto *btStatus = new QLabel("Discoverable: \"Car_Unit_01\"", bt);
    btStatus->setStyleSheet("color: rgba(226,232,240,0.65); font-size: 11px;");
    btCol->addWidget(btStatus);

    auto *btList = new QListWidget(bt);
    btList->addItem("Sony WH-1000XM4  Connected");
    btList->addItem("Pixel 7 Pro  Paired");
    btList->addItem("MacBook Pro  Available");
    btCol->addWidget(btList, 1);

    auto *scan = new QPushButton("Scan Devices", bt);
    scan->setCursor(Qt::PointingHandCursor);
    scan->setStyleSheet("background: rgba(17,24,39,0.55); border: 1px solid rgba(255,255,255,0.10); padding: 8px 12px; border-radius: 10px; font-weight: 800; color: rgba(255,255,255,0.90);");
    btCol->addWidget(scan);

    connect(btToggle, &QCheckBox::toggled, this, [btStatus, btList, scan](bool on) {
        if (on) {
            btStatus->setText("Discoverable: \"Car_Unit_01\"");
            btStatus->setStyleSheet("color: rgba(226,232,240,0.65); font-size: 11px;");
            btList->setEnabled(true);
            scan->setEnabled(true);
        } else {
            btStatus->setText("Bluetooth Off");
            btStatus->setStyleSheet("color: #f87171; font-size: 11px; font-weight: 800;");
            btList->setEnabled(false);
            scan->setEnabled(false);
        }
    });

    connect(scan, &QPushButton::clicked, this, [btList]() {
        btList->addItem("New Device " + QString::number(btList->count() + 1));
        btList->scrollToBottom();
    });

    grid->addWidget(wifi, 0, 0, 1, 1);
    grid->addWidget(bt, 0, 1, 1, 1);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
}
