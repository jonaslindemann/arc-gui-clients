#include "jobdefinitions.h"

JobDefinitionBase::JobDefinitionBase(QObject *parent, QString name) :
    QObject(parent), m_jobDescription()
{
    m_name = name;
    m_jobDescription.Identification.JobName = name.toStdString();
    m_xrsl = "";
    m_jobDescription.Resources.TotalWallTime = 3600;
}

Arc::JobDescription& JobDefinitionBase::jobDescription()
{
    return m_jobDescription;
}


void JobDefinitionBase::setExecutable(QString name)
{
    m_jobDescription.Application.Executable.Path = name.toStdString();
}

QString JobDefinitionBase::getExecutable()
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
    m_jobDescription.Identification.JobName = name.toStdString();
}

QString JobDefinitionBase::getName()
{
    return m_name;
}

void JobDefinitionBase::addInputFile(QString filename, QString sourceLocation)
{
    m_jobDescription.DataStaging.InputFiles.push_front(Arc::InputFileType());
    m_jobDescription.DataStaging.InputFiles.front().Name = filename.toStdString();
}

void JobDefinitionBase::addOutputFile(QString filename, QString targetLocation)
{
    m_jobDescription.DataStaging.OutputFiles.push_front(Arc::OutputFileType());
    m_jobDescription.DataStaging.OutputFiles.front().Name = filename.toStdString();
}

void JobDefinitionBase::setWalltime(int t)
{
    m_jobDescription.Resources.TotalWallTime.range.max = t;
    m_jobDescription.Resources.TotalWallTime.range.min = t;
}

int JobDefinitionBase::getWalltime()
{
    return m_jobDescription.Resources.TotalWallTime.range.max;
}

void JobDefinitionBase::setMemory(int m)
{
    m_jobDescription.Resources.IndividualPhysicalMemory = m;
    m_jobDescription.Resources.IndividualPhysicalMemory = m;
}

int JobDefinitionBase::getMemory()
{
    return m_jobDescription.Resources.IndividualPhysicalMemory;
}

bool JobDefinitionBase::setup()
{
    return true;
}

bool JobDefinitionBase::load()
{
    return true;
}

bool JobDefinitionBase::save()
{
    return true;
}

void JobDefinitionBase::print()
{
    m_jobDescription.UnParse(m_xrsl, "nordugrid:xrsl");
    std::cout << m_xrsl << std::endl;
}

QString JobDefinitionBase::xrslString()
{
    m_jobDescription.UnParse(m_xrsl, "nordugrid:xrsl");
    QString returnString = m_xrsl.c_str();
    return returnString;
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
