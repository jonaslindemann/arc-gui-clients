#include "arcjobcontroller.h"

#include <arc/ArcConfig.h>
#include <arc/ArcLocation.h>
#include <arc/DateTime.h>
#include <arc/IString.h>
#include <arc/Logger.h>
#include <arc/OptionParser.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/Utils.h>
#include <arc/XMLNode.h>
#include <arc/client/Broker.h>
#include <arc/client/JobDescription.h>
#include <arc/client/TargetGenerator.h>
#include <arc/client/Submitter.h>
#include <arc/loader/Plugin.h>
#include <arc/loader/FinderLoader.h>
#include <arc/credential/Credential.h>
#include <arc/client/JobSupervisor.h>

#include <QStringList>
#include <QTableWidget>
#include <QHeaderView>
#include <QFrame>
#include <QtConcurrentRun>
#include <QDebug>
#include <QSet>
#include <QFileDialog>

#include "qdebugstream.h"

ArcJobController::ArcJobController()
    :m_userConfig("", "", Arc::initializeCredentialsType(Arc::initializeCredentialsType::TryCredentials)), logDest(std::cout)
{
    //:m_userConfig("/home/jonas/.arc/client.conf", "/home/jonas/.arc/jobs.xml", Arc::initializeCredentialsType(Arc::initializeCredentialsType::TryCredentials))
    m_tabWidget = 0;
    m_jobSupervisor = 0;

    //Arc::ArcLocation::Init(argv[0]);

    Arc::Logger::getRootLogger().setThreshold(Arc::INFO);
    Arc::Logger::getRootLogger().removeDestinations();
    Arc::Logger::getRootLogger().addDestination(logDest);
    logDest.setFormat(Arc::ShortFormat);

    connect(&m_queryJobStatusWatcher, SIGNAL(finished()), this, SLOT(queryJobStatusFinished()));
    connect(&m_downloadJobsWatcher, SIGNAL(finished()), this, SLOT(downloadJobsFinished()));
    connect(&m_killJobsWatcher, SIGNAL(finished()), this, SLOT(killJobsFinished()));
    connect(&m_cleanJobsWatcher, SIGNAL(finished()), this, SLOT(cleanJobsFinished()));
}

ArcJobController::~ArcJobController()
{
    if (m_jobSupervisor!=0)
        delete m_jobSupervisor;
}

void ArcJobController::setTabWidget(QTabWidget* tabWidget)
{
    m_tabWidget = tabWidget;
    m_tabWidget->clear();
}

void ArcJobController::setStatusOutput(QTextEdit *statusOutput)
{
}

void ArcJobController::setup()
{
    m_tabWidget->clear();
    m_jobLists.clear();

    this->newJobList(QString(m_userConfig.JobListFile().c_str()));
}

void ArcJobController::newJobList(const QString &jobListName)
{
    if (m_tabWidget!=0)
    {
        QTableWidget* jobTable = new QTableWidget(0, 3);
        QStringList labels;
        labels << "JobID" << "Name" << "State";
        jobTable->setHorizontalHeaderLabels(labels);
        jobTable->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
        jobTable->verticalHeader()->hide();
        jobTable->setShowGrid(true);
        jobTable->setFrameStyle(QFrame::NoFrame);
        jobTable->setSelectionBehavior(QTableWidget::SelectRows);
        jobTable->setSelectionMode(QAbstractItemView::MultiSelection);

        connect(jobTable, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));

        m_tabWidget->addTab(jobTable, jobListName);
        m_jobLists.append(jobListName);
        m_jobTables.append(jobTable);

        m_currentJobTable = jobTable;
        m_currentJobListFile = jobListName;
        m_tabWidget->setCurrentIndex(m_tabWidget->count()-1);
    }
}

void ArcJobController::openJobList(const QString& jobListName)
{
    this->newJobList(jobListName);
}

void ArcJobController::setCurrentJobList(int idx)
{
    if ((idx>=0)&&(idx<m_jobLists.size()))
    {
        m_currentJobTable = m_jobTables[idx];
        m_currentJobListFile = m_jobLists[idx];
    }
}

void ArcJobController::setDownloadDir(const QString& downloadDir)
{
    m_downloadDir = downloadDir;
}


void ArcJobController::queryJobStatus()
{
    using namespace std;

    // Read existing jobs from job list file

    Arc::Job::ReadAllJobsFromFile(m_currentJobListFile.toStdString(), m_arcJobList);

    // Create job supervisor and job controllers

    if (m_jobSupervisor!=0)
        delete m_jobSupervisor;

    m_jobSupervisor = new Arc::JobSupervisor(m_userConfig, m_arcJobList);
    std::list<Arc::JobController*> jobControllers = m_jobSupervisor->GetJobControllers();
    std::list<Arc::JobController*>::iterator cit;

    std::list<std::string> status;

    std::vector<const Arc::Job*> jobs;
    for (cit=jobControllers.begin(); cit!=jobControllers.end(); cit++) {
        //cout << "Querying job controller." << endl;
        (*cit)->FetchJobs(status, jobs);
    }

    // Populate QT Table

    std::vector<const Arc::Job*>::iterator sit;

    m_currentJobTable->setRowCount(0);
    m_currentJobTable->setColumnCount(3);
    m_currentJobTable->setSortingEnabled(false);
    int currentRow = 0;

    m_jobList.clear();

    for (sit=jobs.begin(); sit!=jobs.end(); sit++) {
        //cout << (*sit)->JobID << ":" << (*sit)->State.GetGeneralState() << endl;
        m_jobList.append((*sit));
        QStringList status;
        status << QString((*sit)->JobID.fullstr().c_str()) << QString((*sit)->Name.c_str()) << QString((*sit)->State.GetGeneralState().c_str());
        m_currentJobTable->insertRow(currentRow);

        QTableWidgetItem* item = new QTableWidgetItem(status[0]);
        m_currentJobTable->setItem(currentRow, 0, item);
        m_currentJobTable->setItem(currentRow, 1, new QTableWidgetItem(status[1]));
        m_currentJobTable->setItem(currentRow, 2, new QTableWidgetItem(status[2]));

        // Store index to m_jobList in table, to handle case when table is sorted.

        item->setData(Qt::UserRole, currentRow);

        currentRow++;
    }

    m_currentJobTable->setSortingEnabled(true);

}

