#include "storagesplash.h"
#include "ui_storagesplash.h"

StorageSplash::StorageSplash(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StorageSplash)
{
    ui->setupUi(this);
}

StorageSplash::~StorageSplash()
{
    delete ui;
}
