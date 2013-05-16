#include <QString>
#include <QFtp>
#include <QUrl>
#include <QFile>
#include <iostream>
#include "ftpfileserver.h"
#include "mainwindow.h"

FTPFileServer::FTPFileServer(QObject *parent) :
    QObject(parent), FileServer()
{
    ftp = new QFtp(this);

    connect(ftp, SIGNAL(commandFinished(int,bool)),
            this, SLOT(ftpCommandFinished(int,bool)));
    connect(ftp, SIGNAL(listInfo(QUrlInfo)),
            this, SLOT(ftpCommandListFinished(QUrlInfo)));
}

FTPFileServer::~FTPFileServer()
{
    ftp->disconnect();
    ftp->close();
    delete ftp;
}


QStringList FTPFileServer::getFileInfoLabels()
{
    QStringList labels;
    labels << "File" << "Size" << "Owner" << "Group" << "Permissions" <<
            "Last read" << "Last modified";
    return labels;
}


void FTPFileServer::updateFileList(QString urlString)
{
    QUrl url(urlString);
    bool isValidUrl = url.isValid();
    if (isValidUrl == false)
    {
        mainWindow->onError("Bad url");
    }
    else
    {
        fileList.clear();

        QString serverAddress = url.host();
        QString path = url.path();

        if (serverAddress != currentServerAddress)
        {
            ftp->connectToHost(serverAddress, 21);
            ftp->login();
            currentServerAddress = serverAddress;
        }
        currentPath = path;
        currentUrlString = urlString;
        ftp->list(path);
    }
}


bool FTPFileServer::goUpOneFolder()
{
    QString url = currentUrlString.left(currentUrlString.lastIndexOf('/'));
    if (url.length() > (int)strlen("FTP://"))
    {
        updateFileList(url);
        return true;
    }
    else
    {
        return false;
    }
}


/*QString FTPFileServer::getURLOfFolderIndex(int index)
{
    QString url = "";
    bool found = false;

    int dirIndex = 0;
    foreach (ARCFileElement AFE, fileList)
    {
        if (AFE.getFileType() == ARCDir)
        {
            if (index == dirIndex)
            {
                currentUrlString = currentUrlString + "/" + AFE.getFileName();
                url = currentUrlString;
                found = true;
                break;
            }
            ++dirIndex;
        }
    }

    return url;
}*/


QString FTPFileServer::getCurrentURL()
{
    return currentUrlString;
}


QString FTPFileServer::getCurrentPath()
{
    return currentPath;
}


void FTPFileServer::ftpCommandFinished(int commandId, bool error)
{
    std::cout << "FTPFileServer::ftpCommandFinished(): Commandid " << commandId << " error " << error << std::endl;

    QString errorMsg = "";
    QFtp::Error ftpError = ftp->error();
    switch (ftpError)
    {
    case QFtp::NoError:
        errorMsg = "";
        break;
    case QFtp::HostNotFound:
        errorMsg = "The host name lookup failed: " + currentUrlString;
        break;
    case QFtp::ConnectionRefused:
        errorMsg = "The server refused the connection: " + currentUrlString;
        break;
    case QFtp::NotConnected:
        errorMsg = "There is no connection to the server: " + currentUrlString;
        break;
    case QFtp::UnknownError:
        errorMsg = "An unknown ftp error has occurred:" + currentUrlString;
        break;
    }

    if (ftp->currentCommand() == QFtp::List)
    {
        mainWindow->onFileListFinished(error, errorMsg);
    }
    else if (ftp->currentCommand() == QFtp::Get)
    {
        mainWindow->onCopyFromServerFinished(error);
    }
    else if (ftp->currentCommand() == QFtp::Remove)
    {
        mainWindow->onDeleteFinished(error);
    }
    else if (ftp->currentCommand() == QFtp::Mkdir)
    {
        mainWindow->onMakeDirFinished(error);
    }
    else if (ftp->currentCommand() == QFtp::ConnectToHost)
    {
        if (error == true)
        {
            mainWindow->onError(errorMsg);
        }
    }
    else
    {
        if (error == true)
        {
            mainWindow->onError("An unknown ftp error has ocurred");

        }
    }
}

void FTPFileServer::ftpCommandListFinished(const QUrlInfo &urlInfo)
{
    std::cout << "FTPFileServer::ftpCommandListFinished(): " << urlInfo.name().toStdString( ) << std::endl;

    // If the list is empty add ".." (up one dir) as first element
//    if (fileList.isEmpty())
//    {
//        ARCFileElement *newAFE;
//        newAFE = new ARCFileElement("..", "..", ARCDir, 0, "");
//        fileList << (ARCFileElement)(*newAFE);
//        delete newAFE;
//    }

    enum ARCFileType ft = ARCUndefined;
    if (urlInfo.isDir())
    {
        ft = ARCDir;
    }
    else if (urlInfo.isFile())
    {
        ft = ARCFile;
    }
    else
    {
        ft = ARCUndefined;
        // HANDLE THIS!!! /ALEX
    }
    ARCFileElement *newAFE;
    newAFE = new ARCFileElement(urlInfo.name(),
                                currentUrlString + "/" + urlInfo.name(),
                                ft,
                                urlInfo.group(),
                                urlInfo.isExecutable(),
                                urlInfo.isReadable(),
                                urlInfo.isWritable(),
                                urlInfo.lastModified(),
                                urlInfo.lastRead(),
                                urlInfo.owner(),
                                urlInfo.permissions(),
                                urlInfo.size());
    fileList << (ARCFileElement)(*newAFE);
    delete newAFE;
}


bool FTPFileServer::copyFromServer(QString sourcePath, QString destinationPath)
{
    bool success = false;

    if (QFile::exists(destinationPath))
    {
        success = false;
//        QMessageBox::information(this, tr("FTP"),
//                                 tr("There already exists a file called %1 in "
//                                    "the current directory.")
//                                .arg(fileName));
//        return;
    }
    else
    {
        QFile *destinationFile = new QFile(destinationPath);
        if (destinationFile->open(QIODevice::WriteOnly) == false)
        {
            success = false;
//        QMessageBox::information(this, tr("FTP"),
//                                 tr("Unable to save the file %1: %2.")
//                                 .arg(fileName).arg(file->errorString()));
//        delete file;
//        return;
        }
        else
        {
            QUrl url(sourcePath);
            bool isValidUrl = url.isValid();
            if (isValidUrl == true)
            {
                ftp->get(url.path(), destinationFile);

//              progressDialog->setLabelText(tr("Downloading %1...").arg(fileName));
//              downloadButton->setEnabled(false);
//              progressDialog->exec();

                success = true;
            }
            else
            {
                success = false;
            }
        }
    }

    return success;
}


bool FTPFileServer::copyToServer(QString sourcePath, QString destinationPath)
{
    bool success = false;

    return success;
}


bool FTPFileServer::copyToServer(QList<QUrl> &urlList, QString destinationPath)
{
    bool success = false;

    return success;
}


bool FTPFileServer::deleteItem(QString path)
{
    bool success = true;  // Always succeeds at this point, if an error occurs it will turn up later in the callback

    ftp->remove(path);

    return success;
}


bool FTPFileServer::makeDir(QString path)
{
    bool success = false;

    ftp->mkdir(path);

    return success;
}

unsigned int FTPFileServer::getFilePermissions(QString path)
{
    unsigned int permissions = 0;

    return permissions;
}

void FTPFileServer::setFilePermissions(QString path, unsigned int permissions)
{

}
