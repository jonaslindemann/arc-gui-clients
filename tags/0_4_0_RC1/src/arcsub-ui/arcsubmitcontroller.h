#ifndef __arcsubmitcontroller_h__
#define __arcsubmitcontroller_h__

#include <list>

#include <QObject>
#include <QString>
#include <QFutureWatcher>

#include <arc/client/JobDescription.h>

class ArcSubmitController : public QObject
{
    Q_OBJECT

private:
    QString m_jobListFilename;
    std::list<Arc::JobDescription> m_jobDescriptions;

    QFutureWatcher<void> m_submissionWatcher;

public:
    ArcSubmitController();
    virtual ~ArcSubmitController();

    void setJobListFilename(QString filename);

    void addJobDescription(Arc::JobDescription jobDescription);
    void clear();

    void startSubmission();

    int submit();

private Q_SLOTS:
    void submissionFinished();

Q_SIGNALS:
    void onSubmissionFinished();
    void onSubmissionStatus(int currentJobId, int totalJobs, QString text);
};

#endif
