#include "fileserverfactory.h"
#include "ftpfileserver.h"
#include "localfileserver.h"
#include "mainwindow.h"
#include "fileserver.h"
#include "srmfileserver.h"

FTPFileServer*   FileServerFactory::ftpFileServer   = NULL;
LocalFileServer* FileServerFactory::localFileServer = NULL;
SRMFileServer*   FileServerFactory::srmFileServer   = NULL;

const QString FileServerFactory::LOCALFILE_PREFIX = QString("file://");
const QString FileServerFactory::FTP_PREFIX       = QString("ftp://");
const QString FileServerFactory::SRM_PREFIX       = QString("srm://");
const QString FileServerFactory::GSIFTP_PREFIX       = QString("gsiftp://");
const QString FileServerFactory::HTTP_PREFIX       = QString("http://");

FileServerFactory::FileServerFactory()
{
    delete FileServerFactory::ftpFileServer;
    delete FileServerFactory::localFileServer;
    delete FileServerFactory::srmFileServer;
}

FileServer* FileServerFactory::getFileServer(QString type, MainWindow *mw)
{
    FileServer *fileServer = NULL;

    type = type.toLower();

    if (type == FTP_PREFIX || type.left(FTP_PREFIX.length()) == FTP_PREFIX)
    {
        if (FileServerFactory::ftpFileServer == NULL)
        {
            FileServerFactory::ftpFileServer = new FTPFileServer(mw);
        }
        fileServer = FileServerFactory::ftpFileServer;
    }
    else if (type == LOCALFILE_PREFIX || type.left(LOCALFILE_PREFIX.length()) == LOCALFILE_PREFIX)
    {
        if (FileServerFactory::localFileServer == NULL)
        {
            FileServerFactory::localFileServer = new LocalFileServer(mw);
        }
        fileServer = FileServerFactory::localFileServer;
    }
    else if (type == SRM_PREFIX || type.left(SRM_PREFIX.length()) == SRM_PREFIX)
    {
        if (FileServerFactory::srmFileServer == NULL)
        {
            FileServerFactory::srmFileServer = new SRMFileServer(mw);
        }
        fileServer = FileServerFactory::srmFileServer;  // DEFAULT
    }
    else if (type == GSIFTP_PREFIX || type.left(GSIFTP_PREFIX.length()) == GSIFTP_PREFIX)
    {
        if (FileServerFactory::srmFileServer == NULL)
        {
            FileServerFactory::srmFileServer = new SRMFileServer(mw);
        }
        fileServer = FileServerFactory::srmFileServer;  // DEFAULT
    }
    else if (type == HTTP_PREFIX || type.left(HTTP_PREFIX.length()) == HTTP_PREFIX)
    {
        if (FileServerFactory::srmFileServer == NULL)
        {
            FileServerFactory::srmFileServer = new SRMFileServer(mw);
        }
        fileServer = FileServerFactory::srmFileServer;  // DEFAULT
    }
    else
    {
        if (FileServerFactory::localFileServer == NULL)
        {
            FileServerFactory::localFileServer = new LocalFileServer(mw);
        }
        fileServer = FileServerFactory::localFileServer;  // DEFAULT
    }

    return fileServer;
}

FileServer* FileServerFactory::getNewFileServer(QString type, MainWindow *mw)
{
    FileServer *fileServer = NULL;

    type = type.toLower();

    if (type == FTP_PREFIX || type.left(FTP_PREFIX.length()) == FTP_PREFIX)
    {
        fileServer = new FTPFileServer(mw);
    }
    else if (type == LOCALFILE_PREFIX || type.left(LOCALFILE_PREFIX.length()) == LOCALFILE_PREFIX)
    {
        fileServer = new SRMFileServer(mw);
    }
    else if (type == SRM_PREFIX || type.left(SRM_PREFIX.length()) == SRM_PREFIX)
    {
        fileServer = new SRMFileServer(mw);
    }
    else if (type == GSIFTP_PREFIX || type.left(GSIFTP_PREFIX.length()) == GSIFTP_PREFIX)
    {
        fileServer = new SRMFileServer(mw);
    }
    else if (type == HTTP_PREFIX || type.left(HTTP_PREFIX.length()) == HTTP_PREFIX)
    {
        fileServer =  new SRMFileServer(mw);
    }
    else
    {
        fileServer = new SRMFileServer(mw);
    }

    return fileServer;
}

