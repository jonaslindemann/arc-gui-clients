#include "JmBase.h"

#include <QFileInfo>

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

