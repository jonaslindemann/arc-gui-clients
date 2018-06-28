#include "arcfileserver.h"
#include "arcstoragewindow.h"
#include "settings.h"
#include "filetransfer.h"
#include "arcstorage.h"
#include "filetransferlist.h"
#include "arctools.h"

#include <QUrl>
#include <QDebug>

#include "arc-gui-config.h"

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

#include <map>

void print_urls(const Arc::FileInfo& file) {
  for (std::list<Arc::URL>::const_iterator u = file.GetURLs().begin();
       u != file.GetURLs().end(); u++)
    std::cout << "\t" << *u << std::endl;
}

void print_meta(const Arc::FileInfo& file) {
  std::map<std::string, std::string> md = file.GetMetaData();
  for (std::map<std::string, std::string>::iterator mi = md.begin(); mi != md.end(); ++mi)
    std::cout<<mi->first<<":"<<mi->second<<std::endl;
}

// formatted output of details when long list is requested
void print_details(const std::list<Arc::FileInfo>& files, bool show_urls, bool show_meta) {

  if (files.empty()) return;

  unsigned int namewidth = 0;
  unsigned int sizewidth = 0;
  unsigned int csumwidth = 0;

  // find longest length of each field to align the output
  for (std::list<Arc::FileInfo>::const_iterator i = files.begin();
      i != files.end(); i++) {
    if (i->GetName().length() > namewidth) namewidth = i->GetName().length();
    if (i->CheckSize() && i->GetSize() > 0 && // log(0) not good!
        (unsigned int)(log10(i->GetSize()))+1 > sizewidth) sizewidth = (unsigned int)(log10(i->GetSize()))+1;
    if (i->CheckCheckSum() && i->GetCheckSum().length() > csumwidth) csumwidth = i->GetCheckSum().length();
  }
  std::cout << std::setw(namewidth) << std::left << "<Name> ";
  std::cout << "<Type>  ";
  std::cout << std::setw(sizewidth + 4) << std::left << "<Size>     ";
  std::cout << "<Modified>      ";
  std::cout << "<Validity> ";
  std::cout << "<CheckSum> ";
  std::cout << std::setw(csumwidth) << std::right << "<Latency>";
  std::cout << std::endl;

  // set minimum widths to accommodate headers
  if (namewidth < 7) namewidth = 7;
  if (sizewidth < 7) sizewidth = 7;
  if (csumwidth < 8) csumwidth = 8;
  for (std::list<Arc::FileInfo>::const_iterator i = files.begin();
       i != files.end(); i++) {
    std::cout << std::setw(namewidth) << std::left << i->GetName();
    switch (i->GetType()) {
      case Arc::FileInfo::file_type_file:
        std::cout << "  file";
        break;

      case Arc::FileInfo::file_type_dir:
        std::cout << "   dir";
        break;

      default:
        std::cout << " (n/a)";
        break;
    }
    if (i->CheckSize()) {
      std::cout << " " << std::setw(sizewidth) << std::right << Arc::tostring(i->GetSize());
    } else {
      std::cout << " " << std::setw(sizewidth) << std::right << "  (n/a)";
    }
#if ARC_VERSION_MAJOR >= 3
    if (i->CheckModified()) {
      std::cout << " " << i->GetModified();
    } else {
      std::cout << "       (n/a)        ";
    }
#endif
    if (i->CheckValid()) {
      std::cout << " " << i->GetValid();
    } else {
      std::cout << "   (n/a)  ";
    }
    if (i->CheckCheckSum()) {
      std::cout << " " << std::setw(csumwidth) << std::left << i->GetCheckSum();
    } else {
      std::cout << " " << std::setw(csumwidth) << std::left << "   (n/a)";
    }
    if (i->CheckLatency()) {
      std::cout << "    " << i->GetLatency();
    } else {
      std::cout << "      (n/a)";
    }
    std::cout << std::endl;
    if (show_urls) print_urls(*i);
    if (show_meta) print_meta(*i);
  }
}

ArcFileServer::ArcFileServer()
{
    m_usercfg = NULL;
    m_notifyParent = true;
}

