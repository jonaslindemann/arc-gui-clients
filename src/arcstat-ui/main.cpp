#include <QtGui/QApplication>
#include <QSplashScreen>
#include <QPixmap>

#include "jobstatuswindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QPixmap pixmap(":/arcstat-ui/images/splash_arcstat.png");

    JobStatusWindow window;
    window.show();

    QSplashScreen splash(pixmap);
    splash.show();
    splash.raise();
    
    return app.exec();
}
