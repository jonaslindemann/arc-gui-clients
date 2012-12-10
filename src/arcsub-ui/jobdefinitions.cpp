#include "jobdefinitions.h"

#include <QDir>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QSettings>

/*!
   Delete a directory along with all of its contents.

   \param dirName Path of directory to remove.
   \return true on success; false on error.
*/
bool removeDir(const QString &dirName)
{
    bool result = true;
    QDir dir(dirName);

    if (dir.exists(dirName)) {
        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            if (info.isDir()) {
                result = removeDir(info.absoluteFilePath());
            }
            else {
                result = QFile::remove(info.absoluteFilePath());
            }

            if (!result) {
                return result;
            }
        }
        result = dir.rmdir(dirName);
    }

    return result;
}

bool removeDirs(const QString &dirName, const QString &dirFilter)
{
    bool result = true;
    QDir dir(dirName);

    Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
        result = true;
        if (info.isDir()) {

            QString dirName = info.baseName();

            if (dirName.indexOf(dirFilter)!=-1)
                result = removeDir(info.absoluteFilePath());
        }
        if (!result) {
            return result;
        }
    }

    return result;
}


JobDefinitionBase::JobDefinitionBase(QObject *parent, QString name) :
    QObject(parent), m_jobDescription()
{
    m_name = name;
    m_jobDescription.Identification.JobName = name.toStdString();
    m_xrsl = "";
    m_wallTime = 3600;
    m_memory = 2000;
}

Arc::JobDescription& JobDefinitionBase::jobDescription()
{
    return m_jobDescription;
}


void JobDefinitionBase::setExecutable(QString name)
{
    m_jobDescription.Application.Executable.Path = name.toStdString();
}

QString JobDefinitionBase::executable()
{
    QString executable = m_jobDescription.Application.Executable.Path.c_str();
    return executable;
}

void JobDefinitionBase::clearArguments()
{
    m_jobDescription.Application.Executable.Argument.clear();
}

void JobDefinitionBase::addArgument(QString argument)
{
    m_jobDescription.Application.Executable.Argument.push_back(argument.toStdString());
}

void JobDefinitionBase::clearRuntimes()
{
    m_jobDescription.Resources.RunTimeEnvironment.clear();
}

void JobDefinitionBase::addRuntime(QString runtimeName, QString runtimeVersion)
{
    QString RE = runtimeName + "-" + runtimeVersion;
    m_jobDescription.Resources.RunTimeEnvironment.add(RE.toStdString(), Arc::Software::GREATERTHAN);
}

void JobDefinitionBase::setName(QString name)
{
    m_name = name;
}

QString JobDefinitionBase::name()
{
    return m_name;
}

void JobDefinitionBase::clearInputFiles()
{
    m_inputFiles.clear();
    m_inputFileUrls.clear();
}

void JobDefinitionBase::addInputFile(QString filename, QString sourceLocation)
{
    m_inputFiles.append(filename);
    m_inputFileUrls.append(sourceLocation);
}

void JobDefinitionBase::clearOutputFiles()
{
    m_outputFiles.clear();
    m_outputFileUrls.clear();
}

void JobDefinitionBase::addOutputFile(QString filename, QString targetLocation)
{
    m_outputFiles.append(filename);
    m_outputFileUrls.append(targetLocation);
}

int JobDefinitionBase::inputFileCount()
{
    return m_inputFiles.count();
}

QString JobDefinitionBase::inputFileAt(int idx)
{
    if ((idx>=0)&&(idx<m_inputFiles.count()))
        return m_inputFiles.at(idx);
}

QString JobDefinitionBase::inputFileSourceAt(int idx)
{
    if ((idx>=0)&&(idx<m_inputFileUrls.count()))
        return m_inputFileUrls.at(idx);
}

void JobDefinitionBase::removeInputFile(int idx)
{
    if ((idx>=0)&&(idx<m_inputFileUrls.count()))
    {
        m_inputFiles.removeAt(idx);
        m_inputFileUrls.removeAt(idx);
    }
}

int JobDefinitionBase::outputFileCount()
{
    return m_outputFiles.count();
}

QString JobDefinitionBase::outputFileAt(int idx)
{
    if ((idx>=0)&&(idx<m_outputFiles.count()))
        return m_outputFiles.at(idx);
}

