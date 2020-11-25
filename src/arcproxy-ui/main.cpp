#include <QApplication>
#include <QStyleFactory>

#include "arcproxy-utils.h"
#include "arctools.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("lunarc");
    QCoreApplication::setOrganizationDomain("lu.se");
    QCoreApplication::setApplicationName("arc-proxy-ui");

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

    if (!ARCTools::instance()->initUserConfig(true))
        return -1;

    //ArcProxyController proxyController;
    //proxyController.showProxyUIAppLoop(argc, argv);
    return 0;
}
