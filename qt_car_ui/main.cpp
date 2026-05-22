#include "mainwindow.h"
#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsProxyWidget>
#include <QGraphicsView>
#include <QFrame>
#include <QDir>
#include <QFile>
#include <cstdlib>
int main(int argc, char *argv[])
{
    system("RkLunch-stop.sh");

    if (qEnvironmentVariableIsEmpty("XDG_RUNTIME_DIR")) {
        const QString runtimeDir = QStringLiteral("/tmp/runtime-%1")
                                       .arg(QString::fromLocal8Bit(qgetenv("USER")));
        QDir().mkpath(runtimeDir);
        QFile::setPermissions(runtimeDir,
                              QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
        qputenv("XDG_RUNTIME_DIR", runtimeDir.toLocal8Bit());
    }

    QApplication a(argc, argv);
    
    MainWindow *mainWindow = new MainWindow();
    
    QGraphicsScene *scene = new QGraphicsScene();
    QGraphicsProxyWidget *proxy = scene->addWidget(mainWindow);
    proxy->setRotation(180);
    
    QGraphicsView *view = new QGraphicsView(scene);
    view->setFixedSize(800, 480);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setFrameStyle(QFrame::NoFrame);
    view->show();
    
    return a.exec();
}
