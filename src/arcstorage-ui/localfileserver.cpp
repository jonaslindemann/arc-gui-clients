#include "localfileserver.h"
#include <QDir>
#include <QVector>
#include <iostream>
#include <QUrl>
#include "arcfileelement.h"
#include "mainwindow.h"


LocalFileServer::LocalFileServer(MainWindow *mw) : FileServer(mw)
{
    currentPath = "";
}


QStringList LocalFileServer::getFileInfoLabels()
{
    QStringList labels;
    labels << "File" << "Size" << "Owner" << "Group" << "Permissions" <<
            "Last read" << "Last modified";
    return labels;
}

void LocalFileServer::updateFileList(QString URL)
{
    if (URL.left(strlen("file://")).toLower() == "file://")
    {
        URL = URL.right(URL.length() - strlen("file://"));
    }

    QDir dir(URL);

    if (dir.exists() == false)
    {
        mainWindow->onFileListFinished(true, "The path " + URL + " does not exist");
    }
    else
    {
        QFileInfoList fileInfoList = dir.entryInfoList();

        fileList.clear();

        std::cout << "LocalFileServer::updateFileList() : URL: " << URL.toStdString() << " old currentPath: " << currentPath.toStdString() << " new currentPath: " << dir.absolutePath().toStdString() << std::endl;

        currentPath = dir.absolutePath();

        for (int i = 0; i < fileInfoList.size(); ++i)
        {
            enum ARCFileType ft = ARCUndefined;
            if (fileInfoList.at(i).isFile())
            {
                ft = ARCFile;
            }
            else if (fileInfoList.at(i).isDir())
            {
                if (fileInfoList.at(i).fileName() != "." &&
                    fileInfoList.at(i).fileName() != "..")
                {
                    ft = ARCDir;
                }
            }
            else
            {
                ft = ARCUndefined;
                // HANDLE THIS!!! /ALEX
            }

             ARCFileElement *newAFE = new ARCFileElement(fileInfoList.at(i).fileName(),
                                                         fileInfoList.at(i).absoluteFilePath(),
                                                         ft,
                                                         fileInfoList.at(i).group(),
                                                         fileInfoList.at(i).isExecutable(),
                                                         fileInfoList.at(i).isReadable(),
                                                         fileInfoList.at(i).isWritable(),
                                                         fileInfoList.at(i).lastModified(),
                                                         fileInfoList.at(i).lastRead(),
                                                         fileInfoList.at(i).owner(),
                                                         fileInfoList.at(i).permissions(),
                                                         fileInfoList.at(i).size());
             fileList << (ARCFileElement)(*newAFE);
        }
    }

    mainWindow->onFileListFinished(false, "");
}


bool LocalFileServer::goUpOneFolder()
{
    QString url = currentPath.left(currentPath.lastIndexOf('/'));
    updateFileList(url);
    return true;
}


QString LocalFileServer::getCurrentURL()
{
    QString currentURL = "FILE://" + currentPath;

    return currentURL;
}


QString LocalFileServer::getCurrentPath()
{
    return currentPath;
}


bool LocalFileServer::copyFromServer(QString sourcePath, QString destinationPath)
{
    bool success = false;

    if (QFile::exists(sourcePath) == false)
    {
        success = false;
    }
    else
    {
        QFile sourceFile(sourcePath);

        success = sourceFile.copy(destinationPath);
    }

    return success;
}


bool LocalFileServer::copyToServer(QString sourcePath, QString destinationPath)
{
    bool success = false;

    return success;
}

bool LocalFileServer::copyToServer(QList<QUrl> &urlList, QString destinationFolder)
{
    bool success = false;

    QList<QString> *failedFilesList = new QList<QString>;

    success = true;

    for (int i = 0; i < urlList.size(); ++i)
    {
        QUrl url = urlList.at(i);

        QString sourcePath = url.path();

        QFile sourceFile(sourcePath);

        QString sourceFilename = sourcePath.right(sourcePath.length() - sourcePath.lastIndexOf('/') - 1);
        QString destinationPath = destinationFolder + "/" + sourceFilename;

        if (sourceFile.copy(destinationPath) == false)
        {
            success = false;
            (*failedFilesList) << sourcePath;
         }
    }

    updateFileList("FILE://" + currentPath);

    mainWindow->onCopyToServerFinished(!success, *failedFilesList);

    return success;
}

bool LocalFileServer::deleteItem(QString path)
{
    bool success = false;
    QFile::FileError error;

    QFileInfo fileInfo(path);

    if (fileInfo.exists() == false)
    {
        success = false;
    }
    else
    {
        if (fileInfo.isDir())
        {
            QDir dir(path);
            success = dir.rmdir(path);
        }
        else
        {
            QFile file(path);
            success = file.remove();
            if (success == false)
            {
                error = file.error();
            }
        }
    }

    updateFileList("FILE://" + currentPath);

    mainWindow->onDeleteFinished(!success);

    return success;
}


bool LocalFileServer::makeDir(QString path)
{
    bool success = false;

    QDir dir;

    if (dir.mkdir(currentPath + "/" + path) == true)
    {
        success = true;
    }
    else
    {
        success = false;
    }

    updateFileList("FILE://" + currentPath);

    mainWindow->onMakeDirFinished(!success);

    return success;
}


unsigned int LocalFileServer::getFilePermissions(QString path)
{
    unsigned int permissions = 0;

    QFileInfo fileInfo(path);

    if (fileInfo.exists() == false)
    {
        // HANDLE ERROR
    }
    else
    {
        permissions = fileInfo.permissions();
    }

//    mainWindow->onFilepermissions(!success);

    return permissions;
}


void LocalFileServer::setFilePermissions(QString path, unsigned int permissions)
{
    QFileInfo fileInfo(path);

    if (fileInfo.exists() == false)
    {
        // HANDLE ERROR
    }
    else
    {
        QFile::setPermissions(path, (QFile::Permissions)permissions);
    }
}