QStringList ArcFileServer::getFileInfoLabels()
{
    QStringList labels;
    labels << "File" << "Size" << "Type" << "Last modified";
    return labels;
}


bool ArcFileServer::initUserConfig()
{
    bool success = true;
    m_usercfg = ARCTools::instance()->currentUserConfig();
    return success;
}

void ArcFileServer::updateFileListSilent(QString URL)
{
    bool saveState = m_notifyParent;
    m_notifyParent = false;
    this->updateFileList(URL);
    m_notifyParent = saveState;
}

void ArcFileServer::startUpdateFileList(QString URL)
{
    m_currentUrlString = URL;
    currentPath = URL;
    m_updateFileListWatcher.setFuture(QtConcurrent::run(this, &ArcFileServer::updateFileList, URL));
}

void ArcFileServer::updateFileList(QString URL)
{
    logger.msg(Arc::DEBUG, "Updating file list URL = %s (updateFileList)", URL.toStdString());

    Arc::URL arcUrl = URL.toStdString();

    if (!ARCTools::instance()->hasValidProxy())
    {
        if (m_notifyParent)
            Q_EMIT onFileListFinished(true, "Proxy not valid.");
        return;
    }

    m_usercfg = ARCTools::instance()->currentUserConfig();

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

            int i;

            this->clearFileList();

            currentPath = URL;

            for (std::list<Arc::FileInfo>::iterator arcFile = arcFiles.begin(); arcFile != arcFiles.end(); arcFile++)
            {
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
#if ARC_VERSION_MAJOR >= 3
                time_t timet = arcFile->GetModified().GetTime();
#else
                time_t timet = arcFile->GetCreated().GetTime();
#endif
                timeCreated.setTime_t(timet);

                if (!fileNameQS.indexOf(".")==0) // Don't show hidden files.
                {
                    if (ft == ARCDir)
                    {
                        // Check for trailing slash i directory name

                        if (fileNameQS.contains("/"))
                            fileNameQS = fileNameQS.left(fileNameQS.lastIndexOf('/'));
                    }

                    ARCFileElement* arcFileElement = new ARCFileElement(fileNameQS,
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
                    fileList.append(arcFileElement);
                }
            }
        }
    }

    if (m_notifyParent)
        Q_EMIT onFileListFinished(false, "");
}

bool ArcFileServer::goUpOneFolder()
{
    QString url = m_currentUrlString.left(m_currentUrlString.lastIndexOf('/'));
    //if (url.length() > (int)strlen("SRM://"))
    if (url.length() > 4)
    {
        updateFileList(url);
        return true;
    }
    else
    {
        return false;
    }
}

QString ArcFileServer::getCurrentURL()
{
    return currentPath;
}

QString ArcFileServer::getCurrentPath()
{
    return currentPath;
}

bool ArcFileServer::copyFromServer(QString sourcePath, QString destinationPath)
{
    logger.msg(Arc::DEBUG, "SRMServer::copyFromServer()");
    bool success = false;

    m_usercfg = ARCTools::instance()->currentUserConfig();

    FileTransfer* xfr = new FileTransfer(sourcePath.toStdString(), destinationPath.toStdString(), *m_usercfg);
    FileTransferList::instance()->addTransfer(xfr);
    connect(xfr, SIGNAL(onCompleted(FileTransfer*, Arc::DataStatus, QString)), this, SLOT(onCompleted(FileTransfer*, Arc::DataStatus, QString)));

    if (!xfr->execute()) // Startar filöverföringen asynkront.
    {
        logger.msg(Arc::ERROR, "File transfer failed.");
        m_transferList.removeOne(xfr);
        delete xfr;
        Q_EMIT onError("File transfer failed.");
    }

    return success;
}

