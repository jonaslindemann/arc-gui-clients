#ifndef LOCALFILESERVER_H
#define LOCALFILESERVER_H

#include "fileserver.h"
#include "arcfileelement.h"

class MainWindow;

class LocalFileServer : public FileServer
{
public:
    LocalFileServer(MainWindow *mw);

private:

public:
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

};

#endif // LOCALFILESERVER_H
