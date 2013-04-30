#include "arcproxy-utils.h"
#include <QApplication>
#include "arctools.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("lunarc");
    QCoreApplication::setOrganizationDomain("lu.se");
    QCoreApplication::setApplicationName("arc-proxy-ui");

    if (!ARCTools::instance()->initUserConfig(true))
        return -1;

    //ArcProxyController proxyController;
    //proxyController.showProxyUIAppLoop(argc, argv);
    return 0;
}
