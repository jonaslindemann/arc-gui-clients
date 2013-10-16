#ifndef FILETRANSFER_H
#define FILETRANSFER_H

#include <iostream>
#include <string>

#include "arc-gui-config.h"
#include <arc/Thread.h>

#include <arc/ArcLocation.h>
#include <arc/GUID.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/User.h>
#include <arc/UserConfig.h>
#include <arc/credential/Credential.h>
#include <arc/data/FileCache.h>
#include <arc/data/DataHandle.h>
#include <arc/data/DataMover.h>
#include <arc/data/URLMap.h>
#include <arc/OptionParser.h>

#include <QObject>
#include <QString>

#if QT_VERSION < QT_VERSION_CHECK(4, 8, 0)
#include <QTime>
typedef QTime QElapsedTimer;
#else
#include <QTime>
#include <QElapsedTimer>
#endif

/// File transfer states
enum TTransferState { TS_IDLE,     ///<Waiting to be transferred.
                      TS_EXECUTED, ///<Transfer has been initiated.
                      TS_FAILED,   ///<Transfer failed.
                      TS_COMPLETED ///<Transfer completed succedfully.
                    };

/// Represents a single data transfer from one url to another
class FileTransfer : public QObject
{
    Q_OBJECT
private:
    std::string m_id;
    TTransferState m_transferState;
    int m_timeout;
    int m_retries;
    bool m_secure;
    bool m_passive;
    bool m_notpassive;
    bool m_force;
    bool m_verbose;
    bool m_nocopy;
    int m_recursion;
    bool m_completed;
    unsigned long m_transferred;
    unsigned long m_totalSize;

    Arc::URL m_sourceUrl;
    Arc::URL m_destUrl;
    Arc::DataHandle m_sourceHandle;
    Arc::DataHandle m_destHandle;
    Arc::DataMover* m_mover;
    Arc::FileCache* m_cache;
    Arc::URLMap* m_urlMap;
    Arc::UserConfig* m_config;
    Arc::DataStatus m_status;
    Arc::SimpleCondition m_cond;

    QTime m_transferTimer;
    qint64 m_transferTime;
public:
    /// Create a file transfer object.
    /**
     * Creates a file transfer object used for handling a file transfer using the ARC API:s. The
     * transfer object is by default idle and will not start any transfers until the execute method
     * is called.
     * @param source_str is the source URL of a file to be transferred.
     * @param destination_str is the destination URL of the file to transferred.
     * @param usercfg is the current ARC configuration object.
     */
    FileTransfer(const std::string& source_str, const std::string& destination_str, Arc::UserConfig& usercfg);
    virtual ~FileTransfer();

    /// Return the current transfer state.
    TTransferState transferState();

    /// Start the actual file transfer.
    bool execute();

    /// Wait for the file transfer to complete.
    void wait();

    /// Return the ARC transfer status.
    Arc::DataStatus status();

    /// Routine called by the ARC callbacks to indicate a failed/completed file transfer.
    /**
     * This routine should not be called directly. It is public so that the static callback can
     * access the routine.
     */
    void completed(Arc::DataStatus res, std::string error);

    /// Cancel the transfer.
    /**
     * Currently only destroys the ARC data mover object. Graceful cancellation is currently not supported
     * due to restrictions in the ARC API.
     */
    void cancel();

    /// Returns true if transfer is completed.
    bool isCompleted();

    /// Return current transfer id.
    QString id();

    /// Return source URL.
    QString sourceUrl();

    /// Return destination URL.
    QString destUrl();

    /// Updated the current transfer statistics.
    /**
     * This routine should not be called directly. It is public so that the static callback can accces
     * the routine.
     */
    void updateTransferStatus(unsigned long transferred, unsigned long totalSize);

    /// Return current transfer statistics.
    void getTransferStatus(unsigned long& transferred, unsigned long& totalSize);

    double transferTime();
    unsigned long totalTransferred();
    unsigned long totalSize();

Q_SIGNALS:

    /// Signal for updating transfer status
    /**
     * This signal is called whenever the ARC callback is called to convey the current transfer statistics on the
     * ongoing file transfer.
     * @param fileTransfer file transfer object that initiated the signal.
     * @param bytesTransferred currently transfers bytes
     * @param bytesTotal total of bytes to be transferred.
     */
    void onProgress(FileTransfer* fileTransfer, unsigned long long bytesTransferred, unsigned long long bytesTotal);
    void onCompleted(FileTransfer* fileTransfer, bool success, QString error);
};

#endif // FILETRANSFER_H
