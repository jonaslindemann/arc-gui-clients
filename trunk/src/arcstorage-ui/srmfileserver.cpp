#include "srmfileserver.h"
#include "mainwindow.h"
#include "settings.h"
#include "filetransfer.h"
#include "arcstorage.h"
#include "filetransferlist.h"

#include <qurl.h>

#include <arc/ArcLocation.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/credential/Credential.h>
#include <arc/data/DataHandle.h>
#include <arc/data/DataPoint.h>
#include <arc/data/DataMover.h>
#include <arc/OptionParser.h>
#include <arc/FileUtils.h>

SRMFileServer::SRMFileServer(QObject *parent) :
    QObject(parent), FileServer()
{
    m_usercfg = NULL;
}

QStringList SRMFileServer::getFileInfoLabels()
{
    QStringList labels;
    labels << "File" << "Size" << "Type" << "Last modified" << "Owner" << "Group" << "Permissions" <<
            "Last read";

    //labels << "File" << "Size" << "Type" << "Last read" << "Last modified";
    return labels;
}


bool SRMFileServer::initUserConfig()
{
    bool success = false;

    if (m_usercfg == NULL)
    {
        //QString configFilename = Settings::getStringValue("srmConfigFilename");
        m_usercfg = new Arc::UserConfig("", "", Arc::initializeCredentialsType(Arc::initializeCredentialsType::TryCredentials));
        //m_usercfg = new Arc::UserConfig(configFilename.toStdString(), Arc::initializeCredentialsType::SkipCredentials);
        if (m_usercfg != NULL)
        {
            m_usercfg->UtilsDirPath(Arc::UserConfig::ARCUSERDIRECTORY);

            success = true;
        }
    }
    else
    {
        success = true;
    }

    return success;
}

void SRMFileServer::updateFileListSilent(QString URL)
{
    bool saveState = m_notifyParent;
    m_notifyParent = false;
    this->updateFileList(URL);
    m_notifyParent = saveState;
}

