#include "jobstatuswindow.h"
#include "ui_jobstatuswindow.h"

#include <QTableWidget>
#include <QDebug>
#include <QFileDialog>

#include "arcjobcontroller.h"

JobStatusWindow::JobStatusWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::JobStatusWindow)
{
    m_firstShow = true;
    ui->setupUi(this);

    // Redirect standard output

    m_debugStream = new QDebugStream(std::cout, this);
    m_debugStream2 = new QDebugStream(std::cerr, this);

    // Change splitter

    ui->leftRightSplitter->setStretchFactor(0,1);
    ui->leftRightSplitter->setStretchFactor(1,2);
    ui->jobListTable->setColumnWidth(2, 100);

    ui->topBottomSplitter->setStretchFactor(0,4);

    QCoreApplication::setOrganizationName("Lunarc");
    QCoreApplication::setOrganizationDomain("lunarc.lu.se");
    QCoreApplication::setApplicationName("ARC Job Manager");

    // Create job controller

    m_jobController = new ArcJobController();
    m_jobController->setJobTable(ui->jobTable);
    m_jobController->setJobListTable(ui->jobListTable);
    m_jobController->setup();

    connect(m_jobController, SIGNAL(onQueryJobStatusDone()), this, SLOT(onQueryJobStatusDone()));
    connect(m_jobController, SIGNAL(onDownloadJobsDone()), this, SLOT(onDownloadJobsDone()));
    connect(m_jobController, SIGNAL(onKillJobsDone()), this, SLOT(onKillJobsDone()));
    connect(m_jobController, SIGNAL(onCleanJobsDone()), this, SLOT(onCleanJobsDone()));
    connect(m_jobController, SIGNAL(onResubmitJobsDone()), this, SLOT(onResubmitJobsDone()));
    connect(m_jobController, SIGNAL(onQueryAllJobListStatusDone()), this, SLOT(onQueryAllJobListStatusDone()));

    qDebug() << QApplication::arguments();
}

JobStatusWindow::~JobStatusWindow()
{
    delete m_jobController;
    delete m_debugStream;
    delete m_debugStream2;
    delete ui;
}

void JobStatusWindow::disableActions()
{
    ui->jobTable->setDisabled(true);
    ui->actionCleanSelected->setDisabled(true);
    ui->actionDownloadSelected->setDisabled(true);
    ui->actionKillSelected->setDisabled(true);
    ui->actionRefresh->setDisabled(true);
    ui->actionResubmitSelected->setDisabled(true);
}

void JobStatusWindow::enableActions()
{
    ui->jobTable->setEnabled(true);
    ui->actionCleanSelected->setEnabled(true);
    ui->actionDownloadSelected->setEnabled(true);
    ui->actionKillSelected->setEnabled(true);
    ui->actionRefresh->setEnabled(true);
    ui->actionResubmitSelected->setEnabled(true);
}

void JobStatusWindow::customEvent(QEvent * event)
{
    // When we get here, we've crossed the thread boundary and are now
    // executing in the Qt object's thread

    if(event->type() == DEBUG_STREAM_EVENT)
    {
        handleDebugStreamEvent(static_cast<DebugStreamEvent *>(event));
    }

    // use more else ifs to handle other custom events
}

void JobStatusWindow::handleDebugStreamEvent(const DebugStreamEvent *event)
{
    // Now you can safely do something with your Qt objects.
    // Access your custom data using event->getCustomData1() etc.

    ui->logText->append(event->getOutputText());
}

void JobStatusWindow::showEvent ( QShowEvent * event )
{
    if (m_firstShow)
    {
        m_firstShow = false;

        if (QApplication::arguments().length()>1)
        {
            QString filename = QApplication::arguments()[1];

            if (!QFile(filename).exists())
                return;
            else
            {
                qDebug() << "Opening: " << filename;
                this->statusBar()->showMessage("Querying job status...");
                this->enableActions();
                m_jobController->openJobList(filename);
                //m_jobController->startQueryJobStatus();
            }
        }

        this->disableActions();
        m_jobController->startQueryAllJobListStatus();
    }
}

void JobStatusWindow::on_actionRefresh_triggered()
{
    this->statusBar()->showMessage("Querying job status...");
    this->disableActions();
    m_jobController->startQueryJobStatus();
}

void JobStatusWindow::onQueryJobStatusDone()
{
    this->statusBar()->showMessage("");
    this->enableActions();
    qDebug() << "onQueryJobStatusDone()";
}

void JobStatusWindow::onQueryAllJobListStatusDone()
{
    this->statusBar()->showMessage("");
    this->enableActions();
    qDebug() << "onQueryAllJobListStatusDone()";
}

void JobStatusWindow::onDownloadJobsDone()
{
    this->statusBar()->showMessage("");
    this->enableActions();
    qDebug() << "onDownloadJobsDone()";
    m_jobController->startQueryJobStatus();
}

void JobStatusWindow::onKillJobsDone()
{
    this->statusBar()->showMessage("");
    this->enableActions();
    qDebug() << "onKillJobsDone()";
    m_jobController->startQueryJobStatus();
}

void JobStatusWindow::onCleanJobsDone()
{
    this->statusBar()->showMessage("");
    this->enableActions();
    qDebug() << "onCleanJobsDone()";
    m_jobController->startQueryJobStatus();
}

void JobStatusWindow::onResubmitJobsDone()
{
    this->statusBar()->showMessage("");
    this->enableActions();
    qDebug() << "onResubmitJobsDone()";
    m_jobController->startQueryJobStatus();
}

void JobStatusWindow::on_actionOpenJobList_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this,
         tr("Open job list file"), "", tr("Job list files (*.xml)"));

    if (filename!="")
    {
        this->statusBar()->showMessage("Querying job status...");
        this->enableActions();
        m_jobController->openJobList(filename);
        m_jobController->startQueryJobStatus();
    }
}

void JobStatusWindow::on_actionSelectAll_triggered()
{
    m_jobController->selectAllJobs();
}

void JobStatusWindow::on_actionClearSelection_triggered()
{
    m_jobController->clearSelection();
}

void JobStatusWindow::on_actionCleanSelected_triggered()
{
    this->disableActions();
    m_jobController->startCleanJobs();
    this->statusBar()->showMessage("Cleaning jobs...");
}

void JobStatusWindow::on_actionKillSelected_triggered()
{
    this->disableActions();
    m_jobController->startKillJobs();
    this->statusBar()->showMessage("Killing jobs...");
}

void JobStatusWindow::on_actionDownloadSelected_triggered()
{
    QString dirName = QFileDialog::getExistingDirectory(this, "Select download directory");

    if (dirName=="")
        return;

    m_jobController->setDownloadDir(dirName);

    this->disableActions();
    m_jobController->startDownloadJobs();
    this->statusBar()->showMessage("Downloading jobs...");
}

void JobStatusWindow::on_actionExit_triggered()
{
    this->close();
}

void JobStatusWindow::on_actionResubmitSelected_triggered()
{
    this->disableActions();
    m_jobController->startResubmitJobs();
    this->statusBar()->showMessage("Resubmitting jobs...");
}

void JobStatusWindow::on_actionRemoveJobList_triggered()
{
    m_jobController->removeSelectedJobList();
}

void JobStatusWindow::on_actionShowFiles_triggered()
{
    m_jobController->openSessionDir();
}
