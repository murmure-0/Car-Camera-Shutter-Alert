#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include <QWidget>

class SettingsPage : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsPage(QWidget *parent = nullptr);

signals:
    void darkModeToggled(bool on);
};

#endif // SETTINGSPAGE_H

