#include "fileserverfactory.h"
#include "fileserver.h"
#include "srmfileserver.h"

/*
FTPFileServer*   FileServerFactory::ftpFileServer   = NULL;
LocalFileServer* FileServerFactory::localFileServer = NULL;
SRMFileServer*   FileServerFactory::srmFileServer   = NULL;
*/

const QString FileServerFactory::LOCALFILE_PREFIX = QString("file://");
const QString FileServerFactory::FTP_PREFIX       = QString("ftp://");
const QString FileServerFactory::SRM_PREFIX       = QString("srm://");
const QString FileServerFactory::GSIFTP_PREFIX       = QString("gsiftp://");
const QString FileServerFactory::HTTP_PREFIX       = QString("http://");

FileServerFactory::FileServerFactory()
{
}

FileServer* FileServerFactory::createFileServer(QString type)
{
    FileServer *fileServer = NULL;

    type = type.toLower();

    if (type == FTP_PREFIX || type.left(FTP_PREFIX.length()) == FTP_PREFIX)
    {
        fileServer = new SRMFileServer();
    }
    else if (type == LOCALFILE_PREFIX || type.left(LOCALFILE_PREFIX.length()) == LOCALFILE_PREFIX)
    {
        fileServer = new SRMFileServer();
    }
    else if (type == SRM_PREFIX || type.left(SRM_PREFIX.length()) == SRM_PREFIX)
    {
        fileServer = new SRMFileServer();
    }
    else if (type == GSIFTP_PREFIX || type.left(GSIFTP_PREFIX.length()) == GSIFTP_PREFIX)
    {
        fileServer = new SRMFileServer();
    }
    else if (type == HTTP_PREFIX || type.left(HTTP_PREFIX.length()) == HTTP_PREFIX)
    {
        fileServer =  new SRMFileServer();
    }
    else
    {
        fileServer = new SRMFileServer();
    }

    return fileServer;
}

