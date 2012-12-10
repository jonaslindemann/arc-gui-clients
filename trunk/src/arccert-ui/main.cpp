#include <QtGui/QApplication>
#include "certconvertwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CertConvertWindow w;
    w.show();
    
    return a.exec();
    return 0;
}
