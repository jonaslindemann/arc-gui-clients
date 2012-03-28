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
#include <QFileInfo>
#include <QPixmap>
#include <QPainter>
#include <QLabel>

#include "qdebugstream.h"

ArcJobController::ArcJobController()
    :m_userConfig("", "", Arc::initializeCredentialsType(Arc::initializeCredentialsType::TryCredentials)), logDest(std::cout)
{
    //:m_userConfig("/home/jonas/.arc/client.conf", "/home/jonas/.arc/jobs.xml", Arc::initializeCredentialsType(Arc::initializeCredentialsType::TryCredentials))
    m_jobTable = 0;
    m_jobSupervisor = 0;
    m_jobListTable = 0;

    //Arc::ArcLocation::Init(argv[0]);

    Arc::Logger::getRootLogger().setThreshold(Arc::INFO);
    Arc::Logger::getRootLogger().removeDestinations();
    Arc::Logger::getRootLogger().addDestination(logDest);
    logDest.setFormat(Arc::ShortFormat);

    connect(&m_queryJobStatusWatcher, SIGNAL(finished()), this, SLOT(queryJobStatusFinished()));
    connect(&m_downloadJobsWatcher, SIGNAL(finished()), this, SLOT(downloadJobsFinished()));
    connect(&m_killJobsWatcher, SIGNAL(finished()), this, SLOT(killJobsFinished()));
    connect(&m_cleanJobsWatcher, SIGNAL(finished()), this, SLOT(cleanJobsFinished()));
    connect(&m_resubmitJobsWatcher, SIGNAL(finished()), this, SLOT(resubmitJobsFinished()));
}

ArcJobController::~ArcJobController()
{
    if (m_jobSupervisor!=0)
        delete m_jobSupervisor;
}

void ArcJobController::setStatusOutput(QTextEdit *statusOutput)
{
}

void ArcJobController::setup()
{
    m_jobTable->clear();
    this->newJobList(QString(m_userConfig.JobListFile().c_str()));
    this->updateJobList();
}

