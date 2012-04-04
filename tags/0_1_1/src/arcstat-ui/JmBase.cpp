#include "JmBase.h"

#include <QFileInfo>
#include <QDebug>
#include <QPainter>
#include <QPen>

JmJob::JmJob()
{
    m_id = "";
    m_name = "";
    m_state = "";
}

JmJob::~JmJob()
{

}

void JmJob::setId(QString id)
{
    m_id = id;
}

QString JmJob::id()
{
    return m_id;
}

void JmJob::setState(QString state)
{
    m_state = state;
}

QString JmJob::state()
{
    return m_state;
}

void JmJob::setName(QString name)
{
    m_name = name;
}

QString JmJob::name()
{
    return m_name;
}

JmJobList::JmJobList()
{
    m_name = "";
    m_filename = "";
}

JmJobList::~JmJobList()
{

}

void JmJobList::clear()
{
    int i;

    for (i=0; i<m_jobs.count(); i++)
        delete m_jobs.at(i);

    m_jobs.clear();
    m_jobDict.clear();
}

void JmJobList::add(JmJob* job)
{
    m_jobs.append(job);
    m_jobDict[job->id()] = job;
}

void JmJobList::add(QString id, QString name, QString state)
{
    JmJob* job = new JmJob();
    job->setId(id);
    job->setName(name);
    job->setState(state);
    this->add(job);
}

JmJob* JmJobList::at(int idx)
{
    if ((idx>=0)&&(idx<m_jobs.count()))
    {
        return m_jobs.at(idx);
    }
    else
        return 0;
}

JmJob* JmJobList::fromJobId(QString id)
{
    if (m_jobDict.contains(id))
        return m_jobDict[id];
    else
        return 0;
}

int JmJobList::count()
{
    return m_jobs.count();
}

int JmJobList::stateCount(QString state)
{
    int stateCount = 0;
    int i;

    for (i=0; i<m_jobs.count(); i++)
    {
        if (m_jobs.at(i)->state() == state)
            stateCount++;
    }

    return stateCount;
}

void JmJobList::setName(QString name)
{
    m_name = name;
}

QString JmJobList::name()
{
    return m_name;
}

void JmJobList::setFilename(QString filename)
{
    qDebug()<< "setFilename " << filename;
    if (filename!="")
    {
        m_filename = filename;
        QFileInfo fi(m_filename);
        m_name = fi.baseName();
    }
}

QString JmJobList::filename()
{
    return m_filename;
}

JmJobListDisplay::JmJobListDisplay(QWidget *parent)
    :QWidget(parent)
{
    m_jobList = 0;
}

void JmJobListDisplay::setJobList(JmJobList* jobList)
{
    m_jobList = jobList;
}

void JmJobListDisplay::paintEvent(QPaintEvent *event)
{
    if (m_jobList!=0)
    {
        QPainter p(this);
        //painter.fillRect(rect(), Qt::black);
        //painter.setPen(QPen(Qt::blue,1));

        int waitingJobs = 0;
        int runningJobs = 0;
        int finishedJobs = 0;
        int otherJobs = 0;
        int unknownJobs = 0;
        int totalJobs = m_jobList->count();

        waitingJobs += m_jobList->stateCount("Preparing");
        waitingJobs += m_jobList->stateCount("Submitting");
        waitingJobs += m_jobList->stateCount("Hold");
        waitingJobs += m_jobList->stateCount("Queuing");
        waitingJobs += m_jobList->stateCount("Finishing");
        runningJobs += m_jobList->stateCount("Running");
        finishedJobs += m_jobList->stateCount("Finished");
        otherJobs += m_jobList->stateCount("Killed");
        otherJobs += m_jobList->stateCount("Failed");
        otherJobs += m_jobList->stateCount("Deleted");
        unknownJobs = m_jobList->stateCount("Unknown");
        unknownJobs += m_jobList->stateCount("Other");

        p.setBrush(Qt::red);

        // | ----W---- | ---R--- | ---F--- | ---O--- | ---U--- |
        // 0           x1        x2        x3        x4        x5

        int x1 = int((double)(waitingJobs/(double)totalJobs) * (double)this->width());
        int x2 = int((double)((waitingJobs+runningJobs)/(double)totalJobs) * (double)this->width());
        int x3 = int((double)((waitingJobs+runningJobs+finishedJobs)/(double)totalJobs) * (double)this->width());
        int x4 = int((double)((waitingJobs+runningJobs+finishedJobs+otherJobs)/(double)totalJobs) * (double)this->width());
        int x5 = this->width() - 1;

        qDebug() << x1;
        qDebug() << x2;
        qDebug() << x3;
        qDebug() << x4;

        p.setBrush(Qt::yellow);
        p.drawRect(0, 0, x1, this->height()-1);
        p.setBrush(Qt::cyan);
        p.drawRect(x1, 0, x2-x1, this->height()-1);
        p.setBrush(Qt::green);
        p.drawRect(x2, 0, x3-x2, this->height()-1);
        p.setBrush(Qt::red);
        p.drawRect(x3, 0, x4-x3, this->height()-1);
        p.setBrush(Qt::gray);
        p.drawRect(x4, 0, x5-x4, this->height()-1);
        p.end();
    }
}

void JmJobListDisplay::resizeEvent(QResizeEvent *event)
{

}

