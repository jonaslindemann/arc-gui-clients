#include "filepropertiesdialog.h"
#include "ui_filepropertiesdialog.h"

FilePermissionsDialog::FilePermissionsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FilePermissionsDialog)
{
    ui->setupUi(this);
}


FilePermissionsDialog::~FilePermissionsDialog()
{
    delete ui;
}


void FilePermissionsDialog::setPermissions(unsigned int permissions)
{
    if (permissions & 0x4000) ui->readOwner->setChecked(true);
    if (permissions & 0x2000) ui->writeOwner->setChecked(true);
    if (permissions & 0x1000) ui->executeOwner->setChecked(true);
    if (permissions & 0x0400) ui->readUser->setChecked(true);
    if (permissions & 0x0200) ui->writeUser->setChecked(true);
    if (permissions & 0x0100) ui->executeUser->setChecked(true);
    if (permissions & 0x0040) ui->readGroup->setChecked(true);
    if (permissions & 0x0020) ui->writeGroup->setChecked(true);
    if (permissions & 0x0010) ui->executeGroup->setChecked(true);
    if (permissions & 0x0004) ui->readOther->setChecked(true);
    if (permissions & 0x0002) ui->writeOther->setChecked(true);
    if (permissions & 0x0001) ui->executeOther->setChecked(true);
}


unsigned int FilePermissionsDialog::getPermissions()
{
    unsigned int permissions = 0;

    return permissions;
}