void ArcJobController::updateJobList()
{
    m_jobListTable->clear();
    m_jobListTable->setRowCount(0);
    m_jobListTable->setColumnCount(3);

    /*
    JOBSTATE_X(ACCEPTED, "Accepted")\      W
    JOBSTATE_X(PREPARING, "Preparing")\    W
    JOBSTATE_X(SUBMITTING, "Submitting")\  W
    JOBSTATE_X(HOLD, "Hold")\              W
    JOBSTATE_X(QUEUING, "Queuing")\        W
    JOBSTATE_X(RUNNING, "Running")\        R
    JOBSTATE_X(FINISHING, "Finishing")\    W
    JOBSTATE_X(FINISHED, "Finished")\      F
    JOBSTATE_X(KILLED, "Killed")\          O
    JOBSTATE_X(FAILED, "Failed")\          O
    JOBSTATE_X(DELETED, "Deleted")\        O
    JOBSTATE_X(OTHER, "Other")             O
    */

    QStringList labels;
    labels << "Job list" << "Size" << "Status (W/R/F/O)";
    m_jobListTable->setHorizontalHeaderLabels(labels);
    m_jobListTable->horizontalHeader()->setResizeMode(0, QHeaderView::ResizeToContents);
    m_jobListTable->horizontalHeader()->setResizeMode(1, QHeaderView::ResizeToContents);
    m_jobListTable->horizontalHeader()->setResizeMode(2, QHeaderView::Stretch);
    m_jobListTable->verticalHeader()->hide();
    m_jobListTable->setShowGrid(true);
    m_jobListTable->setFrameStyle(QFrame::NoFrame);
    m_jobListTable->setSelectionBehavior(QTableWidget::SelectRows);
    //m_jobTable->setSelectionMode(QAbstractItemView::MultiSelection);

    int i;

    for (i=0; i<m_jmJobLists.count(); i++)
    {
        JmJobList* jobList = m_jmJobLists.at(i);

        QString statusString;

        int waitingJobs = 0;
        int runningJobs = 0;
        int finishedJobs = 0;
        int otherJobs = 0;
        int unknownJobs = 0;
        int totalJobs = jobList->count();

        waitingJobs += jobList->stateCount("Preparing");
        waitingJobs += jobList->stateCount("Submitting");
        waitingJobs += jobList->stateCount("Hold");
        waitingJobs += jobList->stateCount("Queuing");
        waitingJobs += jobList->stateCount("Finishing");
        runningJobs += jobList->stateCount("Running");
        finishedJobs += jobList->stateCount("Finished");
        otherJobs += jobList->stateCount("Killed");
        otherJobs += jobList->stateCount("Failed");
        otherJobs += jobList->stateCount("Deleted");
        unknownJobs = jobList->stateCount("Unknown");
        unknownJobs += jobList->stateCount("Other");

        int barLength = 100;
        int barHeight = 20;

        QPixmap* pix = new QPixmap(barLength, barHeight);
        QPainter p(pix);
        p.setBrush(Qt::red);

        // | ----W---- | ---R--- | ---F--- | ---O--- | ---U--- |
        // 0           x1        x2        x3        x4        x5

        int x1 = int((double)(waitingJobs/(double)totalJobs) * (double)barLength);
        int x2 = int((double)((waitingJobs+runningJobs)/(double)totalJobs) * (double)barLength);
        int x3 = int((double)((waitingJobs+runningJobs+finishedJobs)/(double)totalJobs) * (double)barLength);
        int x4 = int((double)((waitingJobs+runningJobs+finishedJobs+otherJobs)/(double)totalJobs) * (double)barLength);
        int x5 = barLength - 1;

        qDebug() << x1;
        qDebug() << x2;
        qDebug() << x3;
        qDebug() << x4;

        p.setBrush(Qt::yellow);
        p.drawRect(0, 0, x1, barHeight-1);
        p.setBrush(Qt::cyan);
        p.drawRect(x1, 0, x2-x1, barHeight-1);
        p.setBrush(Qt::green);
        p.drawRect(x2, 0, x3-x2, barHeight-1);
        p.setBrush(Qt::red);
        p.drawRect(x3, 0, x4-x3, barHeight-1);
        p.setBrush(Qt::gray);
        p.drawRect(x4, 0, x5-x4, barHeight-1);
        p.end();

        statusString += QString::number(waitingJobs);
        statusString += "/" + QString::number(runningJobs);
        statusString += "/" + QString::number(finishedJobs);
        statusString += "/" + QString::number(otherJobs);
        statusString += "/" + QString::number(unknownJobs);

        QStringList tableRow;
        tableRow << jobList->name() << QString::number(jobList->count()) << statusString;
        m_jobListTable->insertRow(m_jobListTable->rowCount());

        QTableWidgetItem* item = new QTableWidgetItem(tableRow[0]);
        m_jobListTable->setItem(m_jobListTable->rowCount()-1, 0, item);
        m_jobListTable->setItem(m_jobListTable->rowCount()-1, 1, new QTableWidgetItem(tableRow[1]));
        m_jobListTable->setItem(m_jobListTable->rowCount()-1, 2, new QTableWidgetItem(tableRow[2]));

        m_jobListTable->item(m_jobListTable->rowCount()-1, 0)->setTextAlignment(Qt::AlignCenter);
        m_jobListTable->item(m_jobListTable->rowCount()-1, 1)->setTextAlignment(Qt::AlignCenter);
        m_jobListTable->item(m_jobListTable->rowCount()-1, 2)->setTextAlignment(Qt::AlignVCenter|Qt::AlignRight);

        //QIcon icon(pix.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation));

        QLabel* lbl = new QLabel();
        lbl->setPixmap(*pix);
        lbl->setBaseSize(barLength,barHeight);
        lbl->setContentsMargins(10,0,10,0);
        m_jobListTable->setCellWidget(m_jobListTable->rowCount()-1, 2, lbl);
        //m_jobListTable->item(m_jobListTable->rowCount()-1, 2)->setIcon(icon);

    }
}