void SRMFileServer::updateFileList(QString URL)
{
    logger.msg(Arc::DEBUG, "Updating file list URL = %s (updateFileList)", URL.toStdString());

    if (initUserConfig() == FALSE)
    {
        logger.msg(Arc::ERROR, "Failed SRM configuration initialization.");
        if (m_notifyParent)
            Q_EMIT onFileListFinished(true, "Failed SRM configuration initialization");
        return;
    }
    else
    {
        Arc::URL arcUrl = URL.toStdString();

        bool credentialsOk = true;
        if (arcUrl.IsSecureProtocol())
        {
            m_usercfg->InitializeCredentials(Arc::initializeCredentialsType::TryCredentials);
            if (!Arc::Credential::IsCredentialsValid(*m_usercfg))
            {
                logger.msg(Arc::ERROR, "Unable to list content of %s: No valid credentials found.", arcUrl.str());
                credentialsOk = false;
                if (m_notifyParent)
                    Q_EMIT onFileListFinished(true, "SRM Credentials not valid");
                return;
            }
        }

        if (credentialsOk == true)
        {
            Arc::DataHandle dataHandle(arcUrl, *m_usercfg);
            if (!dataHandle)
            {
                logger.msg(Arc::ERROR, "Unsupported URL given");
                if (m_notifyParent)
                    Q_EMIT onFileListFinished(true, "Unsupported URL given. URL = " + arcUrl);
                return;
            }
            else
            {
                dataHandle->SetSecure(false);

                // What information to retrieve
                Arc::DataPoint::DataPointInfoType verb = (Arc::DataPoint::DataPointInfoType)
                    (Arc::DataPoint::INFO_TYPE_MINIMAL |
                     Arc::DataPoint::INFO_TYPE_NAME |
                     Arc::DataPoint::INFO_TYPE_STRUCT |
                     Arc::DataPoint::INFO_TYPE_ALL |
                     Arc::DataPoint::INFO_TYPE_TYPE |
                     Arc::DataPoint::INFO_TYPE_TIMES |
                     Arc::DataPoint::INFO_TYPE_CONTENT |
                     Arc::DataPoint::INFO_TYPE_ACCESS);

                Arc::DataStatus arcResult;
                std::list<Arc::FileInfo> arcFiles;

                // Do file listing
                arcResult = dataHandle->List(arcFiles, verb);

                // Check for errors
                if (!arcResult)
                {
                    if (m_notifyParent)
                        Q_EMIT onFileListFinished(true, "Failed to get file list from: " + URL);
                    return;
                }
                else
                {
                    m_currentUrlString = URL;

                    fileList.clear();
                    currentPath = URL;

                    for (std::list<Arc::FileInfo>::iterator arcFile = arcFiles.begin(); arcFile != arcFiles.end(); arcFile++)
                    {
// Test printing out all MetaData... not working too well right now /ALEX
//                        std::cout << arcFile->GetName() << std::endl;
//                        for (std::map<std::string, std::string>::iterator iter = arcFile->GetMetaData().begin(); iter != arcFile->GetMetaData().end(); ++iter)
//                        {
//                            std::cout << (*iter).first << ": " << (*iter).second << std::endl;
//                        }
//                        std::cout << std::endl;

                        enum ARCFileType ft = ARCUndefined;
                        if (arcFile->GetType() == Arc::FileInfo::file_type_file)
                        {
                            ft = ARCFile;
                        }
                        else if (arcFile->GetType() == Arc::FileInfo::file_type_dir)
                        {
                            ft = ARCDir;
                        }
                        else
                        {
                            ft = ARCUndefined;
                            // HANDLE THIS!!! /ALEX
                        }

                        std::string filenameStdStr = arcFile->GetName();
                        const char *filename = filenameStdStr.c_str();
                        QString fileNameQS(filename);

                        QDateTime timeCreated;
                        time_t timet = arcFile->GetCreated().GetTime();
                        timeCreated.setTime_t(timet);

                        if (!fileNameQS.indexOf(".")==0) // Don't show hidden files.
                        {
                            ARCFileElement *newAFE = new ARCFileElement(fileNameQS,
                                                                        URL + "/" + fileNameQS, //fileInfoList.at(i).absoluteFilePath(),
                                                                        ft,
                                                                        QString("???"), //fileInfoList.at(i).group(),
                                                                        false, //fileInfoList.at(i).isExecutable(),
                                                                        false, // fileInfoList.at(i).isReadable(),
                                                                        false, //fileInfoList.at(i).isWritable(),
                                                                        timeCreated, // fileInfoList.at(i).lastModified(),
                                                                        QDateTime(), // fileInfoList.at(i).lastRead(),
                                                                        QString("???"), // fileInfoList.at(i).owner(),
                                                                        0, // fileInfoList.at(i).permissions(),
                                                                        arcFile->GetSize());
                            fileList << (ARCFileElement)(*newAFE);
                        }
                    }
                }
            }
        }
    }

    if (m_notifyParent)
        Q_EMIT onFileListFinished(false, "");
}

bool SRMFileServer::goUpOneFolder()
{
    QString url = m_currentUrlString.left(m_currentUrlString.lastIndexOf('/'));
    if (url.length() > (int)strlen("SRM://"))
    {
        updateFileList(url);
        return true;
    }
    else
    {
        return false;
    }
}

QString SRMFileServer::getCurrentURL()
{
    return currentPath;
}

QString SRMFileServer::getCurrentPath()
{
    return currentPath;
}

bool SRMFileServer::copyFromServer(QString sourcePath, QString destinationPath)
{
    logger.msg(Arc::DEBUG, "SRMServer::copyFromServer()");
    bool success = false;

    if (initUserConfig() == FALSE)
    {
        logger.msg(Arc::ERROR, "Failed SRM configuration initialisation.");
        Q_EMIT onError("Failed SRM configuration initialization");
    }
    else
    {
        FileTransfer* xfr = new FileTransfer(sourcePath.toStdString(), destinationPath.toStdString(), *m_usercfg);
        FileTransferList::instance()->addTransfer(xfr);
        //m_transferList.append(xfr);
        connect(xfr, SIGNAL(onCompleted(FileTransfer*, Arc::DataStatus, QString)), this, SLOT(onCompleted(FileTransfer*, Arc::DataStatus, QString)));

        if (!xfr->execute()) // Startar filöverföringen asynkront.
        {
            logger.msg(Arc::ERROR, "SRM file transfer failed.");
            m_transferList.removeOne(xfr);
            delete xfr;
            Q_EMIT onError("SRM file transfer failed.");
        }
    }
    return success;
}

