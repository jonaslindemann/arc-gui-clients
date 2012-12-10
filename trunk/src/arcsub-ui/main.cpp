#include <QtGui/QApplication>
#include "jobdefinitionwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    JobDefinitionWindow w;
    w.show();
    
    return a.exec();
    return 0;
}
