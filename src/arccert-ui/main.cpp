#include <QtGui/QApplication>
#include <QStyleFactory>

#include "certconvertwindow.h"

int main(int argc, char *argv[])
{
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

    CertConvertWindow w;
    w.show();
    
    return app.exec();
    return 0;
}
