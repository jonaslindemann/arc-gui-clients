#ifndef ARCJOBCONTROLLER_H
#define ARCJOBCONTROLLER_H

#include <QTabWidget>
#include <QTableWidget>
#include <QStringList>
#include <QTextEdit>
#include <QFutureWatcher>
#include <QHash>
#include <QSet>
#include <QTableWidgetItem>

#include <arc/UserConfig.h>
#include <arc/client/Job.h>
#include <arc/client/JobSupervisor.h>

#define ARC_VERSION_2

#include "JmBase.h"

class ArcJobController : public QObject
{
    Q_OBJECT
private:

    QFutureWatcher<void> m_queryJobStatusWatcher;
    QFutureWatcher<void> m_downloadJobsWatcher;
    QFutureWatcher<void> m_killJobsWatcher;
    QFutureWatcher<void> m_cleanJobsWatcher;
    QFutureWatcher<void> m_resubmitJobsWatcher;
    QFutureWatcher<void> m_queryAllJobListStatusWatcher;
    QTableWidget* m_jobTable;
    QTableWidget* m_jobListTable;

    Arc::JobSupervisor* m_jobSupervisor;
    std::list<Arc::Job> m_arcJobList;
    Arc::UserConfig m_userConfig;
    std::list<std::string> m_selectedJobIds;

    QList<JmJobList*> m_jmJobLists;
    JmJobList* m_currentJmJobList;
    QSet<int> m_jobSelectionIndex;
    int m_currentJobListIndex;

    QString m_downloadDir;
    Arc::LogStream logDest;
    void updateJobList();
    void updateJobTable();

public:
    ArcJobController();
    virtual ~ArcJobController();

    void setup();

    void setJobTable(QTableWidget* tableWidget);
    void setJobListTable(QTableWidget* tableWidget);
    void setStatusOutput(QTextEdit* statusOutput);

    void setDownloadDir(const QString& downloadDir);
    void setCurrentJobList(int idx);

    void newJobList(const QString& jobListName);
    void openJobList(const QString& jobListName);
    void removeSelectedJobList();
    void queryJobStatus(JmJobList* jobList);
    void queryJobStatus();
    void queryAllJobListStatus();
    void cleanJobs();
    void killJobs();
    void getJobs();
    void resubmitJobs();
    void selectAllJobs();
    void clearSelection();

    void saveState();
    void loadState();

    void startQueryJobStatus();
    void startQueryAllJobListStatus();
    void startDownloadJobs();
    void startKillJobs();
    void startCleanJobs();
    void startResubmitJobs();

private Q_SLOTS:
    void queryJobStatusFinished();
    void queryAllJobListStatusFinished();
    void downloadJobsFinished();
    void killJobsFinished();
    void cleanJobsFinished();
    void resubmitJobsFinished();
    void jobTableSelectionChanged();
    void jobListSelectionChanged();

Q_SIGNALS:
    void onQueryJobStatusDone();
    void onDownloadJobsDone();
    void onKillJobsDone();
    void onCleanJobsDone();
    void onResubmitJobsDone();
    void onQueryAllJobListStatusDone();
};

#endif // ARCJOBCONTROLLER_H
