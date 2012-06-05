#include "srmfileserver.h"
#include "mainwindow.h"
#include "settings.h"
#include "filetransfer.h"
#include "arcstorage.h"

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

SRMFileServer::SRMFileServer(MainWindow *mw, QObject *parent) :
    QObject(parent), FileServer(mw)
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
        QString configFilename = Settings::getStringValue("srmConfigFilename");
        m_usercfg = new Arc::UserConfig(configFilename.toStdString(), Arc::initializeCredentialsType::SkipCredentials);
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

void SRMFileServer::updateFileList(QString URL)
{
    logger.msg(Arc::DEBUG, "SRMFileServer::updateFileList(): URL = %s", URL.toStdString());

    if (initUserConfig() == FALSE)
    {
        logger.msg(Arc::ERROR, "Failed SRM configuration initialization.");
        mainWindow->onFileListFinished(true, "Failed SRM configuration initialization");
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
                mainWindow->onFileListFinished(true, "SRM Credentials not valid");

            }
        }

        if (credentialsOk == true)
        {
            Arc::DataHandle dataHandle(arcUrl, *m_usercfg);
            if (!dataHandle)
            {
                logger.msg(Arc::ERROR, "Unsupported URL given");
                mainWindow->onFileListFinished(true, "Unsupported URL given. URL = " + arcUrl);
            }
            else
            {
                dataHandle->SetSecure(false);

                // Check access
//                if(dataHandle->Check()) { std::cout << "passed" << std::endl; }
//                else { std::cout << "failed" << std::endl; }

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
//                  Arc::FileInfo arcFile;
                std::list<Arc::FileInfo> arcFiles;

                // Do file listing
                arcResult = dataHandle->List(arcFiles, verb);

                // Check for errors
                if (!arcResult)
                {
                    mainWindow->onFileListFinished(true, "Failed to get file list from: " + URL);
//                    if (arcFiles.size() == 0)
//                    {
//                        logger.msg(Arc::ERROR, "Failed listing files");
//                        if (arcResult.Retryable())
//                        {
//                          logger.msg(Arc::ERROR, "This seems like a temporary error, please try again later");
//                            return;
//                        }
//                        logger.msg(Arc::INFO, "Warning: Failed listing files but some information is obtained");
//                    }
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
        mainWindow->onFileListFinished(false, "");
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
        mainWindow->onError("Failed SRM configuration initialization");
    }
    else
    {
        FileTransfer* xfr = new FileTransfer(sourcePath.toStdString(), destinationPath.toStdString(), *m_usercfg);
        m_transferList.append(xfr);
        connect(xfr, SIGNAL(onCompleted(FileTransfer*, Arc::DataStatus, QString)), this, SLOT(onCompleted(FileTransfer*, Arc::DataStatus, QString)));

        if (xfr->execute() == TRUE) // Startar filöverföringen asynkront.
        {
            /*
            while (!xfr->isCompleted()) {
                QApplication::processEvents();
            }
            //xfr->wait(); // Blockerande vänterutin. Behövs inte i GUI kod.
            delete xfr;
            mainWindow->onCopyFromServerFinished(false);
            */
        }
        else
        {
            logger.msg(Arc::ERROR, "SRM file transfer failed.");
            mainWindow->onError("SRM file transfer failed.");
            delete xfr;
        }

        mainWindow->onCopyFromServerFinished(false);

//    Det går också att kontrollera status med:
//    xfr->isCompleted()
    }

    return success;
}

bool SRMFileServer::copyToServer(QString sourcePath, QString destinationPath)
{
    logger.msg(Arc::INFO, "SRMServer::copyToServer()");

    bool success = false;

    if (initUserConfig() == FALSE)
    {
        logger.msg(Arc::ERROR, "Failed SRM configuration initialisation.");
        mainWindow->onError("Failed SRM configuration initialization");
    }
    else
    {
        FileTransfer* xfr = new FileTransfer(sourcePath.toStdString(), destinationPath.toStdString(), *m_usercfg);
        m_transferList.append(xfr);
        connect(xfr, SIGNAL(onCompleted(FileTransfer*, bool, QString)), this, SLOT(onCompleted(FileTransfer*, bool, QString)));
        if (xfr->execute() == TRUE) // Startar filöverföringen asynkront.
        {
            /*
            logger.msg(Arc::INFO, "Waiting for transfer to complete.");
            while (!xfr->isCompleted()) {
                QApplication::processEvents();
            }
            logger.msg(Arc::INFO, "Transfer complete.");
            //xfr->wait(); // Blockerande vänterutin. Behövs inte i GUI kod.
            delete xfr;
            mainWindow->onCopyFromServerFinished(false);
            */
        }
        else
        {
            logger.msg(Arc::INFO, "SRM file copy failed.");
            mainWindow->onError("SRM file copy failed");
            delete xfr;
        }

//    Det går också att kontrollera status med:
//    xfr->isCompleted()
    }

    return success;
}

bool SRMFileServer::copyToServer(QList<QUrl> &urlList, QString destinationFolder)
{
    logger.msg(Arc::INFO, "SRMServer::copyToServer(urlList)");

    bool success = false;

    if (initUserConfig() == FALSE)
    {
        logger.msg(Arc::ERROR, "Failed SRM configuration initialization");
        mainWindow->onError("Failed SRM configuration initialization");
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

            FileTransfer* xfr = new FileTransfer(sourcePath.toStdString(), destinationPath.toStdString(), *m_usercfg);
            m_transferList.append(xfr);
            connect(xfr, SIGNAL(onCompleted(FileTransfer*, bool, QString)), this, SLOT(onCompleted(FileTransfer*, bool, QString)));
            xfr->execute(); // Startar filöverföringen asynkront.

            /*
            // Wait while still processing events.

            while (!xfr->isCompleted()) {
                QApplication::processEvents();
            }

            //xfr->wait(); // Blockerande vänterutin. Behövs inte i GUI kod.
            delete xfr;
            */
        }

        /*
        updateFileList(currentPath);

        mainWindow->onCopyToServerFinished(!success, *failedFilesList);
        */
    }

    return success;
}

