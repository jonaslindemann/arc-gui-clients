#include "proxywindow.h"

#include <QMessageBox>

#include "arcproxy-utils.h"
#include "ui_proxywindow.h"

ProxyWindow::ProxyWindow(QWidget *parent, ArcProxyController* proxyController) :
    QMainWindow(parent),
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

void ProxyWindow::closeEvent(QCloseEvent *event)
{
    qDebug() << "closeEvent";
    event->accept();
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
        QMessageBox::warning(this, "Proxy generation", "Failed to create a proxy.");
    else
        QMessageBox::information(this, "Proxy generation", "A proxy certificate has been generated.");

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
        QMessageBox::warning(this, "Proxy generation", "Failed to create a proxy.");
    else
        QMessageBox::information(this, "Proxy generation", "A proxy certificate has been generated.");

    this->close();
}

void ProxyWindow::on_proxyTypeCombo_currentIndexChanged(int index)
{
    if (index == 0)
        m_proxyController->setUseGSIProxy(false);
    else
        m_proxyController->setUseGSIProxy(true);
}
