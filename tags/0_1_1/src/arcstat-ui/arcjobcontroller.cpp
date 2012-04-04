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
#include <QSettings>

#include "qdebugstream.h"

ArcJobController::ArcJobController()
    :m_userConfig("", "", Arc::initializeCredentialsType(Arc::initializeCredentialsType::TryCredentials)), logDest(std::cout)
{
    //:m_userConfig("/home/jonas/.arc/client.conf", "/home/jonas/.arc/jobs.xml", Arc::initializeCredentialsType(Arc::initializeCredentialsType::TryCredentials))
    m_jobTable = 0;
    m_jobSupervisor = 0;
    m_jobListTable = 0;

    this->loadState();

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
    connect(&m_queryAllJobListStatusWatcher, SIGNAL(finished()), this, SLOT(queryAllJobListStatusFinished()));
}

ArcJobController::~ArcJobController()
{
    this->saveState();

    if (m_jobSupervisor!=0)
        delete m_jobSupervisor;
}

void ArcJobController::setStatusOutput(QTextEdit *statusOutput)
{
}

void ArcJobController::setup()
{
    m_jobTable->clear();

    QStringList labels;
    labels << "JobID" << "Name" << "State";
    m_jobTable->setHorizontalHeaderLabels(labels);
    m_jobTable->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
    m_jobTable->verticalHeader()->hide();
    m_jobTable->setShowGrid(true);
    m_jobTable->setFrameStyle(QFrame::NoFrame);

    if (m_jmJobLists.count()==0)
        this->newJobList(QString(m_userConfig.JobListFile().c_str()));
    this->updateJobList();
    connect(m_jobListTable, SIGNAL(itemSelectionChanged()), this, SLOT(jobListSelectionChanged()));
    connect(m_jobTable, SIGNAL(itemSelectionChanged()), this, SLOT(jobTableSelectionChanged()));
}

void ArcJobController::saveState()
{
    qDebug() << "saveState";

    QSettings settings;

    settings.beginGroup("JobLists");
    settings.beginWriteArray("joblists");
    int i;

    for (i=0; i<m_jmJobLists.count(); i++)
    {
        settings.setArrayIndex(i);
        settings.setValue("joblist", m_jmJobLists[i]->filename());
    }
    settings.endArray();
    settings.endGroup();
    settings.sync();
}

void ArcJobController::loadState()
{
    qDebug() << "loadState";

    int i;

    for (i=0;i<m_jmJobLists.count();i++)
        delete m_jmJobLists[i];

    m_jmJobLists.clear();

    QSettings settings;

    settings.beginGroup("JobLists");
    int size = settings.beginReadArray("joblists");
    for (i=0; i<size; i++)
    {
        settings.setArrayIndex(i);
        JmJobList* jobList = new JmJobList();
        jobList->setFilename(settings.value("joblist").toString());
        m_jmJobLists.append(jobList);
    }
    settings.endArray();
    settings.endGroup();

    if (m_jmJobLists.count()>0)
        m_currentJmJobList = m_jmJobLists[0];
    else
        m_currentJmJobList = 0;
}

