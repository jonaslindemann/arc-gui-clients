#include <QtGui/QApplication>
#include <iostream>
#include "mainwindow.h"
#include "arctools.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Check to make sure certs and proxies exists at startup

    if (!ARCTools::instance()->initUserConfig())
        return -1;

    // Start actual user interface

    MainWindow w;
    w.show();
    return a.exec();
}
