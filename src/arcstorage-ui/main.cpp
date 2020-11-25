#include <iostream>

#include <QtGui/QApplication>
#include <QSplashScreen>
#include <QPixmap>
#include <QImage>
#include <QStringList>
#include <QStyleFactory>

#include "arcstoragewindow.h"
#include "arcproxy-utils.h"
#include "arctools.h"


int main(int argc, char *argv[])
{
    // Not sure why this is needed. Can't hurt.

    Arc::SetEnv("X509_CERT_DIR", "/etc/grid-security/certificates");

    QApplication app(argc, argv);

    // Select appropriate style

    QStringList defaultStyles = QStyleFactory::keys();

    if (defaultStyles.contains("Adwaita"))
        app.setStyle("adwaita");
    else if (defaultStyles.contains("Plastique"))
        app.setStyle("Plastique");
    else if (defaultStyles.contains("Cleanlooks"))
        app.setStyle("Cleanlooks");
    else if (defaultStyles.contains("GTK+"))
        app.setStyle("GTK+");

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

    ArcStorageWindow window(0, false);
    window.show();

    QSplashScreen splash(pixmap);
    splash.show();
    splash.raise();

    int result = app.exec();

    Arc::ThreadInitializer().waitExit();

    return result;
}
