#ifndef JMBASE_H
#define JMBASE_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QWidget>

class JmJob : public QObject
{
    Q_OBJECT
private:
    QString m_id;
    QString m_state;
    QString m_name;
public:
    JmJob();
    virtual ~JmJob();

    void setId(QString id);
    QString id();

    void setState(QString state);
    QString state();

    void setName(QString name);
    QString name();
};

class JmJobList : public QObject
{
    Q_OBJECT
private:
    QString m_name;
    QString m_filename;
    QList<JmJob*> m_jobs;
    QHash<QString, JmJob*> m_jobDict;
public:
    JmJobList();
    virtual ~JmJobList();

    void clear();
    void add(JmJob* job);
    void add(QString id, QString name, QString state);
    int count();

    JmJob* at(int idx);
    JmJob* fromJobId(QString id);

    void setName(QString name);
    QString name();

    void setFilename(QString filename);
    QString filename();

    int stateCount(QString state);
};

class JmJobListDisplay : public QWidget
{
    Q_OBJECT
private:
    JmJobList* m_jobList;
public:
    JmJobListDisplay(QWidget *parent = 0);

    void setJobList(JmJobList* jobList);

public Q_SLOTS:

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
};

#endif // JMBASE_H
