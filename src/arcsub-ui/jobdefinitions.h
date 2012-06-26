#ifndef JOBDEFINITIONS_H
#define JOBDEFINITIONS_H

#include <QObject>
#include <QString>
#include <QStringList>

#include <arc/client/JobDescription.h>

// Directory/file structure
//
// directory [m_jobName].jobdef]
//             |
//             +-- [inputFiles]
//             +-- run.sh [template]
//             +-- dir param000
//             |        ...
//             +-- dir param[m_paraSize-1]
//                      |
//                      +-- run.sh [subtituted %(paramId)d %(paramSize)d]

class JobDefinitionBase : public QObject
{
    Q_OBJECT
private:
    Arc::JobDescription m_jobDescription;
    int m_paramSize;
    QString m_name;
    std::string m_xrsl;
public:
    explicit JobDefinitionBase(QObject *parent = 0, QString name = "");

    void setExecutable(QString name);
    QString getExecutable();

    void clearArguments();
    void addArgument(QString argument);

    void setName(QString name);
    QString getName();

    void clearInputFiles();
    void addInputFile(QString filename, QString sourceLocation = "");

    void clearOutputFiles();
    void addOutputFile(QString filename, QString targetLocation = "");

    void clearRuntimes();
    void addRuntime(QString runtimeName, QString runtimeVersion);

    void setWalltime(int t);
    int getWalltime();

    void setMemory(int m);
    int getMemory();

    bool setup();
    bool load();
    bool save();

    void print();
    QString xrslString();

    Arc::JobDescription& jobDescription();

    
Q_SIGNALS:
    
public Q_SLOTS:
    
};

class ShellScriptJob : public JobDefinitionBase
{
    Q_OBJECT
private:
    QStringList m_script;
public:
    explicit ShellScriptJob(QObject *parent = 0, QString name = "");

    void setScript(QStringList script);
    QStringList getScript();
};

#endif // JOBDEFINITIONS_H
