#include <QtGui/QApplication>
#include "jobstatuswindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    JobStatusWindow w;
    w.show();
    
    return a.exec();
}