void ArcJobController::newJobList(const QString &jobListName)
{
    if (m_jobListTable!=0)
    {
        //QTableWidget* jobTable = new QTableWidget(0, 3);
        m_jobTable->clear();
        m_jobTable->setRowCount(0);
        m_jobTable->setColumnCount(3);
        QStringList labels;
        labels << "JobID" << "Name" << "State";
        m_jobTable->setHorizontalHeaderLabels(labels);
        m_jobTable->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
        m_jobTable->verticalHeader()->hide();
        m_jobTable->setShowGrid(true);
        m_jobTable->setFrameStyle(QFrame::NoFrame);
        m_jobTable->setSelectionBehavior(QTableWidget::SelectRows);
        m_jobTable->setSelectionMode(QAbstractItemView::MultiSelection);

        connect(m_jobTable, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));

        JmJobList* jmJobList = new JmJobList();
        jmJobList->setFilename(jobListName);
        m_jmJobLists.append(jmJobList);

        // Store index to m_jobList in table, to handle case when table is sorted.

        m_currentJmJobList = jmJobList;

        this->updateJobList();

    }
}

void ArcJobController::setJobTable(QTableWidget *tableWidget)
{
    m_jobTable = tableWidget;
}

void ArcJobController::setJobListTable(QTableWidget *tableWidget)
{
    m_jobListTable = tableWidget;
}

void ArcJobController::openJobList(const QString& jobListName)
{
    this->newJobList(jobListName);
}