bool SRMFileServer::copyToServer(QString sourcePath, QString destinationPath)
{
    logger.msg(Arc::DEBUG, "SRMServer::copyToServer()");

    bool success = false;

    if (initUserConfig() == FALSE)
    {
        logger.msg(Arc::ERROR, "Failed SRM configuration initialisation.");
        Q_EMIT onError("Failed SRM configuration initialization");
    }
    else
    {
        FileTransfer* xfr = new FileTransfer(sourcePath.toStdString(), destinationPath.toStdString(), *m_usercfg);
        FileTransferList::instance()->addTransfer(xfr);
        //m_transferList.append(xfr);
        connect(xfr, SIGNAL(onCompleted(FileTransfer*, bool, QString)), this, SLOT(onCompleted(FileTransfer*, bool, QString)));
        if (!xfr->execute()) // Startar filöverföringen asynkront.
        {
            logger.msg(Arc::INFO, "SRM file copy failed.");
            //mainWindow->onError("SRM file copy failed");
            m_transferList.removeOne(xfr);
            delete xfr;
            Q_EMIT onError("SRM file transfer failed.");
        }
    }

    return success;
}

bool SRMFileServer::copyToServer(QList<QUrl> &urlList, QString destinationFolder)
{
    logger.msg(Arc::INFO, "Initiating multiple file copy to server (copyToServer).");

    bool success = false;

    if (initUserConfig() == FALSE)
    {
        logger.msg(Arc::ERROR, "Failed SRM configuration initialization");
        //mainWindow->onError("Failed SRM configuration initialization");
        Q_EMIT onError("Failed SRM configuration initialization");
    }
    else
    {
        QList<QString> *failedFilesList = new QList<QString>;

        success = true;

        for (int i = 0; i < urlList.size(); ++i)
        {
            QUrl url = urlList.at(i);

            QString sourcePath = url.path();

            QString sourceFilename = sourcePath.right(sourcePath.length() - sourcePath.lastIndexOf('/') - 1);
            QString destinationPath = destinationFolder + "/" + sourceFilename;

            logger.msg(Arc::INFO, "Adding filertransfer : "+sourcePath.toStdString()+" -> " + destinationPath.toStdString());
            FileTransfer* xfr = new FileTransfer(sourcePath.toStdString(), destinationPath.toStdString(), *m_usercfg);
            FileTransferList::instance()->addTransfer(xfr);

            connect(xfr, SIGNAL(onCompleted(FileTransfer*, bool, QString)), this, SLOT(onCompleted(FileTransfer*, bool, QString)));
            //xfr->execute();
        }
    }
    return success;
}

bool SRMFileServer::deleteItems(QStringList& URLs)
{
    logger.msg(Arc::INFO, "Removing multiple files (deleteItems).");
    bool success = false;

    if (initUserConfig() == FALSE)
    {
        logger.msg(Arc::INFO, "Failed configuration initialization (deleteItems).");
        Q_EMIT onError("Failed SRM configuration initialization");
    }
    else
    {
        for (int i=0; i<URLs.length(); i++)
        {
            Arc::URL arcUrl = URLs.at(i).toStdString();

            bool credentialsOk = true;
            if (arcUrl.IsSecureProtocol())
            {
                m_usercfg->InitializeCredentials(Arc::initializeCredentialsType::TryCredentials);
                if (!Arc::Credential::IsCredentialsValid(*m_usercfg))
                {
                    logger.msg(Arc::ERROR, "Unable to list content of %s: No valid credentials found", arcUrl.str());
                    credentialsOk = false;
                    Q_EMIT onError("SRM Credentials not valid");
                }
            }

            if (credentialsOk == true)
            {
                Arc::DataHandle dataHandle(arcUrl, *m_usercfg);
                if (!dataHandle)
                {
                    logger.msg(Arc::ERROR, "Unsupported URL given");
                    //mainWindow->onError("Unsupported URL given. URL = " + arcUrl);
                    Q_EMIT onError("Unsupported URL given. URL = " + arcUrl);
                    Q_EMIT onDeleteFinished(true);
                    return false;
                }
                else
                {
                    Arc::DataStatus status = dataHandle->Remove();
                    if (!status.Passed())
                    {
                        Q_EMIT onDeleteFinished(true);
                        return false;
                    }
                }
            }
        }
    }

    this->updateFileListSilent(this->getCurrentURL());
    Q_EMIT onDeleteFinished(false);
    success = true;

    return success;
}

