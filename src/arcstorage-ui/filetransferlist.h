#ifndef FILETRANSFERLIST_H
#define FILETRANSFERLIST_H

#include <QMutex>
#include <QList>
#include <QHash>
#include <QThread>

#include "filetransfer.h"

class FileTransferProcessingThread : public QThread
{
    Q_OBJECT
private:
    bool m_terminate;
public:
    FileTransferProcessingThread();

    void shutdown();

private:
    void run();
};

class FileTransferList : public QObject
{
    Q_OBJECT
private:
    QList<FileTransfer*> m_transferList;
    QList<FileTransfer*> m_activeTransferList;
    QHash<QString, FileTransfer*> m_transferDict;
    QHash<QString, FileTransfer*> m_activeTransferDict;
    QMutex m_accessMutex;
    int m_maxTransfers;
public:
    static FileTransferList* instance()
    {
        static QMutex mutex;
        if (!m_instance)
        {
            mutex.lock();

            if (!m_instance)
                m_instance = new FileTransferList;

            mutex.unlock();
        }

        return m_instance;
    }

    static void drop()
    {
        static QMutex mutex;
        mutex.lock();
        delete m_instance;
        m_instance = 0;
        mutex.unlock();
    }

    void processTransfers();

    void addTransfer(FileTransfer* fileTransfer);
    void removeTransfer(FileTransfer* fileTransfer);
    FileTransfer* getTransfer(int i);
    int getTransferCount();
    FileTransfer* getTransfer(QString id);

    void cancelAllTransfers();

    void updateStatus(QString id, unsigned long transferred, unsigned long totalSize);

private:
    FileTransferList();

    FileTransferList(const FileTransferList &); // hide copy constructor
    FileTransferList& operator=(const FileTransferList &); // hide assign op
    // we leave just the declarations, so the compiler will warn us
    // if we try to use those two functions by accident

    static FileTransferList* m_instance;

Q_SIGNALS:
    void onUpdateStatus(QString id);
    void onAddTransfer(QString id);
    void onRemoveTransfer(QString id);
};

#endif // FILETRANSFERLIST_H
