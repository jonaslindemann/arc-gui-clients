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
    m_paramSize = 5;
    m_executable = "run.sh";
    m_processorCount = 1;
    m_sweepType = ST_SINGLE_INPUT;
}

Arc::JobDescription& JobDefinitionBase::jobDescription()
{
    return m_jobDescription;
}


void JobDefinitionBase::setExecutable(QString name)
{
    m_executable = name;
}

QString JobDefinitionBase::executable()
{
    return m_executable;
}

void JobDefinitionBase::setProcessorCount(int count)
{
    m_processorCount = count;
}

int JobDefinitionBase::processorCount()
{
    return m_processorCount;
}

void JobDefinitionBase::setSweepType(TSweepType sweepType)
{
    m_sweepType = sweepType;
}

JobDefinitionBase::TSweepType JobDefinitionBase::sweepType()
{
    return m_sweepType;
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
    m_runtimeEnvironments.clear();
}

void JobDefinitionBase::addRuntime(QString runtimeName, QString runtimeVersion)
{
    if (runtimeVersion.length()!=0)
        m_runtimeEnvironments.append(runtimeName + "-" + runtimeVersion);
    else
        m_runtimeEnvironments.append(runtimeName);
}

int JobDefinitionBase::runtimeCount()
{
    return m_runtimeEnvironments.count();
}

QString JobDefinitionBase::runtimeAt(int idx)
{
    if ((idx>=0)&&(idx<m_runtimeEnvironments.count()))
        return m_runtimeEnvironments[idx];
}

void JobDefinitionBase::removeRuntime(int idx)
{
    if ((idx>=0)&&(idx<m_runtimeEnvironments.count()))
        m_runtimeEnvironments.removeAt(idx);
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
    if (sourceLocation.length()==0)
    {
        m_inputFiles.append(QFileInfo(filename).fileName());
        m_inputFileUrls.append(filename);
    }
    else
    {
        m_inputFiles.append(filename);
        m_inputFileUrls.append(sourceLocation);
    }
}

void JobDefinitionBase::setInputFileAt(int idx, QString filename, QString sourceLocation)
{
    if ((idx>=0)&&(idx<m_inputFiles.count()))
    {
        m_inputFiles[idx] = filename;
        if (sourceLocation.length()>0)
            m_inputFileUrls[idx] = sourceLocation;
    }
}

