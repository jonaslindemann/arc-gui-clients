#include "jobinfo.h"

JobInfo::JobInfo()
{
}

QString JobInfo::getId()
{
    return m_id;
}

void JobInfo::setId(QString id)
{
    m_id = id;
}

QString JobInfo::getName()
{
    return m_name;
}

void JobInfo::setName(QString name)
{
    m_name = name;
}

QString JobInfo::getStatus()
{
    return m_status;
}

void setStatus(QString status)
{
    m_status = status;
}
