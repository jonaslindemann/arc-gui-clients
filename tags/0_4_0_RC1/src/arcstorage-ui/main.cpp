#include <QtGui/QApplication>
#include <iostream>
#include "arcstoragewindow.h"
#include "arctools.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Set application information

    QCoreApplication::setOrganizationName("lunarc");
    QCoreApplication::setOrganizationDomain("lu.se");
    QCoreApplication::setApplicationName("arc-storage-ui");

    // Check to make sure certs and proxies exists at startup

    if (!ARCTools::instance()->initUserConfig())
        return -1;

    // Start actual user interface

    ArcStorageWindow w;
    w.show();
    return a.exec();
}
