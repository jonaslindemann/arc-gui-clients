#ifndef ARCJOBCONTROLLER_H
#define ARCJOBCONTROLLER_H

#include <QTabWidget>
#include <QTableWidget>
#include <QStringList>
#include <QTextEdit>
#include <QFutureWatcher>
#include <QHash>
#include <QSet>

#include <arc/UserConfig.h>
#include <arc/client/Job.h>
#include <arc/client/JobSupervisor.h>

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
    QTableWidget* m_jobTable;
    QTableWidget* m_jobListTable;

    Arc::JobSupervisor* m_jobSupervisor;
    std::list<Arc::Job> m_arcJobList;
    Arc::UserConfig m_userConfig;
    std::list<std::string> m_selectedJobIds;

    QList<JmJobList*> m_jmJobLists;
    JmJobList* m_currentJmJobList;
    QSet<int> m_jobSelectionIndex;

    QString m_downloadDir;
    Arc::LogStream logDest;
public:
    ArcJobController();
    virtual ~ArcJobController();

    void setup();

    void updateJobList();

    void setJobTable(QTableWidget* tableWidget);
    void setJobListTable(QTableWidget* tableWidget);
    void setStatusOutput(QTextEdit* statusOutput);
    void newJobList(const QString& jobListName);
    void openJobList(const QString& jobListName);

    void setDownloadDir(const QString& downloadDir);

    void setCurrentJobList(int idx);

    void queryJobStatus();
    void startQueryJobStatus();
    void startDownloadJobs();
    void startKillJobs();
    void startCleanJobs();
    void startResubmitJobs();

    void selectAllJobs();
    void clearSelection();

    void cleanJobs();
    void killJobs();
    void getJobs();
    void resubmitJobs();

private Q_SLOTS:
    void queryJobStatusFinished();
    void downloadJobsFinished();
    void killJobsFinished();
    void cleanJobsFinished();
    void resubmitJobsFinished();
    void itemSelectionChanged();

Q_SIGNALS:
    void onQueryJobStatusDone();
    void onDownloadJobsDone();
    void onKillJobsDone();
    void onCleanJobsDone();
    void onResubmitJobsDone();
};

#endif // ARCJOBCONTROLLER_H