bool SRMFileServer::deleteItem(QString URL)
{
    bool success = false;

    std::cout << "SRMFileServer::deleteItem(): URL = " + URL.toStdString() << std::endl;

    if (initUserConfig() == FALSE)
    {
        std::cout << "SRMFileServer::SRMFileServer() Failed configuration initialization " << std::endl;
//        logger.msg(Arc::ERROR, "Failed configuration initialization");
        mainWindow->onError("Failed SRM configuration initialization");
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
                mainWindow->onError("SRM Credentials not valid");

            }
        }

        if (credentialsOk == true)
        {
            Arc::DataHandle dataHandle(arcUrl, *m_usercfg);
            if (!dataHandle)
            {
                logger.msg(Arc::ERROR, "Unsupported URL given");
                mainWindow->onError("Unsupported URL given. URL = " + arcUrl);
            }
            else
            {
                Arc::DataStatus status = dataHandle->Remove();
                if (status.Passed())
                {
                    updateFileList(m_currentUrlString);
                    mainWindow->onDeleteFinished(false);
                }
                else
                {
                    mainWindow->onDeleteFinished(true);
                }
            }
        }
    }

    return success;
}

bool SRMFileServer::makeDir(QString path)
{
    qDebug() << "SRMFileServer::makeDir()";
    bool success = false;
    path = currentPath + "/" + path + "/";
    std::string dirName = path.toStdString();
    qDebug() << path;

    Arc::URL arcUrl = dirName;

    if (arcUrl.IsSecureProtocol()) {
      (*m_usercfg).InitializeCredentials(Arc::initializeCredentialsType::RequireCredentials);
      if (!Arc::Credential::IsCredentialsValid(*m_usercfg)) {
        logger.msg(Arc::ERROR, "Unable to create directory %s: No valid credentials found", arcUrl.str());
        mainWindow->onMakeDirFinished(true);
        return false;
      }
    }

    Arc::DataHandle url(arcUrl, *m_usercfg);
    if (!url) {
      logger.msg(Arc::ERROR, "Unsupported URL given");
      mainWindow->onMakeDirFinished(true);
      return false;
    }
    url->SetSecure(false);
    Arc::DataStatus res = url-> CreateDirectory(false);
    if (!res.Passed()) {
      logger.msg(Arc::ERROR, "%s%s", std::string(res), (res.GetDesc().empty() ? " " : ": "+res.GetDesc()));
      if (res.Retryable())
        logger.msg(Arc::ERROR, "This seems like a temporary error, please try again later");
      mainWindow->onMakeDirFinished(true);
      return false;
    }

    this->updateFileList(this->getCurrentURL());
    mainWindow->onMakeDirFinished(false);

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
    logger.msg(Arc::INFO, "SRMFileServer::onCompleted.");
    m_transferList.removeOne(fileTransfer);
    delete fileTransfer;

    if (m_transferList.length()==0)
    {
        this->updateFileList(currentPath);
        mainWindow->onCopyFromServerFinished(false);
    }
}