QString JobDefinitionBase::outputFileSourceAt(int idx)
{
    if ((idx>=0)&&(idx<m_outputFileUrls.count()))
        return m_outputFileUrls.at(idx);
}

void JobDefinitionBase::removeOutputFile(int idx)
{
    if ((idx>=0)&&(idx<m_outputFileUrls.count()))
    {
        m_outputFiles.removeAt(idx);
        m_outputFileUrls.removeAt(idx);
    }
}

void JobDefinitionBase::setWalltime(int walltime)
{
    m_wallTime = walltime;
}

int JobDefinitionBase::walltime()
{
    return m_wallTime;
}

void JobDefinitionBase::setMemory(int m)
{
    m_memory = m;
}

int JobDefinitionBase::memory()
{
    return m_memory;
}

void JobDefinitionBase::setParamSize(int nSize)
{
    m_paramSize = nSize;
}

int JobDefinitionBase::paramSize()
{
    return m_paramSize;
}

void JobDefinitionBase::setEmail(QString email)
{
    m_email = email;
}

QString JobDefinitionBase::email()
{
    return m_email;
}

void JobDefinitionBase::setupJobDir(QString createPath)
{
    if (createPath.length()==0)
        m_jobDir = QDir::currentPath()+"/"+m_name+".jobdef";
    else
        m_jobDir = createPath+"/"+m_name+".jobdef";

    qDebug() << "Creating : " << m_jobDir;
    if (!QDir(m_jobDir).exists())
        QDir().mkdir(m_jobDir);
}

void JobDefinitionBase::setupJobDescription()
{
    m_jobDescription.Resources.IndividualPhysicalMemory = m_memory;
    m_jobDescription.Resources.TotalWallTime.range.max = m_wallTime;
    m_jobDescription.Resources.TotalWallTime.range.min = m_wallTime;

    m_jobDescription.DataStaging.InputFiles.clear();
    for (int i=0; i<m_inputFiles.count(); i++)
    {
        m_jobDescription.DataStaging.InputFiles.push_front(Arc::InputFileType());
        m_jobDescription.DataStaging.InputFiles.front().Name = m_inputFiles[i].toStdString();
    }

    m_jobDescription.DataStaging.OutputFiles.clear();
    for (int i=0; i<m_outputFiles.count(); i++)
    {
        m_jobDescription.DataStaging.OutputFiles.push_front(Arc::OutputFileType());
        m_jobDescription.DataStaging.OutputFiles.front().Name = m_outputFiles[i].toStdString();
    }
}

void JobDefinitionBase::setupParamDirs()
{
    // Clean any existing param directories

    ::removeDirs(m_jobDir, "param");

    for (int i=0; i<m_paramSize; i++)
    {
        // Create the parameter directory

        QString numberString;
        numberString.sprintf("%03d", i);
        QString paramDir = "param"+numberString;
        if (!QDir(m_jobDir+"/"+paramDir).exists())
            QDir().mkdir(m_jobDir+"/"+paramDir);

        qDebug() << "Creating : " << m_jobDir+"/"+paramDir;

        // Define job name

        QString jobName = m_name+numberString;
        m_jobDescription.Identification.JobName = jobName.toStdString();


        // Write job description file in parameter directory

        QFile xrslFile(m_jobDir+"/"+paramDir+"/"+jobName+".xrsl");
        xrslFile.open(QFile::WriteOnly);
        QTextStream out(&xrslFile);
        out << xrslString() << endl;
        xrslFile.close();

        // Create job script file

        this->doCreateRunScript(m_jobDir+"/"+paramDir+"/run.sh", i, m_paramSize, jobName);

    }
}

void JobDefinitionBase::clear()
{
    m_inputFiles.clear();
    m_outputFiles.clear();
}

bool JobDefinitionBase::setup()
{
    this->setupJobDir();
    this->setupJobDescription();
    this->setupParamDirs();
    return true;
}