bool ArcFileServer::copyToServer(QString sourcePath, QString destinationPath)
{
    logger.msg(Arc::DEBUG, "SRMServer::copyToServer()");

    bool success = false;

    m_usercfg = ARCTools::instance()->currentUserConfig();

    FileTransfer* xfr = new FileTransfer(sourcePath.toStdString(), destinationPath.toStdString(), *m_usercfg);
    FileTransferList::instance()->addTransfer(xfr);
    connect(xfr, SIGNAL(onCompleted(FileTransfer*, bool, QString)), this, SLOT(onCompleted(FileTransfer*, bool, QString)));

    if (!xfr->execute()) // Startar filöverföringen asynkront.
    {
        logger.msg(Arc::INFO, "File copy failed.");
        m_transferList.removeOne(xfr);
        delete xfr;
        Q_EMIT onError("File transfer failed.");
    }

    return success;
}

void ArcFileServer::listFiles(QList<QUrl> &urlList, QString currentDir)
{
    Arc::URL arcUrl = currentDir.toStdString();

    m_usercfg = ARCTools::instance()->currentUserConfig();

    Arc::DataHandle dataHandle(arcUrl, *m_usercfg);
    if (!dataHandle)
    {
        logger.msg(Arc::ERROR, "Unsupported URL given");
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
            return;
        }
        else
        {
            for (std::list<Arc::FileInfo>::iterator arcFile = arcFiles.begin(); arcFile != arcFiles.end(); arcFile++)
            {
                if (arcFile->GetType() == Arc::FileInfo::file_type_file)
                {
                    QUrl url;
                    QString filename = arcFile->GetName().c_str();
                    url.setUrl(currentDir+filename);
                    urlList.append(url);
                }
                else if (arcFile->GetType() == Arc::FileInfo::file_type_dir)
                {
                    QUrl url;
                    QString dirName = arcFile->GetName().c_str();

                    // Check for trailing slash (happens when https is used)

                    if (dirName.contains("/"))
                        dirName = dirName.left(dirName.lastIndexOf('/'));

                    QString newDir = currentDir+dirName+"/";
                    this->listFiles(urlList, newDir);
                }
                else
                {
                }
            }
        }
    }
}

bool ArcFileServer::copyToServer(QList<QUrl> &urlList, QString destinationFolder)
{
    logger.msg(Arc::INFO, "Initiating multiple file copy to server (copyToServer).");

    bool success = false;

    m_usercfg = ARCTools::instance()->currentUserConfig();

    QList<QString> *failedFilesList = new QList<QString>;

    success = true;

    for (int i = 0; i < urlList.size(); ++i)
    {
        QUrl url = urlList.at(i);
        QString sourcePath = url.path();

        // 0123456789
        // adasdasda/

        if (sourcePath.lastIndexOf("/") != sourcePath.length()-1)
        {
            QString sourceFilename = sourcePath.right(sourcePath.length() - sourcePath.lastIndexOf('/') - 1);
            QString destinationPath = destinationFolder + "/" + sourceFilename;

            logger.msg(Arc::INFO, "Adding dir filertransfer : "+sourcePath.toStdString()+" -> " + destinationPath.toStdString());
            FileTransfer* xfr = new FileTransfer(sourcePath.toStdString(), destinationPath.toStdString(), *m_usercfg);
            FileTransferList::instance()->addTransfer(xfr);
            connect(xfr, SIGNAL(onCompleted(FileTransfer*, bool, QString)), this, SLOT(onCompleted(FileTransfer*, bool, QString)));
        }
        else
        {
            //  "/home/jonas/Development/job_templates/"
            //  | sourceLocation        | sourceDirName |
            //  |              sourcePath              |
            //  |              sourceDir              |
            QString sourceDir = sourcePath.left(sourcePath.length()-1);
            QString sourceDirName = sourceDir.right(sourceDir.length() - sourceDir.lastIndexOf("/")-1);
            QString sourceLocation = sourceDir.left(sourceDir.lastIndexOf("/"));
            QList<QUrl> fileList;
            this->listFiles(fileList, sourcePath);

            for (int j=0; j<fileList.size(); ++j)
            {
                QUrl fileUrl = fileList.at(j);
                QString fileSourcePath = fileUrl.toString();
                QString locationPath = fileSourcePath;
                locationPath.remove(sourceLocation);
                QString destinationPath = destinationFolder + locationPath;

                logger.msg(Arc::INFO, "Adding file transfer : "+fileSourcePath.toStdString()+" -> " + destinationPath.toStdString());
                FileTransfer* xfr = new FileTransfer(fileSourcePath.toStdString(), destinationPath.toStdString(), *m_usercfg);
                FileTransferList::instance()->addTransfer(xfr);
                connect(xfr, SIGNAL(onCompleted(FileTransfer*, bool, QString)), this, SLOT(onCompleted(FileTransfer*, bool, QString)));
            }
        }
    }

    return success;
}