void ArcJobController::updateJobList()
{
    if (m_jobListTable->selectedItems().count()!=0)
        this->m_currentJobListIndex = m_jobListTable->selectedItems()[0]->row();
    else
        this->m_currentJobListIndex = -1;

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
    labels << "Job list" << "Size" << "Status (W/R/F/O/U)";
    m_jobListTable->setHorizontalHeaderLabels(labels);
    m_jobListTable->horizontalHeader()->setResizeMode(0, QHeaderView::ResizeToContents);
    m_jobListTable->horizontalHeader()->setResizeMode(1, QHeaderView::ResizeToContents);
    m_jobListTable->horizontalHeader()->setResizeMode(2, QHeaderView::Stretch);
    m_jobListTable->verticalHeader()->hide();
    m_jobListTable->setShowGrid(true);
    m_jobListTable->setFrameStyle(QFrame::NoFrame);
    m_jobListTable->setSelectionBehavior(QTableWidget::SelectRows);
    m_jobTable->setSelectionMode(QAbstractItemView::MultiSelection);

    int i;

    for (i=0; i<m_jmJobLists.count(); i++)
    {
        JmJobList* jobList = m_jmJobLists.at(i);

        QStringList tableRow;
        tableRow << jobList->name() << QString::number(jobList->count()) << "";
        m_jobListTable->insertRow(m_jobListTable->rowCount());

        QTableWidgetItem* item = new QTableWidgetItem(tableRow[0]);
        m_jobListTable->setItem(m_jobListTable->rowCount()-1, 0, item);
        m_jobListTable->setItem(m_jobListTable->rowCount()-1, 1, new QTableWidgetItem(tableRow[1]));
        m_jobListTable->setItem(m_jobListTable->rowCount()-1, 2, new QTableWidgetItem(tableRow[2]));

        m_jobListTable->item(m_jobListTable->rowCount()-1, 0)->setTextAlignment(Qt::AlignCenter);
        m_jobListTable->item(m_jobListTable->rowCount()-1, 1)->setTextAlignment(Qt::AlignCenter);
        m_jobListTable->item(m_jobListTable->rowCount()-1, 2)->setTextAlignment(Qt::AlignVCenter|Qt::AlignRight);

        JmJobListDisplay* jobDisplay = new JmJobListDisplay();
        jobDisplay->setJobList(jobList);
        m_jobListTable->setCellWidget(m_jobListTable->rowCount()-1, 2, jobDisplay);

    }

    if (m_currentJobListIndex!=-1)
        m_jobListTable->selectRow(m_currentJobListIndex);
}

