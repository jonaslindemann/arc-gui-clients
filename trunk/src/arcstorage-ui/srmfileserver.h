#ifndef SRMFILESERVER_H
#define SRMFILESERVER_H

#include <QObject>
#include <arc/UserConfig.h>
#include "fileserver.h"
#include "filetransfer.h"

class MainWindow;

class SRMFileServer : public QObject, public FileServer
{
    Q_OBJECT

private:
    Arc::UserConfig* m_usercfg;
    QString m_currentUrlString;

    QList<FileTransfer*> m_transferList;

    bool initUserConfig();

public:
    explicit SRMFileServer(MainWindow *mw, QObject *parent = 0);

    QStringList getFileInfoLabels();
    void updateFileList(QString URL);
    QVector<ARCFileElement> &getFileList() { return fileList; }
    bool goUpOneFolder();
    QString getCurrentURL();
    QString getCurrentPath();
    bool copyFromServer(QString sourcePath, QString destinationPath);
    bool copyToServer(QString sourcePath, QString destinationPath);
    bool copyToServer(QList<QUrl> &urlList, QString destinationPath);
    bool deleteItem(QString path);
    bool makeDir(QString path);
    unsigned int getFilePermissions(QString path);
    void setFilePermissions(QString path, unsigned int permissions);
public Q_SLOTS:
    void onCompleted(FileTransfer* fileTransfer, bool success, QString error);

};

#endif // SRMFILESERVER_H
