#include "arcproxy-utils.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("lunarc");
    QCoreApplication::setOrganizationDomain("lu.se");
    QCoreApplication::setApplicationName("arc-proxy-ui");

    ArcProxyController proxyController;
    proxyController.showProxyUIAppLoop();
    return 0;
}
