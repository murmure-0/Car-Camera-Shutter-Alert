#ifndef GPSPAGE_H
#define GPSPAGE_H

#include <QWidget>

class QLineEdit;
class QLabel;
class QNetworkAccessManager;
class MapBackgroundWidget;

class GpsPage : public QWidget
{
    Q_OBJECT
public:
    explicit GpsPage(QWidget *parent = nullptr);
    void updateGps(double lat, double lon, int sat);

private:
    void requestMap();

    MapBackgroundWidget *m_map = nullptr;
    QLineEdit *m_searchInput = nullptr;
    QNetworkAccessManager *m_network = nullptr;
    QLabel *m_speedVal = nullptr;
    QLabel *m_dirVal = nullptr;
    QLabel *m_latVal = nullptr;
    QLabel *m_lonVal = nullptr;
    int m_zoom = 14;
};

#endif // GPSPAGE_H
