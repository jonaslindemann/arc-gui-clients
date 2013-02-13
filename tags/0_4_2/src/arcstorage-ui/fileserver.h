#ifndef FILESERVER_H
#define FILESERVER_H

#include <QVector>
#include <QString>
#include <QVariant>

#include "arcfileelement.h"

class MainWindow;

/**
  * The FileServer class described an abstract File Server. It contains
  * abstract methods that describe operations that a file server would
  * typically need to perform. It doesn't act as a file server, it
  * talks to a file server.
  */


class FileServer
{
private:



protected:
    /** Pointer to main window needed for callbacks (get files done, delete file done, copy file done... */
    //MainWindow *mainWindow;

    /** A list of the files that were last browsed (and are currently displayed in the gui) */
    QVector<ARCFileElement*> fileList;

    /** The current path (that is displayed in the gui) */
    QString currentPath;

    bool m_notifyParent;

    void clearFileList();

public:
    FileServer();

    void setNotifyParent(bool flag);
    bool getNotifyParent();

    /** Returns the column names in the file list (name, size, last change date...). Allows different protocols to display their own info */
    virtual QStringList getFileInfoLabels() = 0;

    /** Reads a new list of files from the server to the filelist */
    virtual void updateFileList(QString URL) = 0;

    /** Return a reference to the filelist */
    virtual QVector<ARCFileElement*> &getFileList() = 0;

    /** Go up one folder in the folder structure (cd ..) */
    virtual bool goUpOneFolder() = 0;

    /** Get current URL (== file protocol + path) */
    virtual QString getCurrentURL() = 0;

    /** Get current path */
    virtual QString getCurrentPath() = 0;

    /** Get file permissions */
    virtual unsigned int getFilePermissions(QString path) = 0;

    /** Set file permissions */
    virtual void setFilePermissions(QString path, unsigned int permissions) = 0;

    /** Copy a file from the server to a local disk */
    virtual bool copyFromServer(QString sourcePath, QString destinationPah) = 0;

    /** Copy a file from local disk to server */
    virtual bool copyToServer(QString sourcePath, QString destinationPah) = 0;

    /** Copy a list of files from local disk to server */
    virtual bool copyToServer(QList<QUrl> &urlList, QString destinationPath) = 0;

    /** Delete a file from the server */
    virtual bool deleteItem(QString path) = 0;

    /** Delete several files from the server */
    virtual bool deleteItems(QStringList& paths) = 0;

    /** Create a folder on the server */
    virtual bool makeDir(QString path) = 0;
};

#endif // FILESERVER_H