bool ArcFileServer::deleteItems(QStringList& URLs)
{
    logger.msg(Arc::INFO, "Removing multiple files (deleteItems).");
    bool success = false;

    m_usercfg = ARCTools::instance()->currentUserConfig();

    for (int i=0; i<URLs.length(); i++)
    {
        Arc::URL arcUrl = URLs.at(i).toStdString();

        Arc::DataHandle dataHandle(arcUrl, *m_usercfg);
        if (!dataHandle)
        {
            logger.msg(Arc::ERROR, "Unsupported URL given");
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

    this->updateFileListSilent(this->getCurrentURL());
    Q_EMIT onDeleteFinished(false);
    success = true;

    return success;
}

bool ArcFileServer::deleteItem(QString URL)
{
    bool success = false;

    m_usercfg = ARCTools::instance()->currentUserConfig();

    Arc::URL arcUrl = URL.toStdString();

    Arc::DataHandle dataHandle(arcUrl, *m_usercfg);
    if (!dataHandle)
    {
        logger.msg(Arc::ERROR, "Unsupported URL given");
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

    return success;
}

bool ArcFileServer::makeDir(QString path)
{
    bool success = false;
    path = currentPath + "/" + path + "/";
    std::string dirName = path.toStdString();
    logger.msg(Arc::INFO, "Creating directory: "+dirName);

    m_usercfg = ARCTools::instance()->currentUserConfig();

    Arc::URL arcUrl = dirName;

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

unsigned int ArcFileServer::getFilePermissions(QString path)
{
    int filePermissions = 0;

    return filePermissions;
}

void ArcFileServer::setFilePermissions(QString path, unsigned int permissions)
{

}

QMap<QString, QString> ArcFileServer::fileProperties(QString URL)
{
    QMap<QString, QString> propertyMap;

    Arc::URL arcUrl = URL.toStdString();

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

    Arc::DataHandle dataHandle(arcUrl, *m_usercfg);
    if (!dataHandle)
    {
        logger.msg(Arc::ERROR, "Unsupported URL given");
        Q_EMIT onError("Unsupported URL given. URL = " + arcUrl);
    }
    else
    {
        Arc::FileInfo file;
        dataHandle->Stat(file);

        std::map<std::string, std::string> md = file.GetMetaData();

        for (std::map<std::string, std::string>::iterator mi = md.begin(); mi != md.end(); ++mi)
        {
            QString property = mi->first.c_str();
            QString value = mi->second.c_str();
            propertyMap[property] = value;
        }
    }

    return propertyMap;
}

bool ArcFileServer::rename(QString fromURL, QString toURL)
{
    bool success = false;
#if ARC_VERSION_MAJOR >= 3
    m_usercfg = ARCTools::instance()->currentUserConfig();

    Arc::URL arcUrl = fromURL.toStdString();
    Arc::URL newUrl = toURL.toStdString();

    Arc::DataHandle dataHandle(arcUrl, *m_usercfg);
    if (!dataHandle)
    {
        logger.msg(Arc::ERROR, "Unsupported URL given");
        Q_EMIT onError("Unsupported URL given. URL = " + arcUrl);
    }
    else
    {
        Arc::DataStatus status = dataHandle->Rename(newUrl);
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

    return success;
#else
    return false;
#endif
}


void ArcFileServer::onCompleted(FileTransfer* fileTransfer, bool success, QString error)
{
    logger.msg(Arc::DEBUG, "ArcFileServer::onCompleted.");

    FileTransferList::instance()->removeTransfer(fileTransfer);
    //delete fileTransfer;

    if (FileTransferList::instance()->getTransferCount()==0)
    {
        this->updateFileList(currentPath);
        Q_EMIT onCopyFromServerFinished(false);
    }
}

