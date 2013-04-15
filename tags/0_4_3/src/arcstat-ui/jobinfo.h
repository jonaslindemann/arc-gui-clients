#ifndef JOBINFO_H
#define JOBINFO_H

#include <QString>

class JobInfo
{
private:
    QString m_id;
    QString m_name;
    QString m_status;
public:
    JobInfo();

    QString getId();
    void setId(QString id);

    QString getName();
    void setName(QString name);

    QString getStatus();
    void setStatus(QString status);

};

#endif // JOBINFO_H
