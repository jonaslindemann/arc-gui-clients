#ifndef JOBDEFINITIONS_H
#define JOBDEFINITIONS_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QSettings>

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
    QString m_jobDir;
    QStringList m_inputFiles;
    QStringList m_inputFileUrls;
    QStringList m_outputFiles;
    QStringList m_outputFileUrls;
    QStringList m_runtimeEnvironments;
    int m_wallTime;
    int m_memory;
    QString m_email;
    QString m_executable;

    void setupJobDir(QString createPath = "");
    void setupParamDirs();
    void setupJobDescription();
public:
    explicit JobDefinitionBase(QObject *parent = 0, QString name = "");

    void setParamSize(int nSize);
    int paramSize();

    void setExecutable(QString name);
    QString executable();

    void clearArguments();
    void addArgument(QString argument);

    void setName(QString name);
    QString name();

    void setEmail(QString email);
    QString email();

    void clearInputFiles();
    void addInputFile(QString filename, QString sourceLocation = "");
    int inputFileCount();
    QString inputFileAt(int idx);
    QString inputFileSourceAt(int idx);
    void removeInputFile(int idx);

    void clearOutputFiles();
    void addOutputFile(QString filename, QString targetLocation = "");
    int outputFileCount();
    QString outputFileAt(int idx);
    QString outputFileSourceAt(int idx);
    void removeOutputFile(int idx);

    void clearRuntimes();
    void addRuntime(QString runtimeName, QString runtimeVersion);
    int runtimeCount();
    QString runtimeAt(int idx);
    void removeRuntime(int idx);

    void setWalltime(int t);
    int walltime();

    void setMemory(int m);
    int memory();

    void clear();
    bool setup();
    bool load(QString jobDefDir);
    bool save(QString saveDir);
    void print();

    QString xrslString(QString jobName="");
    Arc::JobDescription& jobDescription();

    Arc::JobDescription& jobDescriptionParam(int i);

protected:
    virtual void doCreateRunScript(QString scriptFilename, int paramNumber, int paramSize, QString jobName);
    virtual void doSaveSettings(QSettings& settings);
    virtual void doLoadSettings(QSettings& settings);
    
Q_SIGNALS:
    
public Q_SLOTS:
    
};

class ShellScriptDefinition : public JobDefinitionBase
{
    Q_OBJECT
private:
    QString m_script;
public:
    explicit ShellScriptDefinition(QObject *parent = 0, QString name = "");

    void setScript(QString script);
    QString script();

protected:
    virtual void doCreateRunScript(QString scriptFilename, int paramNumber, int paramSize, QString jobName);
    virtual void doSaveSettings(QSettings& settings);
    virtual void doLoadSettings(QSettings& settings);
};

#endif // JOBDEFINITIONS_H
