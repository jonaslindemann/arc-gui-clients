#ifndef FILESERVERFACTORY_H
#define FILESERVERFACTORY_H
#include "ftpfileserver.h"
#include "localfileserver.h"
#include "srmfileserver.h"

class MainWindow;
class FileServer;
class SRMFileServer;

/** This class describes a File Server Factory. You ask for you server by
  * protocol name and recieve a File Server object in return.
  */
class FileServerFactory
{
private:
    static FTPFileServer   *ftpFileServer;
    static LocalFileServer *localFileServer;
    static SRMFileServer   *srmFileServer;

public:
    static const QString LOCALFILE_PREFIX;
    static const QString FTP_PREFIX;
    static const QString SRM_PREFIX;
    static const QString GSIFTP_PREFIX;
    static const QString HTTP_PREFIX;

    FileServerFactory();

    /** This method takes the name of a protocol and returns a File Server object */
    static FileServer* getFileServer(QString type, MainWindow *mw);
};

#endif // FILESERVERFACTORY_H