void ArcJobController::updateJobTable()
{
    // Populate QT Table

    m_jobTable->setRowCount(0);
    m_jobTable->setColumnCount(3);
    m_jobTable->setSortingEnabled(false);


    QStringList labels;
    labels << "JobID" << "Name" << "State";
    m_jobTable->setHorizontalHeaderLabels(labels);
    m_jobTable->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
    m_jobTable->verticalHeader()->hide();
    m_jobTable->setShowGrid(true);
    m_jobTable->setFrameStyle(QFrame::NoFrame);

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

void ArcJobController::newJobList(const QString &jobListName)
{
    if (m_jobListTable!=0)
    {
        /*
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
        */

        JmJobList* jmJobList = new JmJobList();
        jmJobList->setFilename(jobListName);
        m_jmJobLists.append(jmJobList);

        // Store index to m_jobList in table, to handle case when table is sorted.

        m_currentJmJobList = jmJobList;

        this->updateJobList();
        this->updateJobTable();

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

void ArcJobController::queryJobStatus(JmJobList* jobList)
{
    // Read existing jobs from job list file

    Arc::Job::ReadAllJobsFromFile(jobList->filename().toStdString(), m_arcJobList);

    // Clear the current JmJobList;

    jobList->clear();

    // Create an initial JmJobList will _all_ jobs in ARC joblist

    std::list<Arc::Job>::iterator jli;

    for (jli=m_arcJobList.begin(); jli!=m_arcJobList.end(); jli++) {
        QString jobId = (*jli).JobID.fullstr().c_str();
        QString jobName = (*jli).Name.c_str();
        jobList->add(jobId, jobName, "Unknown");
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
        jobList->fromJobId(jobId)->setState(jobState);
    }
}

void ArcJobController::queryJobStatus()
{
    using namespace std;

    if (m_currentJmJobList==0)
        return;

    this->queryJobStatus(m_currentJmJobList);

    // Update job table

    this->updateJobTable();
}

void ArcJobController::queryAllJobListStatus()
{
    int i;

    for (i=0; i<m_jmJobLists.count(); i++)
        this->queryJobStatus(m_jmJobLists[i]);

}

void ArcJobController::startQueryJobStatus()
{
    //Arc::Logger::getRootLogger().removeDestinations();
    m_queryJobStatusWatcher.setFuture(QtConcurrent::run(this, &ArcJobController::queryJobStatus));
}

void ArcJobController::startQueryAllJobListStatus()
{
    //Arc::Logger::getRootLogger().removeDestinations();
    m_queryAllJobListStatusWatcher.setFuture(QtConcurrent::run(this, &ArcJobController::queryAllJobListStatus));
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
    this->updateJobTable();
    Q_EMIT onQueryJobStatusDone();
}

void ArcJobController::queryAllJobListStatusFinished()
{
    qDebug() << "query finished.";
    this->updateJobList();
    this->updateJobTable();
    Q_EMIT onQueryAllJobListStatusDone();
}

void ArcJobController::downloadJobsFinished()
{
    qDebug() << "download finished.";
    this->updateJobList();
    this->updateJobTable();
    Q_EMIT onDownloadJobsDone();
}

void ArcJobController::killJobsFinished()
{
    qDebug() << "download finished.";
    this->updateJobList();
    this->updateJobTable();
    Q_EMIT onKillJobsDone();
}

void ArcJobController::cleanJobsFinished()
{
    qDebug() << "download finished.";
    this->updateJobList();
    this->updateJobTable();
    Q_EMIT onCleanJobsDone();
}

void ArcJobController::resubmitJobsFinished()
{
    qDebug() << "download finished.";
    this->updateJobList();
    this->updateJobTable();
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

    //-------------------------------------------------

    bool all = false;
    std::string joblist;
    std::string jobidfileout;
    std::list<std::string> jobidfilesin;
    std::list<std::string> clusters;
    std::list<std::string> qlusters;
    std::list<std::string> indexurls;
    bool keep = false;
    bool same = false;
    std::list<std::string> status;
    bool show_plugins = false;
    int timeout = -1;
    std::string conffile;
    std::string debug;
    std::string broker;

    // Different selected services are needed in two different context,
    // so the two copies of UserConfig objects will contain different
    // selected services.
    Arc::UserConfig usercfg2 = m_userConfig;

    if (m_jobSupervisor!=0)
        delete m_jobSupervisor;

    m_jobSupervisor = new Arc::JobSupervisor(m_userConfig, m_selectedJobIds);
    //Arc::JobSupervisor m_jobSupervisor(m_userConfig, m_selectedJobIds);
    if (!m_jobSupervisor->JobsFound()) {
      std::cout << Arc::IString("No jobs") << std::endl;
      return;
    }
    std::list<Arc::JobController*> jobcont = m_jobSupervisor->GetJobControllers();

    // If the user specified a joblist on the command line joblist equals
    // usercfg.JobListFile(). If not use the default, ie. usercfg.JobListFile().
    if (jobcont.empty()) {
        qDebug() << "No job controller plugins loaded";
      return;
    }

    // Clearing jobs.
    m_selectedJobIds.clear();

    std::list<Arc::Job> toberesubmitted;
    for (std::list<Arc::JobController*>::iterator it = jobcont.begin();
         it != jobcont.end(); it++) {
      std::list<Arc::Job> cont_jobs;
      cont_jobs = (*it)->GetJobDescriptions(status, true);
      toberesubmitted.insert(toberesubmitted.begin(), cont_jobs.begin(), cont_jobs.end());
    }
    if (toberesubmitted.empty()) {
        qDebug() << "No jobs to resubmit";
      return;
    }

    if (same) {
      qlusters.clear();
      usercfg2.ClearSelectedServices();
    }
    else if (!qlusters.empty() || !indexurls.empty())
      usercfg2.ClearSelectedServices();

    // Preventing resubmitted jobs to be send to old clusters

    for (std::list<Arc::Job>::iterator it = toberesubmitted.begin();
         it != toberesubmitted.end(); it++)
      if (same) {
        qlusters.push_back(it->Flavour + ":" + it->Cluster.str());
        qDebug() << "Trying to resubmit job to " << it->Cluster.str().c_str();
      }
      else {
        qlusters.remove(it->Flavour + ":" + it->Cluster.str());
        qlusters.push_back("-" + it->Flavour + ":" + it->Cluster.str());
        qDebug() << "Disregarding " << it->Cluster.str().c_str();
      }
    qlusters.sort();
    qlusters.unique();

    usercfg2.AddServices(qlusters, Arc::COMPUTING);
    if (!same && !indexurls.empty())
      usercfg2.AddServices(indexurls, Arc::INDEX);

    // Resubmitting jobs
    Arc::TargetGenerator targen(usercfg2);
    targen.RetrieveExecutionTargets();

    if (targen.GetExecutionTargets().empty()) {
      std::cout << Arc::IString("Job submission aborted because no resource returned any information") << std::endl;
      return;
    }

    Arc::BrokerLoader loader;
    Arc::Broker *ChosenBroker = loader.load(m_userConfig.Broker().first, usercfg2);
    if (!ChosenBroker) {
        qDebug() << "Unable to load broker " << usercfg2.Broker().first.c_str();
      return;
    }

    std::list<Arc::Job> resubmittedJobs;

    // Loop over jobs
    for (std::list<Arc::Job>::iterator it = toberesubmitted.begin();
         it != toberesubmitted.end(); it++) {
      resubmittedJobs.push_back(Arc::Job());

      std::list<Arc::JobDescription> jobdescs;
      Arc::JobDescription::Parse(it->JobDescriptionDocument, jobdescs); // Do not check for validity. We are only interested in that the outgoing job description is valid.
      if (jobdescs.empty()) {
        std::cout << Arc::IString("Job resubmission failed, unable to parse obtained job description") << std::endl;
        resubmittedJobs.pop_back();
        continue;
      }
      jobdescs.front().Identification.ActivityOldId = it->ActivityOldID;
      jobdescs.front().Identification.ActivityOldId.push_back(it->JobID.str());

      // remove the queuename which was added during the original submission of the job
      jobdescs.front().Resources.QueueName = "";

      if (ChosenBroker->Submit(targen.GetExecutionTargets(), jobdescs.front(), resubmittedJobs.back())) {
        std::string jobid = resubmittedJobs.back().JobID.str();
        if (!jobidfileout.empty())
          if (!Arc::Job::WriteJobIDToFile(jobid, jobidfileout))
              qDebug() << "Cannot write jobid " << jobid.c_str() << " to file " << jobidfileout.c_str();
        m_selectedJobIds.push_back(it->JobID.str());
      }
      else {
        std::cout << Arc::IString("Job resubmission failed, no more possible targets") << std::endl;
        resubmittedJobs.pop_back();
      }
    } //end loop over all job descriptions

    if (!Arc::Job::WriteJobsToFile(m_userConfig.JobListFile(), resubmittedJobs)) {
      std::cout << Arc::IString("Warning: Failed to lock job list file %s", m_userConfig.JobListFile())
                << std::endl;
      std::cout << Arc::IString("To recover missing jobs, run arcsync") << std::endl;
    }

    if (m_selectedJobIds.empty())
      return;

    //m_userConfig.ClearSelectedServices();

    // Only kill and clean jobs that have been resubmitted

    if (m_jobSupervisor!=0)
        delete m_jobSupervisor;

    m_jobSupervisor = new Arc::JobSupervisor(m_userConfig, m_selectedJobIds);

    //Arc::JobSupervisor killmaster(m_userConfig, m_selectedJobIds);
    if (!m_jobSupervisor->JobsFound()) {
      std::cout << Arc::IString("No jobs") << std::endl;
      return;
    }
    std::list<Arc::JobController*> killcont = m_jobSupervisor->GetJobControllers();
    if (killcont.empty()) {
        qDebug() << "No job controller plugins loaded";
      return;
    }

    for (std::list<Arc::JobController*>::iterator it = killcont.begin();
         it != killcont.end(); it++) {
      // Order matters.
      if (!(*it)->Kill(status, keep) && !keep && !(*it)->Clean(status, true)) {
          qDebug() << "Job could not be killed or cleaned";
      }
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

void ArcJobController::jobTableSelectionChanged()
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
            m_selectedJobIds.push_back(m_currentJmJobList->at(realIdx)->id().toStdString());
            qDebug() << m_currentJmJobList->at(realIdx)->id();
        }
    }

    qDebug() << "<--";
}

void ArcJobController::jobListSelectionChanged()
{
    qDebug() << "jobListSelectionChanged()";

    if (m_jobListTable->selectedItems().count()>0)
    {
        m_currentJmJobList = m_jmJobLists[m_jobListTable->selectedItems()[0]->row()];
        m_userConfig.JobListFile(m_currentJmJobList->filename().toStdString());
        this->updateJobTable();
    }
}

void ArcJobController::removeSelectedJobList()
{
    if (m_currentJmJobList!=0)
    {
        m_jmJobLists.removeOne(m_currentJmJobList);
        delete m_currentJmJobList;
        if (m_jmJobLists.count()>0)
            m_currentJmJobList = m_jmJobLists.first();
        else
            m_currentJmJobList = 0;

        this->updateJobList();
        this->updateJobTable();
    }
}