void JobDefinitionBase::setInputSourceAt(int idx, QString sourceLocation)
{
    if ((idx>=0)&&(idx<m_inputFiles.count()))
        m_inputFileUrls[idx] = sourceLocation;
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

//------

void JobDefinitionBase::clearPerJobFiles()
{
    m_perJobFiles.clear();
    m_perJobFileUrls.clear();
}

void JobDefinitionBase::addPerJobFile(QString filename, QString sourceLocation)
{
    if (sourceLocation.length()==0)
    {
        m_perJobFiles.append(QFileInfo(filename).fileName());
        m_perJobFileUrls.append(filename);
    }
    else
    {
        m_perJobFiles.append(filename);
        m_perJobFileUrls.append(sourceLocation);
    }
}

void JobDefinitionBase::setPerJobFileAt(int idx, QString filename, QString sourceLocation)
{
    if ((idx>=0)&&(idx<m_perJobFiles.count()))
    {
        m_perJobFiles[idx] = filename;
        if (sourceLocation.length()>0)
            m_perJobFileUrls[idx] = sourceLocation;
    }
}

void JobDefinitionBase::setPerJobSourceAt(int idx, QString sourceLocation)
{
    if ((idx>=0)&&(idx<m_perJobFiles.count()))
        m_perJobFileUrls[idx] = sourceLocation;
}

int JobDefinitionBase::perJobFileCount()
{
    return m_perJobFiles.count();
}

QString JobDefinitionBase::perJobFileAt(int idx)
{
    if ((idx>=0)&&(idx<m_perJobFiles.count()))
        return m_perJobFiles.at(idx);
}

QString JobDefinitionBase::perJobFileSourceAt(int idx)
{
    if ((idx>=0)&&(idx<m_perJobFileUrls.count()))
        return m_perJobFileUrls.at(idx);
}

void JobDefinitionBase::removePerJobFile(int idx)
{
    if ((idx>=0)&&(idx<m_perJobFileUrls.count()))
    {
        m_perJobFiles.removeAt(idx);
        m_perJobFileUrls.removeAt(idx);
    }
}

//-----

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

void JobDefinitionBase::setOutputFileAt(int idx, QString filename, QString targetLocation)
{
    if ((idx>=0)&&(idx<m_outputFiles.count()))
    {
        m_outputFileUrls[idx] = filename;
        if (targetLocation.length()>0)
            m_outputFileUrls[idx] = targetLocation;
    }
}

void JobDefinitionBase::setOutputTargetAt(int idx, QString targetLocation)
{
    if ((idx>=0)&&(idx<m_outputFiles.count()))
        m_outputFileUrls[idx] = targetLocation;
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

QString JobDefinitionBase::outputFileTargetAt(int idx)
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

QString JobDefinitionBase::jobDir()
{
    return m_jobDir;
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

void JobDefinitionBase::setupJobDescription(int param)
{
    m_jobDescription.Identification.JobName = m_name.toStdString();
    m_jobDescription.Resources.IndividualPhysicalMemory = m_memory;
    m_jobDescription.Resources.TotalWallTime.range.max = m_wallTime;
    m_jobDescription.Resources.TotalWallTime.range.min = m_wallTime;
    m_jobDescription.Resources.SlotRequirement.NumberOfSlots = m_processorCount;

    m_jobDescription.Application.Executable.Path = m_executable.toStdString();
    m_jobDescription.Application.Output = "stdout.txt";
    m_jobDescription.Application.Error = "stderr.txt";

    m_jobDescription.DataStaging.InputFiles.clear();
    for (int i=0; i<m_inputFiles.count(); i++)
    {
        QString inputFilename = m_inputFiles[i];
        QString inputFileSource = m_inputFileUrls[i];
        if (param!=-1)
            this->doProcessInputFile(inputFilename, inputFileSource, param, m_paramSize, this->name());

        m_jobDescription.DataStaging.InputFiles.push_front(Arc::InputFileType());
        m_jobDescription.DataStaging.InputFiles.front().Name = inputFilename.toStdString();
        if (!inputFileSource.contains(":"))
            m_jobDescription.DataStaging.InputFiles.front().Sources.push_back(Arc::URL("file:////"+inputFileSource.toStdString()));
        else
            m_jobDescription.DataStaging.InputFiles.front().Sources.push_back(Arc::URL(inputFileSource.toStdString()));

    }

    if ((m_perJobFiles.count()>0)&&(param!=-1))
    {
        if (param<m_perJobFiles.count())
        {
            QString inputFilename = m_perJobFiles[param];
            QString inputFileSource = m_perJobFileUrls[param];
            if (param!=-1)
                this->doProcessInputFile(inputFilename, inputFileSource, param, m_paramSize, this->name());

            m_jobDescription.DataStaging.InputFiles.push_front(Arc::InputFileType());
            m_jobDescription.DataStaging.InputFiles.front().Name = inputFilename.toStdString();
            if (!inputFileSource.contains(":"))
                m_jobDescription.DataStaging.InputFiles.front().Sources.push_back(Arc::URL("file:////"+inputFileSource.toStdString()));
            else
                m_jobDescription.DataStaging.InputFiles.front().Sources.push_back(Arc::URL(inputFileSource.toStdString()));
        }
    }

    m_jobDescription.DataStaging.OutputFiles.clear();
    for (int i=0; i<m_outputFiles.count(); i++)
    {
        QString outputFilename = m_outputFiles[i];
        QString outputFileTarget = m_outputFileUrls[i];
        if (param!=-1)
            this->doProcessOutputFile(outputFilename, outputFileTarget, param, m_paramSize, this->name());

        m_jobDescription.DataStaging.OutputFiles.push_front(Arc::OutputFileType());
        m_jobDescription.DataStaging.OutputFiles.front().Name = outputFilename.toStdString();
        m_jobDescription.DataStaging.OutputFiles.front().Targets.push_back(Arc::URL(outputFileTarget.toStdString()));
    }

    if (m_outputFiles.count()==0)
    {
        m_jobDescription.DataStaging.OutputFiles.push_front(Arc::OutputFileType());
        m_jobDescription.DataStaging.OutputFiles.front().Name = "/";
    }
    m_jobDescription.Application.LogDir = "joblog";

    m_jobDescription.Resources.RunTimeEnvironment.clear();

    for (int i=0; i<m_runtimeEnvironments.count(); i++)
        m_jobDescription.Resources.RunTimeEnvironment.add(m_runtimeEnvironments[i].toStdString(), Arc::Software::GREATERTHANOREQUAL);
}

Arc::JobDescription& JobDefinitionBase::jobDescriptionParam(int i)
{
    this->setupJobDescription(i);

    QString numberString;
    numberString.sprintf("%03d", i);

    QString paramDir = "param"+numberString;

    // Define job name

    QString jobName = m_name+"-"+numberString;
    m_jobDescription.Identification.JobName = jobName.toStdString();
    m_jobDescription.Application.Executable.Path = "run.sh";

    m_jobDescription.DataStaging.InputFiles.push_front(Arc::InputFileType());
    m_jobDescription.DataStaging.InputFiles.front().Name = m_executable.toStdString();
    m_jobDescription.DataStaging.InputFiles.front().Sources.push_back(Arc::URL( ("file://"+m_jobDir+"/"+paramDir+"/"+"run.sh").toStdString()));

    return m_jobDescription;
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

        QString jobName = m_name+"-"+numberString;
        m_jobDescription.Identification.JobName = jobName.toStdString();

        // Write job description file in parameter directory

        /*
        QFile xrslFile(m_jobDir+"/"+paramDir+"/job.xrsl");
        xrslFile.open(QFile::WriteOnly);
        QTextStream out(&xrslFile);
        out << xrslString(jobName) << endl;
        xrslFile.close();
        */

        /*
        // Create symbolic links to input files

        for (int j=0; j<m_inputFiles.count(); j++)
        {
            if (m_inputFileUrls[j].length()==0)
            {
                QFileInfo fileInfo(m_inputFiles[j]);
                QString destinationFile = m_jobDir + QDir::separator() + paramDir + QDir::separator() + fileInfo.fileName();
                QFile::link(m_inputFiles[j], destinationFile);
            }
        }
        */
        // Create job script file

        QString perJobFilename;
        if (i<m_perJobFiles.count())
            perJobFilename = m_perJobFiles.at(i);
        else
            perJobFilename = "";

        QString script;

        QFile scriptFile(m_jobDir+"/"+paramDir+"/run.sh");
        scriptFile.open(QFile::WriteOnly);

        QTextStream out(&scriptFile);

        this->doCreateRunScript(i, m_paramSize, jobName, perJobFilename, script);
        out << script;

        scriptFile.close();
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

            jobDefConfig.beginGroup("Information");
            m_name = jobDefConfig.value("Name", "Noname").toString();
            m_email = jobDefConfig.value("Email","").toString();
            jobDefConfig.endGroup();

            jobDefConfig.beginGroup("Resources");
            m_wallTime = jobDefConfig.value("Walltime", 120).toInt();
            m_memory = jobDefConfig.value("Memory", 2000).toInt();
            jobDefConfig.endGroup();

            jobDefConfig.beginGroup("ParameterSweep");
            m_paramSize = jobDefConfig.value("Size", 5).toInt();
            jobDefConfig.endGroup();

            jobDefConfig.beginGroup("InputFiles");
            for (int i=0; i<jobDefConfig.childKeys().count()/2; i++)
            {
                QString inputFile = jobDefConfig.value("InputFile"+QString::number(i), "").toString();
                QString inputFileSource = jobDefConfig.value("InputFileSource"+QString::number(i), "").toString();
                if (inputFile.length()!=0)
                    this->addInputFile(inputFile, inputFileSource);
            }
            jobDefConfig.endGroup();

            jobDefConfig.beginGroup("PerJobFiles");
            for (int i=0; i<jobDefConfig.childKeys().count()/2; i++)
            {
                QString inputFile = jobDefConfig.value("InputFile"+QString::number(i), "").toString();
                QString inputFileSource = jobDefConfig.value("InputFileSource"+QString::number(i), "").toString();
                if (inputFile.length()!=0)
                    this->addPerJobFile(inputFile, inputFileSource);
            }
            jobDefConfig.endGroup();

            jobDefConfig.beginGroup("OutputFiles");
            for (int i=0; i<jobDefConfig.childKeys().count()/2; i++)
            {
                QString outputFile = jobDefConfig.value("OutputFile"+QString::number(i), "").toString();
                QString outputFileSource = jobDefConfig.value("OutputFileTarget"+QString::number(i), "").toString();
                if (outputFile.length()!=0)
                    this->addOutputFile(outputFile, outputFileSource);
            }
            jobDefConfig.endGroup();

            jobDefConfig.beginGroup("RuntimeEnvironments");
            for (int i=0; i<jobDefConfig.childKeys().count(); i++)
            {
                QString runtimeEnvironment = jobDefConfig.value("RuntimeEnvironment"+QString::number(i), "").toString();
                if (runtimeEnvironment.length()!=0)
                    m_runtimeEnvironments.append(runtimeEnvironment);
            }
            jobDefConfig.endGroup();

            this->doLoadSettings(jobDefConfig);
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

    jobDefConfig.clear();

    jobDefConfig.beginGroup("Information");
    jobDefConfig.setValue("Name", m_name);
    jobDefConfig.setValue("Email", m_email);
    jobDefConfig.endGroup();

    jobDefConfig.beginGroup("Resources");
    jobDefConfig.setValue("Walltime", m_wallTime);
    jobDefConfig.setValue("Memory", m_memory);
    jobDefConfig.endGroup();

    jobDefConfig.beginGroup("ParameterSweep");
    jobDefConfig.setValue("Size", m_paramSize);
    jobDefConfig.endGroup();

    jobDefConfig.beginGroup("InputFiles");
    for (int i=0; i<m_inputFiles.count(); i++)
    {
        jobDefConfig.setValue("InputFile"+QString::number(i), m_inputFiles[i]);
        jobDefConfig.setValue("InputFileSource"+QString::number(i), m_inputFileUrls[i]);
    }
    jobDefConfig.endGroup();

    jobDefConfig.beginGroup("PerJobFiles");
    for (int i=0; i<m_perJobFiles.count(); i++)
    {
        jobDefConfig.setValue("InputFile"+QString::number(i), m_perJobFiles[i]);
        jobDefConfig.setValue("InputFileSource"+QString::number(i), m_perJobFileUrls[i]);
    }
    jobDefConfig.endGroup();

    jobDefConfig.beginGroup("OutputFiles");
    for (int i=0; i<m_outputFiles.count(); i++)
    {
        jobDefConfig.setValue("OutputFile"+QString::number(i), m_outputFiles[i]);
        jobDefConfig.setValue("OutputFileTarget"+QString::number(i), m_outputFileUrls[i]);
    }
    jobDefConfig.endGroup();

    jobDefConfig.beginGroup("RuntimeEnvironments");
    for (int i=0; i<m_runtimeEnvironments.count(); i++)
        jobDefConfig.setValue("RuntimeEnvironment"+QString::number(i), m_runtimeEnvironments[i]);
    jobDefConfig.endGroup();

    this->doSaveSettings(jobDefConfig);

    jobDefConfig.sync();

    return true;
}

void JobDefinitionBase::print()
{
    this->setupJobDescription();
    m_jobDescription.UnParse(m_xrsl, "nordugrid:xrsl");
    std::cout << m_xrsl << std::endl;
}

QString JobDefinitionBase::xrslString(QString jobName)
{
    this->setupJobDescription();

    if (jobName.length()==0)
        m_jobDescription.Identification.JobName = m_name.toStdString();
    else
        m_jobDescription.Identification.JobName = jobName.toStdString();

    m_jobDescription.UnParse(m_xrsl, "nordugrid:xrsl");
    QString returnString = m_xrsl.c_str();
    return returnString;
}

QString JobDefinitionBase::xrslStringParam(int param)
{
    this->setupJobDescription(param);
    m_jobDescription.UnParse(m_xrsl, "nordugrid:xrsl");
    QString returnString = m_xrsl.c_str();
    return returnString;
}

QString JobDefinitionBase::runScript(int param)
{
    this->setupJobDescription(param);
    QString runScript;

    // Create job script file

    QString perJobFilename;
    if (param<m_perJobFiles.count())
        perJobFilename = m_perJobFiles.at(param);
    else
        perJobFilename = "";

    this->doCreateRunScript(param, m_paramSize, m_name, perJobFilename, runScript);

    return runScript;
}



void JobDefinitionBase::doCreateRunScript(int paramNumber, int paramSize, QString jobName, QString perJobFilename, QString& script)
{
    /*
    QFile scriptFile(scriptFilename);
    scriptFile.open(QFile::WriteOnly);
    QTextStream out(&scriptFile);
    */

    script = "";

    QTextStream out(&script);
    out << "#!/bin/sh" << endl;
    out << "echo Job : " << jobName << endl;
    out << "echo I am " << paramNumber << " of " << paramSize << endl;

    /*
    scriptFile.close();
    */
}

void JobDefinitionBase::doProcessInputFile(QString& inputFilename, QString& inputSourceURL, int paramNumber, int paramSize, QString jobName)
{
    inputFilename = inputFilename.arg(QString::number(paramNumber), QString::number(paramSize), jobName);
    inputSourceURL = inputSourceURL.arg(QString::number(paramNumber), QString::number(paramSize), jobName);
}

void JobDefinitionBase::doProcessOutputFile(QString& outputFilename, QString& outputTargetURL, int paramNumber, int paramSize, QString jobName)
{
    outputFilename = outputFilename.arg(QString::number(paramNumber), QString::number(paramSize), jobName);
    outputTargetURL = outputTargetURL.arg(QString::number(paramNumber), QString::number(paramSize), jobName);
}

void JobDefinitionBase::doSaveSettings(QSettings& settings)
{

}

void JobDefinitionBase::doLoadSettings(QSettings& settings)
{

}

// ---------------------------------------------------------------------------------------------------------------


ShellScriptDefinition::ShellScriptDefinition(QObject *parent, QString name) :
    JobDefinitionBase(parent, name)
{
}

void ShellScriptDefinition::setScript(QString script)
{
    m_script = script;
}

QString ShellScriptDefinition::script()
{
    return m_script;
}

void ShellScriptDefinition::doCreateRunScript(int paramNumber, int paramSize, QString jobName, QString perJobFilename, QString& script)
{
    /*
    QFile scriptFile(scriptFilename);
    scriptFile.open(QFile::WriteOnly);
    QTextStream out(&scriptFile);
    */
    script = m_script.arg(QString::number(paramNumber), QString::number(paramSize), jobName, perJobFilename);

    /*
    scriptFile.close();
    */
}

void ShellScriptDefinition::doSaveSettings(QSettings& settings)
{
    QStringList scriptFile = m_script.split("\n");

    settings.beginGroup("ScriptTemplate");
    for (int i=0; i<scriptFile.count(); i++)
        settings.setValue("row"+QString::number(i), scriptFile[i]);
    settings.endGroup();
}

void ShellScriptDefinition::doLoadSettings(QSettings& settings)
{
    m_script = "";

    settings.beginGroup("ScriptTemplate");
    for (int i=0; i<settings.childKeys().count(); i++)
    {
        QString row = settings.value("row"+QString::number(i), "").toString();
        m_script += row + "\n";
    }
    settings.endGroup();
}


