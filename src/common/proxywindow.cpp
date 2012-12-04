#include "proxywindow.h"

#include <QMessageBox>

#include "arcproxy-utils.h"
#include "ui_proxywindow.h"

ProxyWindow::ProxyWindow(QWidget *parent, ArcProxyController* proxyController) :
    QDialog(parent),
    ui(new Ui::ProxyWindow)
{
    ui->setupUi(this);

    m_proxyController = proxyController;

    // Redirect standard output

    /*
    m_debugStream = new QDebugStream(std::cout, this);
    m_debugStream2 = new QDebugStream(std::cerr, this);
    */

    // Setup proxy controller

    m_proxyController->initialize();

    ui->identityText->setText(m_proxyController->getIdentity());

    QDateTime currentTime = QDateTime::currentDateTime();
    QDateTime notValidAfter = currentTime.addSecs(12*60*60);

    ui->proxyLifeTimeEdit->setDateTime(notValidAfter);
    ui->passphraseText->setFocus();

    if (m_proxyController->getUseGSIProxy())
        ui->proxyTypeCombo->setCurrentIndex(1);
    else
        ui->proxyTypeCombo->setCurrentIndex(0);

    int i;

    for (i=0; i<m_proxyController->vomsList().count(); i++)
        ui->vomsServerCombo->addItem(m_proxyController->vomsList().at(i)->alias());

    ui->vomsServerCombo->clearEditText();
    m_proxyController->checkProxy();
    m_proxyController->setUiReturnStatus(ArcProxyController::RS_FAILED);

    ui->vomsConfigTable->setRowCount(m_proxyController->vomsList().count());

    ui->vomsConfigTable->horizontalHeader()->setResizeMode(0, QHeaderView::ResizeToContents);
    ui->vomsConfigTable->horizontalHeader()->setResizeMode(1, QHeaderView::ResizeToContents);
    ui->vomsConfigTable->horizontalHeader()->setResizeMode(2, QHeaderView::ResizeToContents);
    ui->vomsConfigTable->horizontalHeader()->setResizeMode(3, QHeaderView::ResizeToContents);
    ui->vomsConfigTable->horizontalHeader()->setResizeMode(4, QHeaderView::ResizeToContents );

    for (i=0; i<m_proxyController->vomsList().count(); i++)
    {
        ui->vomsConfigTable->setItem(i, 0, new QTableWidgetItem(m_proxyController->vomsList().at(i)->alias()));
        ui->vomsConfigTable->setItem(i, 1, new QTableWidgetItem(m_proxyController->vomsList().at(i)->machine()));
        ui->vomsConfigTable->setItem(i, 2, new QTableWidgetItem(m_proxyController->vomsList().at(i)->port()));
        ui->vomsConfigTable->setItem(i, 3, new QTableWidgetItem(m_proxyController->vomsList().at(i)->hostDN()));
        ui->vomsConfigTable->setItem(i, 4, new QTableWidgetItem(m_proxyController->vomsList().at(i)->officialName()));
    }
}

ProxyWindow::~ProxyWindow()
{
    delete ui;
    /*
    delete m_debugStream;
    delete m_debugStream2;
    */
}

void ProxyWindow::customEvent(QEvent * event)
{
    // When we get here, we've crossed the thread boundary and are now
    // executing in the Qt object's thread

    if(event->type() == DEBUG_STREAM_EVENT)
    {
        handleDebugStreamEvent(static_cast<DebugStreamEvent *>(event));
    }

    // use more else ifs to handle other custom events
}

void ProxyWindow::handleDebugStreamEvent(const DebugStreamEvent *event)
{
    // Now you can safely do something with your Qt objects.
    // Access your custom data using event->getCustomData1() etc.

    /*
    ui->logText->append(event->getOutputText());
    */
}

void ProxyWindow::on_generateButton_clicked()
{   
    if (ui->passphraseText->text().isEmpty())
    {
        QMessageBox::warning(this, "Proxy generation", "Passphrase can't be empy.");
        return;
    }

    QDateTime currentTime = QDateTime::currentDateTime();
    QDateTime notAfterTime = ui->proxyLifeTimeEdit->dateTime();

    m_proxyController->setValidityPeriod(currentTime.secsTo(notAfterTime));
    m_proxyController->setPassphrase(ui->passphraseText->text());
    int result = m_proxyController->generateProxy();

    if (result!=EXIT_SUCCESS)
    {
        m_proxyController->setUiReturnStatus(ArcProxyController::RS_OK);
        QMessageBox::warning(this, "Proxy generation", "Failed to create a proxy.");
    }
    else
    {
        m_proxyController->setUiReturnStatus(ArcProxyController::RS_FAILED);
        QMessageBox::information(this, "Proxy generation", "A proxy certificate has been generated.");
    }

    this->close();
}

void ProxyWindow::on_removeButton_clicked()
{
    m_proxyController->removeProxy();
}

void ProxyWindow::on_passphraseText_returnPressed()
{
    if (ui->passphraseText->text().isEmpty())
    {
        QMessageBox::warning(this, "Proxy generation", "Passphrase can't be empy.");
        return;
    }

    QDateTime currentTime = QDateTime::currentDateTime();
    QDateTime notAfterTime = ui->proxyLifeTimeEdit->dateTime();

    m_proxyController->setValidityPeriod(currentTime.secsTo(notAfterTime));

    m_proxyController->setPassphrase(ui->passphraseText->text());
    int result = m_proxyController->generateProxy();

    if (result!=EXIT_SUCCESS)
    {
        m_proxyController->setUiReturnStatus(ArcProxyController::RS_OK);
        QMessageBox::warning(this, "Proxy generation", "Failed to create a proxy.");
    }
    else
    {
        m_proxyController->setUiReturnStatus(ArcProxyController::RS_FAILED);
        QMessageBox::information(this, "Proxy generation", "A proxy certificate has been generated.");
    }

    this->close();
}

void ProxyWindow::on_proxyTypeCombo_currentIndexChanged(int index)
{
    if (index == 0)
        m_proxyController->setUseGSIProxy(false);
    else
        m_proxyController->setUseGSIProxy(true);
}

void ProxyWindow::on_addVomsServer_clicked()
{
    m_proxyController->addVomsServer(ui->vomsServerCombo->currentText(), ui->vomsRoleText->text());

    if (ui->vomsRoleText->text().length()!=0)
        ui->vomsList->addItem(ui->vomsServerCombo->currentText()+":/"+ui->vomsRoleText->text());
    else
        ui->vomsList->addItem(ui->vomsServerCombo->currentText());
}

void ProxyWindow::on_removeVomsServer_clicked()
{

}

void ProxyWindow::on_addVomsServerConfig_clicked()
{

}

void ProxyWindow::on_removeVomsServerConfig_clicked()
{

}

void ProxyWindow::on_modifyVomsConfigItem_clicked()
{
    ui->vomsConfigTable->editItem(ui->vomsConfigTable->currentItem());
}
