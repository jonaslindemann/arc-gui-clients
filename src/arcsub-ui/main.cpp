#include <QtGui/QApplication>
#include "jobdefinitionwindow.h"
#include "arctools.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Set application information

    QCoreApplication::setOrganizationName("lunarc");
    QCoreApplication::setOrganizationDomain("lu.se");
    QCoreApplication::setApplicationName("arc-sub-ui");

    // Check to make sure certs and proxies exists at startup

    if (!ARCTools::instance()->initUserConfig())
        return -1;

    JobDefinitionWindow w;
    w.show();
    
    return a.exec();
    return 0;
}
