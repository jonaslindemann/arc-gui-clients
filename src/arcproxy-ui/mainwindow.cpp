#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Redirect standard output

    m_debugStream = new QDebugStream(std::cout, this);
    m_debugStream2 = new QDebugStream(std::cerr, this);

    // Setup proxy controller

    m_proxyController.initialize();
    ui->identityText->setText(m_proxyController.getIdentity());
    ui->passphraseText->setFocus();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_debugStream;
    delete m_debugStream2;
}

void MainWindow::customEvent(QEvent * event)
{
    // When we get here, we've crossed the thread boundary and are now
    // executing in the Qt object's thread

    if(event->type() == DEBUG_STREAM_EVENT)
    {
        handleDebugStreamEvent(static_cast<DebugStreamEvent *>(event));
    }

    // use more else ifs to handle other custom events
}

void MainWindow::handleDebugStreamEvent(const DebugStreamEvent *event)
{
    // Now you can safely do something with your Qt objects.
    // Access your custom data using event->getCustomData1() etc.

    ui->logText->append(event->getOutputText());
}

void MainWindow::on_generateButton_clicked()
{
    m_proxyController.setPassphrase(ui->passphraseText->text());
    int result = m_proxyController.generateProxy();

    if (result!=EXIT_SUCCESS)
    {
        QMessageBox::warning(this, "Proxy generation", "Failed to create a proxy.");
    }
}

void MainWindow::on_removeButton_clicked()
{
    m_proxyController.removeProxy();
}
