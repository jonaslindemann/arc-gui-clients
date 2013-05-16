#include <QtGui/QApplication>
#include <QSplashScreen>
#include <QPixmap>

#include "jobdefinitionwindow.h"
#include "arctools.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QPixmap pixmap(":/arcsub-ui/images/splash_arcsub.png");

    // Set application information

    QCoreApplication::setOrganizationName("lunarc");
    QCoreApplication::setOrganizationDomain("lu.se");
    QCoreApplication::setApplicationName("arc-sub-ui");

    // Check to make sure certs and proxies exists at startup

    if (!ARCTools::instance()->initUserConfig())
        return -1;

    JobDefinitionWindow window;
    window.show();

    QSplashScreen splash(pixmap);
    splash.show();
    splash.raise();
    
    return app.exec();
    return 0;
}