void ArcJobController::startQueryJobStatus()
{
    //Arc::Logger::getRootLogger().removeDestinations();
    m_queryJobStatusWatcher.setFuture(QtConcurrent::run(this, &ArcJobController::queryJobStatus));
}

void ArcJobController::startDownloadJobs()
{
    //Arc::Logger::getRootLogger().removeDestinations();
    m_downloadJobsWatcher.setFuture(QtConcurrent::run(this, &ArcJobController::getJobs));
}

void ArcJobController::startKillJobs()
{
    //Arc::Logger::getRootLogger().removeDestinations();
    m_killJobsWatcher.setFuture(QtConcurrent::run(this, &ArcJobController::killJobs));
}

void ArcJobController::startCleanJobs()
{
    //Arc::Logger::getRootLogger().removeDestinations();
    m_cleanJobsWatcher.setFuture(QtConcurrent::run(this, &ArcJobController::cleanJobs));
}

void ArcJobController::queryJobStatusFinished()
{
    qDebug() << "query finished.";
    Q_EMIT onQueryJobStatusDone();
}

void ArcJobController::downloadJobsFinished()
{
    qDebug() << "download finished.";
    Q_EMIT onDownloadJobsDone();
}

void ArcJobController::killJobsFinished()
{
    qDebug() << "download finished.";
    Q_EMIT onKillJobsDone();
}

void ArcJobController::cleanJobsFinished()
{
    qDebug() << "download finished.";
    Q_EMIT onCleanJobsDone();
}

void ArcJobController::selectAllJobs()
{
    qDebug() << "selectAllJobs";
    m_currentJobTable->selectAll();
}

void ArcJobController::clearSelection()
{
    qDebug() << "clearSelection";
    m_currentJobTable->clearSelection();
}

void ArcJobController::cleanJobs()
{
    // Return if nothing is selected

    if (m_selectedJobIds.size()==0)
        return;

    // Create job supervisor and job controllers

    qDebug() << "Creating job supervisor...";
    Arc::JobSupervisor jobSupervisor(m_userConfig, m_selectedJobIds);
    qDebug() << "Getting job controllers...";
    std::list<Arc::JobController*> jobControllers = jobSupervisor.GetJobControllers();
    std::list<Arc::JobController*>::iterator cit;

    std::list<std::string> status;

    for (cit=jobControllers.begin(); cit!=jobControllers.end(); cit++) {
        qDebug() << "Cleaning job...";
        (*cit)->Clean(status, false);
    }
}

void ArcJobController::killJobs()
{
    // Return if nothing is selected

    if (m_selectedJobIds.size()==0)
        return;

    // Create job supervisor and job controllers

    qDebug() << "Creating job supervisor...";
    Arc::JobSupervisor jobSupervisor(m_userConfig, m_selectedJobIds);
    qDebug() << "Getting job controllers...";
    std::list<Arc::JobController*> jobControllers = jobSupervisor.GetJobControllers();
    std::list<Arc::JobController*>::iterator cit;

    std::list<std::string> status;

    for (cit=jobControllers.begin(); cit!=jobControllers.end(); cit++) {
        qDebug() << "Kill job...";
        (*cit)->Kill(status, false);
    }
}

void ArcJobController::getJobs()
{
    // Return if nothing is selected

    if (m_selectedJobIds.size()==0)
        return;

    // Do we have a download dir?

    if (m_downloadDir == "")
        return;

    // Create job supervisor and job controllers

    qDebug() << "Creating job supervisor...";
    Arc::JobSupervisor jobSupervisor(m_userConfig, m_selectedJobIds);
    qDebug() << "Getting job controllers...";
    std::list<Arc::JobController*> jobControllers = jobSupervisor.GetJobControllers();
    std::list<Arc::JobController*>::iterator cit;

    std::list<std::string> status;

    for (cit=jobControllers.begin(); cit!=jobControllers.end(); cit++) {
        qDebug() << "Get job...";
        (*cit)->Get(status, m_downloadDir.toStdString(), false, false);
    }
}

void ArcJobController::itemSelectionChanged()
{   
    int i;

    m_jobSelectionIndex.clear();

    qDebug() << "-->";
    for (i=0; i<m_currentJobTable->selectedItems().count(); i++)
    {
        QTableWidgetItem* item = m_currentJobTable->selectedItems()[i];
        m_jobSelectionIndex.insert(item->row());
    }

    QSetIterator<int> it(m_jobSelectionIndex);

    m_selectedJobList.clear();
    m_selectedJobIds.clear();

    if (m_jobSelectionIndex.count()>0)
    {
        while (it.hasNext()) {
            int idx = it.next();
            QTableWidgetItem* item = m_currentJobTable->item(idx, 0);
            int realIdx = item->data(Qt::UserRole).toInt();
            qDebug() << realIdx;
            m_selectedJobList.append(m_jobList[realIdx]);
            m_selectedJobIds.push_back(m_jobList[realIdx]->JobID.str());
        }
    }

    qDebug() << "<--";
}