void ArcJobController::setCurrentJobList(int idx)
{
    if ((idx>=0)&&(idx<m_jmJobLists.size()))
    {
        m_currentJmJobList = m_jmJobLists[idx];
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

    Arc::Job::ReadAllJobsFromFile(m_currentJmJobList->filename().toStdString(), m_arcJobList);

    // Clear the current JmJobList;

    m_currentJmJobList->clear();

    // Create an initial JmJobList will _all_ jobs in ARC joblist

    std::list<Arc::Job>::iterator jli;

    for (jli=m_arcJobList.begin(); jli!=m_arcJobList.end(); jli++) {
        QString jobId = (*jli).JobID.fullstr().c_str();
        QString jobName = (*jli).Name.c_str();
        m_currentJmJobList->add(jobId, jobName, "Unknown");
    }

    // Create job supervisor and job controllers

    if (m_jobSupervisor!=0)
        delete m_jobSupervisor;

    m_jobSupervisor = new Arc::JobSupervisor(m_userConfig, m_arcJobList);
    std::list<Arc::JobController*> jobControllers = m_jobSupervisor->GetJobControllers();

    // Query status

    std::list<std::string> status;
    std::vector<const Arc::Job*> jobs;
    std::list<Arc::JobController*>::iterator cit;
    for (cit=jobControllers.begin(); cit!=jobControllers.end(); cit++) {
        //cout << "Querying job controller." << endl;
        (*cit)->FetchJobs(status, jobs);
    }

    // Update JmJobList

    std::vector<const Arc::Job*>::iterator sit;

    for (sit=jobs.begin(); sit!=jobs.end(); sit++) {
        QString jobId = (*sit)->JobID.fullstr().c_str();
        QString jobState = (*sit)->State.GetGeneralState().c_str();
        m_currentJmJobList->fromJobId(jobId)->setState(jobState);
    }

    // Populate QT Table

    m_jobTable->setRowCount(0);
    m_jobTable->setColumnCount(3);
    m_jobTable->setSortingEnabled(false);
    int currentRow = 0;

    int i;

    for (i=0; i<m_currentJmJobList->count(); i++) {

        QStringList status;
        status << m_currentJmJobList->at(i)->id() << m_currentJmJobList->at(i)->name() << m_currentJmJobList->at(i)->state();

        m_jobTable->insertRow(currentRow);
        QTableWidgetItem* item = new QTableWidgetItem(status[0]);
        m_jobTable->setItem(currentRow, 0, item);
        m_jobTable->setItem(currentRow, 1, new QTableWidgetItem(status[1]));
        m_jobTable->setItem(currentRow, 2, new QTableWidgetItem(status[2]));

        if (m_currentJmJobList->at(i)->state() == "Finished")
            m_jobTable->item(currentRow, 2)->setBackgroundColor(Qt::green);
        else if (m_currentJmJobList->at(i)->state() == "Failed")
            m_jobTable->item(currentRow, 2)->setBackgroundColor(Qt::red);
        else if (m_currentJmJobList->at(i)->state() == "Unknown")
            m_jobTable->item(currentRow, 2)->setBackgroundColor(Qt::gray);
        else if (m_currentJmJobList->at(i)->state() == "Running")
            m_jobTable->item(currentRow, 2)->setBackgroundColor(Qt::cyan);
        else
            m_jobTable->item(currentRow, 2)->setBackgroundColor(Qt::yellow);

        m_jobTable->item(currentRow, 1)->setTextAlignment(Qt::AlignCenter);
        m_jobTable->item(currentRow, 2)->setTextAlignment(Qt::AlignCenter);

        // Store index to m_jobList in table, to handle case when table is sorted.

        item->setData(Qt::UserRole, currentRow);

        currentRow++;
    }

    m_jobTable->setSortingEnabled(true);

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

void ArcJobController::startResubmitJobs()
{
    //Arc::Logger::getRootLogger().removeDestinations();
    m_resubmitJobsWatcher.setFuture(QtConcurrent::run(this, &ArcJobController::resubmitJobs));
}

void ArcJobController::queryJobStatusFinished()
{
    qDebug() << "query finished.";
    this->updateJobList();
    Q_EMIT onQueryJobStatusDone();
}

void ArcJobController::downloadJobsFinished()
{
    qDebug() << "download finished.";
    this->updateJobList();
    Q_EMIT onDownloadJobsDone();
}

void ArcJobController::killJobsFinished()
{
    qDebug() << "download finished.";
    this->updateJobList();
    Q_EMIT onKillJobsDone();
}

void ArcJobController::cleanJobsFinished()
{
    qDebug() << "download finished.";
    this->updateJobList();
    Q_EMIT onCleanJobsDone();
}

void ArcJobController::resubmitJobsFinished()
{
    qDebug() << "download finished.";
    this->updateJobList();
    Q_EMIT onResubmitJobsDone();
}

void ArcJobController::selectAllJobs()
{
    qDebug() << "selectAllJobs";
    m_jobTable->selectAll();
}

void ArcJobController::clearSelection()
{
    qDebug() << "clearSelection";
    m_jobTable->clearSelection();
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

void ArcJobController::resubmitJobs()
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
        //(*cit)->Resubmit(status, false);
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
    for (i=0; i<m_jobTable->selectedItems().count(); i++)
    {
        QTableWidgetItem* item = m_jobTable->selectedItems()[i];
        m_jobSelectionIndex.insert(item->row());
    }

    QSetIterator<int> it(m_jobSelectionIndex);

    //m_selectedJobList.clear();
    m_selectedJobIds.clear();

    if (m_jobSelectionIndex.count()>0)
    {
        while (it.hasNext()) {
            int idx = it.next();
            QTableWidgetItem* item = m_jobTable->item(idx, 0);
            int realIdx = item->data(Qt::UserRole).toInt();
            qDebug() << realIdx;
            //m_selectedJobList.append(m_jobList[realIdx]);
            //m_selectedJobIds.push_back(m_jobList[realIdx]->JobID.str());
            m_selectedJobIds.push_back(m_currentJmJobList->at(idx)->id().toStdString());
            qDebug() << m_currentJmJobList->at(idx)->id();
        }
    }

    qDebug() << "<--";
}
