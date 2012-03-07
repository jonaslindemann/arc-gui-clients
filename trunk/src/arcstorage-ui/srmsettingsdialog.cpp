#include "srmsettingsdialog.h"
#include "ui_srmsettingsdialog.h"
#include <qabstractbutton.h>

SRMSettingsDialog::SRMSettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SRMSettingsDialog)
{
    ui->setupUi(this);
}

SRMSettingsDialog::~SRMSettingsDialog()
{
    delete ui;
}

void SRMSettingsDialog::setConfigFilename(QString s)
{
    configFilename = s;
    ui->SRMUserConfigurationFileLineEdit->setText(configFilename);
}

QString SRMSettingsDialog::getConfigFilename()
{
    return configFilename;
}

void SRMSettingsDialog::on_buttonBox_clicked(QAbstractButton* button)
{
    configFilename = ui->SRMUserConfigurationFileLineEdit->text();
}
