#include "gpspage.h"

#include "hudwidgets.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QtGlobal>
#include <QUrl>
#include <QUrlQuery>
#include <QVBoxLayout>

GpsPage::GpsPage(QWidget *parent)
    : QWidget(parent)
{
    m_map = new MapBackgroundWidget(this);
    auto *root = new QGridLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->addWidget(m_map);

    auto *grid = new QGridLayout(m_map);
    grid->setContentsMargins(16, 16, 16, 16);
    grid->setSpacing(10);

    auto *speed = makeGlassPanel(m_map);
    speed->setFixedSize(120, 120);
    auto *speedCol = new QVBoxLayout(speed);
    speedCol->setContentsMargins(12, 12, 12, 12);
    speedCol->setSpacing(4);
    auto *speedLbl = new QLabel("Speed", speed);
    speedLbl->setStyleSheet("color: rgba(226,232,240,0.60); font-size: 11px; font-weight: 800;");
    m_speedVal = new QLabel("0", speed);
    m_speedVal->setStyleSheet("color: rgba(255,255,255,0.95); font-size: 40px; font-weight: 900;");
    auto *speedUnit = new QLabel("km/h", speed);
    speedUnit->setStyleSheet("color: #22d3ee; font-size: 11px; font-weight: 800;");
    speedCol->addWidget(speedLbl, 0, Qt::AlignHCenter);
    speedCol->addWidget(m_speedVal, 0, Qt::AlignHCenter);
    speedCol->addWidget(speedUnit, 0, Qt::AlignHCenter);

    auto *coord = makeGlassPanel(m_map);
    coord->setFixedWidth(220);
    auto *coordCol = new QVBoxLayout(coord);
    coordCol->setContentsMargins(12, 12, 12, 12);
    coordCol->setSpacing(6);
    m_dirVal = new QLabel("NW 315°", coord);
    m_dirVal->setStyleSheet("color: rgba(255,255,255,0.92); font-weight: 900; font-size: 14px;");
    m_latVal = new QLabel("Lat: 34.0522 N", coord);
    m_latVal->setStyleSheet("color: rgba(226,232,240,0.80); font-size: 11px;");
    m_lonVal = new QLabel("Lon: 118.2437 W", coord);
    m_lonVal->setStyleSheet("color: rgba(226,232,240,0.80); font-size: 11px;");
    coordCol->addWidget(m_dirVal);
    coordCol->addWidget(m_latVal);
    coordCol->addWidget(m_lonVal);

    auto *search = new QFrame(m_map);
    search->setProperty("glass", true);
    search->setFixedHeight(36);
    search->setFixedWidth(320);
    search->setStyleSheet("border-radius: 18px;");
    auto *searchRow = new QHBoxLayout(search);
    searchRow->setContentsMargins(12, 0, 6, 0);
    searchRow->setSpacing(8);
    auto *searchIcon = new QLabel("⌕", search);
    searchIcon->setStyleSheet("color: rgba(226,232,240,0.55); font-weight: 900;");
    m_searchInput = new QLineEdit(search);
    m_searchInput->setText("Baidu Building");
    auto *go = new QPushButton("→", search);
    go->setFixedSize(28, 28);
    go->setStyleSheet("background: rgba(8,145,178,0.92); border-radius: 14px; color: white; font-weight: 900;");
    searchRow->addWidget(searchIcon);
    searchRow->addWidget(m_searchInput, 1);
    searchRow->addWidget(go);

    auto *zoomWrap = new QWidget(m_map);
    auto *zoomCol = new QVBoxLayout(zoomWrap);
    zoomCol->setContentsMargins(0, 0, 0, 0);
    zoomCol->setSpacing(8);
    auto makeZ = [this](const QString &t) {
        auto *b = new QPushButton(t, m_map);
        b->setProperty("glass", true);
        b->setFixedSize(40, 40);
        b->setStyleSheet("border-radius: 12px; font-weight: 900;");
        b->setCursor(Qt::PointingHandCursor);
        return b;
    };
    auto *plus = makeZ("+");
    auto *minus = makeZ("−");
    auto *cross = makeZ("◎");
    cross->setStyleSheet("border-radius: 12px; font-weight: 900; color: #22d3ee;");
    zoomCol->addWidget(plus);
    zoomCol->addWidget(minus);
    zoomCol->addSpacing(6);
    zoomCol->addWidget(cross);

    grid->addWidget(speed, 0, 0, 1, 1, Qt::AlignLeft | Qt::AlignTop);
    grid->addWidget(search, 0, 1, 1, 1, Qt::AlignTop | Qt::AlignHCenter);
    grid->addWidget(coord, 2, 0, 1, 1, Qt::AlignLeft | Qt::AlignBottom);
    grid->addWidget(zoomWrap, 2, 2, 1, 1, Qt::AlignRight | Qt::AlignBottom);
    grid->setRowStretch(1, 1);
    grid->setColumnStretch(1, 1);

    m_network = new QNetworkAccessManager(this);

    connect(go, &QPushButton::clicked, this, &GpsPage::requestMap);
    connect(m_searchInput, &QLineEdit::returnPressed, this, &GpsPage::requestMap);
    connect(plus, &QPushButton::clicked, this, [this]() {
        if (m_zoom < 18) {
            ++m_zoom;
            requestMap();
        }
    });
    connect(minus, &QPushButton::clicked, this, [this]() {
        if (m_zoom > 1) {
            --m_zoom;
            requestMap();
        }
    });
    connect(cross, &QPushButton::clicked, this, [this]() {
        m_zoom = 12;
        requestMap();
    });

    requestMap();
}

void GpsPage::updateGps(double lat, double lon, int sat)
{
    if (m_latVal) {
        m_latVal->setText(QStringLiteral("Lat: ") + QString::number(lat, 'f', 6));
    }
    if (m_lonVal) {
        m_lonVal->setText(QStringLiteral("Lon: ") + QString::number(lon, 'f', 6));
    }
    if (m_dirVal) {
        if (sat >= 0) {
            m_dirVal->setText(QStringLiteral("Sats: ") + QString::number(sat));
        } else {
            m_dirVal->setText(QString());
        }
    }
}

void GpsPage::requestMap()
{
    const QSize fallbackSize(800, 480);
    const QSize targetSize = m_map && !m_map->size().isEmpty() ? m_map->size() : fallbackSize;
    const int width = qBound(1, targetSize.width(), 1024);
    const int height = qBound(1, targetSize.height(), 1024);
    const QString center = m_searchInput && !m_searchInput->text().trimmed().isEmpty()
        ? m_searchInput->text().trimmed()
        : QStringLiteral("Beijing");

    QUrl url(QStringLiteral("http://api.map.baidu.com/staticimage"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("center"), center);
    query.addQueryItem(QStringLiteral("markers"), center);
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