bool SRMFileServer::deleteItem(QString URL)
{
    bool success = false;

    //std::cout << "SRMFileServer::deleteItem(): URL = " + URL.toStdString() << std::endl;

    if (initUserConfig() == FALSE)
    {
        std::cout << "SRMFileServer::SRMFileServer() Failed configuration initialization " << std::endl;
        Q_EMIT onError("Failed SRM configuration initialization");
    }
    else
    {
        Arc::URL arcUrl = URL.toStdString();

        bool credentialsOk = true;
        if (arcUrl.IsSecureProtocol())
        {
            m_usercfg->InitializeCredentials(Arc::initializeCredentialsType::TryCredentials);
            if (!Arc::Credential::IsCredentialsValid(*m_usercfg))
            {
                logger.msg(Arc::ERROR, "Unable to list content of %s: No valid credentials found", arcUrl.str());
                credentialsOk = false;
                Q_EMIT onError("SRM Credentials not valid");
            }
        }

        if (credentialsOk == true)
        {
            Arc::DataHandle dataHandle(arcUrl, *m_usercfg);
            if (!dataHandle)
            {
                logger.msg(Arc::ERROR, "Unsupported URL given");
                //mainWindow->onError("Unsupported URL given. URL = " + arcUrl);
                Q_EMIT onError("Unsupported URL given. URL = " + arcUrl);
            }
            else
            {
                Arc::DataStatus status = dataHandle->Remove();
                if (status.Passed())
                {
                    this->updateFileListSilent(this->getCurrentURL());
                    Q_EMIT onDeleteFinished(false);
                    success = true;
                }
                else
                {
                    Q_EMIT onDeleteFinished(true);
                    success = false;
                }
            }
        }
    }

    return success;
}

bool SRMFileServer::makeDir(QString path)
{
    bool success = false;
    path = currentPath + "/" + path + "/";
    std::string dirName = path.toStdString();
    logger.msg(Arc::INFO, "Creating directory: "+dirName);

    Arc::URL arcUrl = dirName;

    if (arcUrl.IsSecureProtocol()) {
      (*m_usercfg).InitializeCredentials(Arc::initializeCredentialsType::RequireCredentials);
      if (!Arc::Credential::IsCredentialsValid(*m_usercfg)) {
        logger.msg(Arc::ERROR, "Unable to create directory %s: No valid credentials found", arcUrl.str());
        Q_EMIT onMakeDirFinished(true);
        return false;
      }
    }

    Arc::DataHandle url(arcUrl, *m_usercfg);
    if (!url) {
      logger.msg(Arc::ERROR, "Unsupported URL given");
      Q_EMIT onMakeDirFinished(true);
      return false;
    }
    url->SetSecure(false);
    Arc::DataStatus res = url-> CreateDirectory(false);
    if (!res.Passed()) {
      logger.msg(Arc::ERROR, "%s%s", std::string(res), (res.GetDesc().empty() ? " " : ": "+res.GetDesc()));
      if (res.Retryable())
        logger.msg(Arc::ERROR, "This seems like a temporary error, please try again later");
      Q_EMIT onMakeDirFinished(true);
      return false;
    }

    this->updateFileList(this->getCurrentURL());
    Q_EMIT onMakeDirFinished(false);

    return true;
}

unsigned int SRMFileServer::getFilePermissions(QString path)
{
    int filePermissions = 0;

    return filePermissions;
}

void SRMFileServer::setFilePermissions(QString path, unsigned int permissions)
{

}

void SRMFileServer::onCompleted(FileTransfer* fileTransfer, bool success, QString error)
{
    logger.msg(Arc::DEBUG, "SRMFileServer::onCompleted.");

    FileTransferList::instance()->removeTransfer(fileTransfer);
    delete fileTransfer;

    if (FileTransferList::instance()->getTransferCount()==0)
    {
        this->updateFileList(currentPath);
        Q_EMIT onCopyFromServerFinished(false);
    }
}