bool JobDefinitionBase::load(QString jobDefDir)
{
    if (QDir(jobDefDir).exists())
    {
        if (QFile(jobDefDir+"/config.ini").exists())
        {
            m_jobDir = jobDefDir;
            QSettings jobDefConfig(m_jobDir+"/config.ini", QSettings::IniFormat);

            jobDefConfig.clear();

            jobDefConfig.beginGroup("Information");
            m_name = jobDefConfig.value("Name", "Noname").toString();
            m_email = jobDefConfig.value("Email","").toString();
            jobDefConfig.endGroup();

            jobDefConfig.beginGroup("ParameterSweep");
            m_paramSize = jobDefConfig.value("Size", 5).toInt();
            jobDefConfig.endGroup();

            jobDefConfig.beginGroup("InputFiles");
            for (int i=0; i<jobDefConfig.childKeys().count()/2; i++)
            {
                QString inputFile = jobDefConfig.value("inputFile"+QString::number(i), "").toString();
                QString inputFileSource = jobDefConfig.value("inputFileSource"+QString::number(i), "").toString();
                if (inputFile.length()!=0)
                    this->addInputFile(inputFile, inputFileSource);
            }
            jobDefConfig.endGroup();

            jobDefConfig.beginGroup("OutputFiles");
            for (int i=0; i<jobDefConfig.childKeys().count()/2; i++)
            {
                QString outputFile = jobDefConfig.value("outputFile"+QString::number(i), "").toString();
                QString outputFileSource = jobDefConfig.value("outputFileSource"+QString::number(i), "").toString();
                if (outputFile.length()!=0)
                    this->addOutputFile(outputFile, outputFileSource);
            }
            jobDefConfig.endGroup();
        }
        else
            return false;
    }
    else
        return false;
}

bool JobDefinitionBase::save(QString saveDir)
{
    this->setupJobDir(saveDir);
    this->setupJobDescription();
    this->setupParamDirs();

    QSettings jobDefConfig(m_jobDir+"/config.ini", QSettings::IniFormat);

    jobDefConfig.beginGroup("Information");
    jobDefConfig.setValue("Name", m_name);
    jobDefConfig.setValue("Email", m_email);
    jobDefConfig.endGroup();

    jobDefConfig.beginGroup("ParameterSweep");
    jobDefConfig.setValue("Size", m_paramSize);
    jobDefConfig.endGroup();

    jobDefConfig.beginGroup("InputFiles");
    for (int i=0; i<m_inputFiles.count(); i++)
    {
        jobDefConfig.setValue("inputFile"+QString::number(i), m_inputFiles[i]);
        jobDefConfig.setValue("inputFileSource"+QString::number(i), m_inputFileUrls[i]);
    }
    jobDefConfig.endGroup();

    jobDefConfig.beginGroup("OutputFiles");
    for (int i=0; i<m_outputFiles.count(); i++)
    {
        jobDefConfig.setValue("OutputFiles"+QString::number(i), m_outputFiles[i]);
        jobDefConfig.setValue("OutputFileSource"+QString::number(i), m_outputFileUrls[i]);
    }
    jobDefConfig.endGroup();

    return true;
}

void JobDefinitionBase::print()
{
    this->setupJobDescription();
    m_jobDescription.UnParse(m_xrsl, "nordugrid:xrsl");
    std::cout << m_xrsl << std::endl;
}

QString JobDefinitionBase::xrslString()
{
    this->setupJobDescription();
    m_jobDescription.UnParse(m_xrsl, "nordugrid:xrsl");
    QString returnString = m_xrsl.c_str();
    return returnString;
}

void JobDefinitionBase::doCreateRunScript(QString scriptFilename, int paramNumber, int paramSize, QString jobName)
{
    QFile scriptFile(scriptFilename);
    scriptFile.open(QFile::WriteOnly);
    QTextStream out(&scriptFile);

    out << "#!/bin/sh" << endl;
    out << "echo Job : " << jobName << endl;
    out << "echo I am " << paramNumber << " of " << paramSize << endl;

    scriptFile.close();
}


ShellScriptJob::ShellScriptJob(QObject *parent, QString name) :
    JobDefinitionBase(parent, name)
{
    this->setExecutable("/bin/sh");
    this->addArgument("./run.sh");
    this->addInputFile("run.sh");
}

void ShellScriptJob::setScript(QStringList script)
{
    m_script = script;
}

QStringList ShellScriptJob::getScript()
{
    return m_script;
}
