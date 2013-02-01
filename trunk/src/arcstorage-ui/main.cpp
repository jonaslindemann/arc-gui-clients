#include <QtGui/QApplication>
#include <QSplashScreen>
#include <QPixmap>

#include <iostream>

#include "arcstoragewindow.h"
#include "arctools.h"
#include <QImage>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Show splash screen

    QPixmap pixmap(":/resources/images/splash_arc_storage.png");

    // Set application information

    QCoreApplication::setOrganizationName("lunarc");
    QCoreApplication::setOrganizationDomain("lu.se");
    QCoreApplication::setApplicationName("arc-storage-ui");

    // Check to make sure certs and proxies exists at startup

    if (!ARCTools::instance()->initUserConfig())
        return -1;

    // Start actual user interface

    ArcStorageWindow window;
    window.show();

    QSplashScreen splash(pixmap);
    splash.show();
    splash.raise();

    return app.exec();
}
