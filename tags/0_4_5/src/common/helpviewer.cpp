#include "helpviewer.h"
#include "ui_helpviewer.h"

HelpViewer::HelpViewer(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HelpViewer)
{
    ui->setupUi(this);
}

HelpViewer::~HelpViewer()
{
    delete ui;
}
