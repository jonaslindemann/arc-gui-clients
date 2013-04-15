#ifndef FILETRANSFERLIST_H
#define FILETRANSFERLIST_H

#include <QMutex>
#include <QList>
#include <QHash>
#include <QThread>

#include "filetransfer.h"

/// Thread class for driving the FileTransferList.
class FileTransferProcessingThread : public QThread
{
    Q_OBJECT
private:
    bool m_terminate;
    bool m_pause;
public:
    /// Create a file transfer processing thread.
    FileTransferProcessingThread();

    /// Shutdown file transfer processing
    void shutdown();

    /// Pause file transfer processing
    void pause();

    /// Resume file transfer processing processing
    void resume();

private:
    void run();
};

/// Maintains a global file transfer processing list
class FileTransferList : public QObject
{
    Q_OBJECT
private:
    QList<FileTransfer*> m_transferList;
    QList<FileTransfer*> m_activeTransferList;
    QHash<QString, FileTransfer*> m_transferDict;
    QHash<QString, FileTransfer*> m_activeTransferDict;
    FileTransferProcessingThread* m_fileProcessingThread;
    QMutex m_accessMutex;
    int m_maxTransfers;
public:
    /// Returns the global FileTransferList instance.
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

    /// Shuts down the global FileTransferList instance.
    static void drop()
    {
        static QMutex mutex;
        mutex.lock();
        delete m_instance;
        m_instance = 0;
        mutex.unlock();
    }

    /// Drives the file transfer processing.
    /**
     * This method checks for idle file transfer objects. If there are slots available, idle tasks
     * are executed. The method is called by the FileTransferProcessingThread.
     */
    void processTransfers();

    /// Sets the file processing thread used.
    void setProcessingThread(FileTransferProcessingThread* processingThread);

    /// Adds a FileTransfer object to the list.
    void addTransfer(FileTransfer* fileTransfer);

    /// Remove a FileTransfer object from the list.
    void removeTransfer(FileTransfer* fileTransfer);

    /// Return a FileTransfer object from the list
    /**
     * @param i is the position in the list.
     * @return a valid FileTransfer object. If i is outside the list a null pointer is returned.
     */
    FileTransfer* getTransfer(int i);

    /// Returns the size of the transfer list.
    int getTransferCount();

    /// Return a transfer object with a given id.
    /**
     * @param id string representing the FileTransfer object.
     * @return a valid FilaTransfer object if the id of the FileTransfer object is found, otherwise
     * return a null object.
     */
    FileTransfer* getTransfer(QString id);

    /// Cancels all idle transfers.
    /**
     * Due to limitiations in the current ARC API active transfers cannot be cancelled. Idle transfers
     * are removed and destroyed from the list.
     */
    void cancelAllTransfers();

    /// Updates the file transfer statistics of a FileTransfer object.
    /**
     * This method is called from the ARC progress callbacks for updating file transfer status. The method is
     * public to be able to be accessed from a static callback and should not be called directly.
     * @param id of FileTransfer object.
     * @param transferred currently transferred bytes.
     * @param totalSize total size of transfer.
     */
    void updateStatus(QString id, unsigned long transferred, unsigned long totalSize);

    /// Pauses all file transfer processing.
    /**
     * This method pauses transfer processing by pausing the FileTransferProcessingThread thread. Active file processing
     * tasks can't be paused.
     */
    void pauseProcessing();

    /// Resumes file transfer processing.
    /**
     * Resumes file transfer processing by resuming the FileTransferProcessingThread thread.
     */
    void resumeProcessing();

private:
    /// Create a file transfer list
    FileTransferList();

    FileTransferList(const FileTransferList &); // hide copy constructor
    FileTransferList& operator=(const FileTransferList &); // hide assign op
    // we leave just the declarations, so the compiler will warn us
    // if we try to use those two functions by accident

    static FileTransferList* m_instance;

Q_SIGNALS:
    /// This signal is sent when there is updates in file processing.
    void onUpdateStatus(QString id);

    /// This signal is sent when a file transfer is added to the list.
    void onAddTransfer(QString id);

    /// This signal is sent when a file transfer is removed from the list.
    void onRemoveTransfer(QString id);
};

#endif // FILETRANSFERLIST_H
