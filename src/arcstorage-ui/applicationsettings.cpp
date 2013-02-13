#include "applicationsettings.h"
#include "ui_applicationsettings.h"

#include "globalstateinfo.h"

ApplicationSettings::ApplicationSettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ApplicationSettings)
{
    ui->setupUi(this);
}

void ApplicationSettings::showEvent(QShowEvent *e)
{
    QDialog::showEvent(e);

    if (GlobalStateInfo::instance()->logLevel()==GlobalStateInfo::LL_VERBOSE)
        ui->logLevelCombo->setCurrentIndex(0);
    if (GlobalStateInfo::instance()->logLevel()==GlobalStateInfo::LL_INFO)
        ui->logLevelCombo->setCurrentIndex(1);
    if (GlobalStateInfo::instance()->logLevel()==GlobalStateInfo::LL_WARNING)
        ui->logLevelCombo->setCurrentIndex(2);
    if (GlobalStateInfo::instance()->logLevel()==GlobalStateInfo::LL_ERROR)
        ui->logLevelCombo->setCurrentIndex(3);

    ui->maxTransferSpin->setValue(GlobalStateInfo::instance()->maxTransfers());
    ui->passiveTransfersCheck->setChecked(GlobalStateInfo::instance()->passiveTransfers());
    ui->rememberWindowPosCheck->setChecked(GlobalStateInfo::instance()->rememberWindowPositions());
    ui->rememberWindowURLsCheck->setChecked(GlobalStateInfo::instance()->rememberStartupDirs());
    ui->secureTransfersCheck->setChecked(GlobalStateInfo::instance()->secureTransfers());
    ui->transferRetriesSpin->setValue(GlobalStateInfo::instance()->transferRetries());
    ui->transferTimeoutSpin->setValue(GlobalStateInfo::instance()->transferTimeout());
    ui->windowURLText->setText(GlobalStateInfo::instance()->newWindowUrl());
    ui->redirectLogCheck->setChecked(GlobalStateInfo::instance()->redirectLog());
}


ApplicationSettings::~ApplicationSettings()
{
    delete ui;
}

void ApplicationSettings::on_okButton_clicked()
{
    GlobalStateInfo::instance()->setMaxTransfers(ui->maxTransferSpin->value());
    GlobalStateInfo::instance()->setPassiveTransfers(ui->passiveTransfersCheck->isChecked());
    GlobalStateInfo::instance()->setRememberWindowPositions(ui->rememberWindowPosCheck->isChecked());
    GlobalStateInfo::instance()->setRememberStartupDirs(ui->rememberWindowURLsCheck->isChecked());
    GlobalStateInfo::instance()->setSecureTransfers(ui->secureTransfersCheck->isChecked());
    GlobalStateInfo::instance()->setTransferRetries(ui->transferRetriesSpin->value());
    GlobalStateInfo::instance()->setTransferTimeout(ui->transferTimeoutSpin->value());
    GlobalStateInfo::instance()->setNewWindowUrl(ui->windowURLText->text());
    GlobalStateInfo::instance()->setRedirectLog(ui->redirectLogCheck->isChecked());

    if (ui->logLevelCombo->currentIndex()==0)
        GlobalStateInfo::instance()->setLogLevel(GlobalStateInfo::LL_VERBOSE);
    if (ui->logLevelCombo->currentIndex()==1)
        GlobalStateInfo::instance()->setLogLevel(GlobalStateInfo::LL_INFO);
    if (ui->logLevelCombo->currentIndex()==2)
        GlobalStateInfo::instance()->setLogLevel(GlobalStateInfo::LL_WARNING);
    if (ui->logLevelCombo->currentIndex()==3)
        GlobalStateInfo::instance()->setLogLevel(GlobalStateInfo::LL_ERROR);

    this->accept();
}

void ApplicationSettings::on_cancelButton_clicked()
{
    this->reject();
}
