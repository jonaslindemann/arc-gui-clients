#include "arcproxy_manager.h"


int main(int argc, char *argv[])
{

    setlocale(LC_ALL, "");

    ArcProxyManager proxyManager(argc, argv);
    proxyManager.init();

    if (proxyManager.showInfoOpt())
    {
        proxyManager.showInfo();
        return 0;
    }

    if (proxyManager.removeProxyOpt())
    {
        proxyManager.removeProxy();
        return 0;
    }

    if (!proxyManager.hasValidProxy())
        proxyManager.createProxy();

    return 0;
}

