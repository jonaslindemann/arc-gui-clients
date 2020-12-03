#ifndef __ArcFileServer_H__
#define __ArcFileServer_H__

#include <QObject>
#include <QFutureWatcher>
#include <QMap>

#include <arc/UserConfig.h>

#include "fileserver.h"
#include "filetransfer.h"

/// ARC File server class
class ArcFileServer : public QObject, public FileServer
{
    Q_OBJECT

public:
    /// Create a ArcFileServer object.
    explicit ArcFileServer();

    QStringList getFileInfoLabels();
    void updateFileList(QString URL);
    QVector<std::shared_ptr<ARCFileElement>> &getFileList() { return fileList; }
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
    bool rename(QString fromURL, QString toURL);

    /// Starts a background file file list update.
    /**
     * This calls the updateFileList() method on a background thread. When completed
     * The onFileListFinished() method is called.
     * @param URL to pass to the updateFileList() method.
     */
    void startUpdateFileList(QString URL);

Q_SIGNALS:
    /// Called when the updateFileList() has completed on the background thread.
    void onFileListFinished(bool error, QString errorMsg);

    /// Called when an error occurs.
    void onError(QString errorStr);

    /// Called when a file transfer has completed.
    void onCopyFromServerFinished(bool error);

    /// Called when a delete operation has completed. (NOT IMPLEMENTED YET)
    void onDeleteFinished(bool error);

    /// Called when a makedir operation has completed. (NOT IMPLEMENTED YET)
    void onMakeDirFinished(bool error);

    /// Called when a copy to server operation has completed.
    void onCopyToServerFinished(bool error, QList<QString> &failedFiles);

    /// Called when a rename operation has completed.
    void onRenameFinished(bool error);

public Q_SLOTS:
    /// This slot is used by FileTransfer object to "call" home when a transfer has completed.
    void onCompleted(FileTransfer* fileTransfer, bool success, QString error);

private:
    Arc::UserConfig* m_usercfg;
    QString m_currentUrlString;
    QList<std::shared_ptr<FileTransfer>> m_transferList;
    bool m_notifyParent;
    bool initUserConfig();
    void updateFileListSilent(QString URL);
    void listFiles(QList<QUrl> &urlList, QString currentDir);
    QFutureWatcher<void> m_updateFileListWatcher;
};

#endif // ArcFileServer_H
