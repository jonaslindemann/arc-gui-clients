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

class ArcJobController : public QObject
{
    Q_OBJECT
private:
    QTabWidget* m_tabWidget;
    QStringList m_jobLists;
    QString m_currentJobListFile;
    QList<QTableWidget*> m_jobTables;
    QFutureWatcher<void> m_queryJobStatusWatcher;
    QFutureWatcher<void> m_downloadJobsWatcher;
    QFutureWatcher<void> m_killJobsWatcher;
    QFutureWatcher<void> m_cleanJobsWatcher;
    QFuture<void> m_queryJobStatusFuture;
    QTableWidget* m_currentJobTable;

    Arc::JobSupervisor* m_jobSupervisor;
    std::list<Arc::Job> m_arcJobList;
    Arc::UserConfig m_userConfig;
    QList<const Arc::Job*> m_jobList;
    QSet<int> m_jobSelectionIndex;
    QList<const Arc::Job*> m_selectedJobList;
    std::list<std::string> m_selectedJobIds;

    QString m_downloadDir;
    Arc::LogStream logDest;
public:
    ArcJobController();
    virtual ~ArcJobController();

    void setup();

    void setTabWidget(QTabWidget* tabWidget);
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

    void selectAllJobs();
    void clearSelection();

    void cleanJobs();
    void killJobs();
    void getJobs();

private Q_SLOTS:
    void queryJobStatusFinished();
    void downloadJobsFinished();
    void killJobsFinished();
    void cleanJobsFinished();
    void itemSelectionChanged();

Q_SIGNALS:
    void onQueryJobStatusDone();
    void onDownloadJobsDone();
    void onKillJobsDone();
    void onCleanJobsDone();
};

#endif // ARCJOBCONTROLLER_H
