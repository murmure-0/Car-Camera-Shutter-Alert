#ifndef ENVIRONMENTPAGE_H
#define ENVIRONMENTPAGE_H

#include <QVector>
#include <QWidget>

class QLabel;
class LineChartWidget;

class EnvironmentPage : public QWidget
{
    Q_OBJECT
public:
    explicit EnvironmentPage(QWidget *parent = nullptr);
    void updateEnvironment(double temp, double humi);

private:
    QLabel *m_tempVal = nullptr;
    QLabel *m_humVal = nullptr;
    LineChartWidget *m_tempChart = nullptr;
    LineChartWidget *m_humChart = nullptr;
    QVector<double> m_tempSeries;
    QVector<double> m_humSeries;
};

#endif // ENVIRONMENTPAGE_H
