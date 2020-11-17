#include "proxywindow.h"

#include <QMessageBox>
#include <QSettings>
#include <QInputDialog>
#include <QMessageBox>

#include "arcproxy-utils.h"
#include "ui_proxywindow.h"

#include "helpwindow.h"

ProxyWindow::ProxyWindow(QWidget *parent, ArcProxyController* proxyController) :
    QDialog(parent),
    ui(new Ui::ProxyWindow)
{
    m_helpWindow = 0;

    ui->setupUi(this);

    m_proxyController = proxyController;

    // Setup proxy controller

    m_proxyController->initialize();

    // Setup user interface

    ui->identityText->setText(m_proxyController->getIdentity());

    QDateTime currentTime = QDateTime::currentDateTime();
    QDateTime notValidAfter = currentTime.addSecs(12*60*60);

    ui->proxyLifeTimeEdit->setDateTime(notValidAfter);
    ui->generateButton->setFocus();

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

    m_configTableDirty = false;

    ui->NSSProfileList->clear();
    for (i=0; i<m_proxyController->nssPathCount(); i++)
    {
        ui->NSSProfileList->addItem(m_proxyController->getNssPath(i));
    }

    ui->NSSProfileList->setCurrentRow(0);

    this->readSettings();
}

void ProxyWindow::writeSettings()
{
    // Store list of VOMS servers in application settings

    QSettings settings;
    settings.remove("VomsServers");
    settings.beginGroup("VomsServers");
    for (int i=0; i<m_proxyController->vomsServerCount(); i++)
        settings.setValue("Server"+QString::number(i), m_proxyController->getVomsServer(i));
    settings.endGroup();

    settings.sync();
}

void ProxyWindow::readSettings()
{
    // Reading from application settings

    QSettings settings;
    for (int i=0; i<100; i++)
    {
        if (settings.childGroups().contains("VomsServers"))
        {
            settings.beginGroup("VomsServers");

            if (settings.contains("Server"+QString::number(i)))
            {
                QVariant serverDeclaration = settings.value("Server"+QString::number(i));
                m_proxyController->addVomsServerAndRole(serverDeclaration.toString());
            }
            settings.endGroup();
        }
    }

    // Updating user interface

    ui->vomsList->clear();

    for (int i=0; i<m_proxyController->vomsServerCount(); i++)
        ui->vomsList->addItem(m_proxyController->getVomsServer(i));
}

ProxyWindow::~ProxyWindow()
{
    // Write settings before exiting window

    if (m_configTableDirty)
    {
        int retVal = QMessageBox::question(0, "VOMS Server configuration", "Save vomses configuration file", QMessageBox::Yes, QMessageBox::No);

        if (retVal == QMessageBox::Yes)
            m_proxyController->vomsList().write();
    }

    this->writeSettings();

    if (m_helpWindow!=0)
        delete m_helpWindow;

    delete ui;
}

void ProxyWindow::on_generateButton_clicked()
{   
    // Ask for private key passphrase

    QString passphrase;

    m_proxyController->setUseNssDb(false);

#ifdef USE_NSSDB
    if (ui->tabWidget->currentIndex()==0)
        m_proxyController->setUseNssDb(true);
    else
        m_proxyController->setUseNssDb(false);

    if (m_proxyController->useNssDb())
    {
        if (ui->NSSProfileList->currentRow()>=0)
            m_proxyController->setNssPath(ui->NSSProfileList->currentItem()->text());
    }
#endif

    if (!m_proxyController->useNssDb())
    {
        bool ok;
        passphrase = QInputDialog::getText(this, tr("Proxy generation"),
                                                   tr("Private key passphrase:"), QLineEdit::Password,
                                                   "", &ok);

        if (!ok)
            return;

        if (passphrase.length()==0)
        {
            QMessageBox::warning(this, "Proxy generation", "Passphrase can't be empy.");
            return;
        }
    }

    // Generate proxy using ArcProxyController

    QDateTime currentTime = QDateTime::currentDateTime();
    QDateTime notAfterTime = ui->proxyLifeTimeEdit->dateTime();

    m_proxyController->setValidityPeriod(currentTime.secsTo(notAfterTime));
    m_proxyController->setPassphrase(passphrase);
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

    // Close window

    this->close();
}

void ProxyWindow::on_removeButton_clicked()
{
    m_proxyController->removeProxy();
}

void ProxyWindow::on_proxyTypeCombo_currentIndexChanged(int index)
{
    if (index == 0)
        m_proxyController->setUseGSIProxy(false);
    else
        m_proxyController->setUseGSIProxy(true);
}

void ProxyWindow::on_vomsList_clicked(const QModelIndex &index)
{
    if (index.row()!=-1)
    {
        QString serverLine = ui->vomsList->currentItem()->text();
        QStringList parts = serverLine.split(":/");
        if (parts.length() == 1)
        {
            ui->vomsServerCombo->setEditText(serverLine);
            ui->vomsRoleText->setText("");
        }
        else
        {
            ui->vomsServerCombo->setEditText(parts.at(0));
            ui->vomsRoleText->setText(parts.at(1));
        }
    }
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
    if (ui->vomsList->currentRow()!=-1)
    {
        m_proxyController->removeVomsServer(ui->vomsServerCombo->currentText(), ui->vomsRoleText->text());
        QListWidgetItem* row = ui->vomsList->takeItem(ui->vomsList->currentRow());
        delete row;
    }
}

void ProxyWindow::on_addVomsServerConfig_clicked()
{
    ui->vomsConfigTable->setRowCount(ui->vomsConfigTable->rowCount()+1);
}

void ProxyWindow::on_removeVomsServerConfig_clicked()
{
    ui->vomsConfigTable->removeRow(ui->vomsConfigTable->currentRow());
}

void ProxyWindow::on_modifyVomsConfigItem_clicked()
{
    ui->vomsConfigTable->editItem(ui->vomsConfigTable->currentItem());
}


void ProxyWindow::on_vomsConfigTable_cellChanged(int row, int column)
{
    m_configTableDirty = true;
}

void ProxyWindow::on_helpButton_clicked()
{
    m_helpWindow = new HelpWindow(this);
    m_helpWindow->setWindowFlags(m_helpWindow->windowFlags() | Qt::WindowStaysOnTopHint);
    m_helpWindow->show();
}

void ProxyWindow::on_NSSProfileList_itemClicked(QListWidgetItem *item)
{
    QString selectedPath = item->text();
    m_proxyController->setNssPath(selectedPath);
}

void ProxyWindow::on_tabWidget_currentChanged(int index)
{
    if (ui->tabWidget->currentIndex()==0)
        m_proxyController->setUseNssDb(true);
    else
        m_proxyController->setUseNssDb(false);

    m_proxyController->initialize();

    ui->identityText->setText(m_proxyController->getIdentity());

    QDateTime currentTime = QDateTime::currentDateTime();
    QDateTime notValidAfter = currentTime.addSecs(12*60*60);

    ui->proxyLifeTimeEdit->setDateTime(notValidAfter);
    ui->generateButton->setFocus();

    if (m_proxyController->getUseGSIProxy())
        ui->proxyTypeCombo->setCurrentIndex(1);
    else
        ui->proxyTypeCombo->setCurrentIndex(0);

}
