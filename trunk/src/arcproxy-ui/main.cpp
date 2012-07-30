#include <QtGui/QApplication>
#include "proxywindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ProxyWindow w;
    w.show();
    
    return a.exec();
    return 0;
}
