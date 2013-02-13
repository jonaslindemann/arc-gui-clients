#ifndef SRMFILESERVER_H
#define SRMFILESERVER_H

#include <QObject>
#include <QFutureWatcher>
#include <QMap>
#include <arc/UserConfig.h>
#include "fileserver.h"
#include "filetransfer.h"

class SRMFileServer : public QObject, public FileServer
{
    Q_OBJECT

private:
    Arc::UserConfig* m_usercfg;
    QString m_currentUrlString;
    QList<FileTransfer*> m_transferList;
    bool m_notifyParent;
    bool initUserConfig();
    void updateFileListSilent(QString URL);
    void listFiles(QList<QUrl> &urlList, QString currentDir);

    QFutureWatcher<void> m_updateFileListWatcher;

public:
    explicit SRMFileServer();

    QStringList getFileInfoLabels();

    void startUpdateFileList(QString URL);
    void updateFileList(QString URL);


    QVector<ARCFileElement*> &getFileList() { return fileList; }
    bool goUpOneFolder();
    QString getCurrentURL();
    QString getCurrentPath();
    bool copyFromServer(QString sourcePath, QString destinationPath);
    bool copyToServer(QString sourcePath, QString destinationPath);
    bool copyToServer(QList<QUrl> &urlList, QString destinationPath);
    bool deleteItem(QString URL);
    bool deleteItems(QStringList& URLs);
    bool makeDir(QString path);
    unsigned int getFilePermissions(QString path);
    void setFilePermissions(QString path, unsigned int permissions);

    QMap<QString, QString> fileProperties(QString URL);

Q_SIGNALS:
    void onFileListFinished(bool error, QString errorMsg);
    void onError(QString errorStr);
    void onCopyFromServerFinished(bool error);
    void onDeleteFinished(bool error);
    void onMakeDirFinished(bool error);
    void onCopyToServerFinished(bool error, QList<QString> &failedFiles);

    void onUpdateFileListFinished();

public Q_SLOTS:
    void onCompleted(FileTransfer* fileTransfer, bool success, QString error);

};

#endif // SRMFILESERVER_H
