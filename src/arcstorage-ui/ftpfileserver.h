#ifndef FTPFILESERVER_H
#define FTPFILESERVER_H

#include <QFtp>
#include <QObject>
#include "fileserver.h"

class MainWindow;

class FTPFileServer : public QObject, public FileServer
{
    Q_OBJECT
public:
    explicit FTPFileServer(MainWindow *mw, QObject *parent = 0);
    ~FTPFileServer();

private:
//    QVector<ARCFileElement> fileList;
    QFtp *ftp;
    QString currentUrlString;
    QString currentServerAddress;

private Q_SLOTS:
    void ftpCommandFinished(int commandId, bool error);
    void ftpCommandListFinished(const QUrlInfo &urlInfo);


public:
    /** Described in the abstract class FileServer */
    QStringList getFileInfoLabels();

    /** Described in the abstract class FileServer */
    void updateFileList(QString URL);

    /** Described in the abstract class FileServer */
    QVector<ARCFileElement> &getFileList() { return fileList; }

    /** Described in the abstract class FileServer */
    bool goUpOneFolder();

    /** Described in the abstract class FileServer */
    QString getCurrentURL();

    /** Described in the abstract class FileServer */
    QString getCurrentPath();

    /** Described in the abstract class FileServer */
    bool copyFromServer(QString sourcePath, QString destinationPath);

    /** Described in the abstract class FileServer */
    bool copyToServer(QString sourcePath, QString destinationPath);

    /** Described in the abstract class FileServer */
    bool copyToServer(QList<QUrl> &urlList, QString destinationPath);

    /** Described in the abstract class FileServer */
    bool deleteItem(QString path);

    /** Described in the abstract class FileServer */
    bool makeDir(QString path);

    /** Described in the abstract class FileServer */
    unsigned int getFilePermissions(QString path);

    /** Described in the abstract class FileServer */
    void setFilePermissions(QString path, unsigned int permissions);

    virtual bool deleteItems(QStringList& paths) {return false;}

};

#endif // FTPFILESERVER_H
