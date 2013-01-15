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

enum TTransferState { TS_IDLE,
                       TS_EXECUTED,
                       TS_FAILED,
                       TS_COMPLETED
                         };

/** Jonas Lindemann should document this class :)
  *
  */
class FileTransfer : public QObject
{
    Q_OBJECT
private:
    Arc::URL m_sourceUrl;
    Arc::URL m_destUrl;
    Arc::DataHandle m_sourceHandle;
    Arc::DataHandle m_destHandle;
    Arc::DataMover* m_mover;
    Arc::FileCache* m_cache;
    Arc::URLMap* m_urlMap;
    Arc::UserConfig* m_config;
    int m_timeout;
    int m_retries;
    bool m_secure;
    bool m_passive;
    bool m_notpassive;
    bool m_force;
    bool m_verbose;
    bool m_nocopy;
    int m_recursion;
    Arc::DataStatus m_status;
    Arc::SimpleCondition m_cond;
    bool m_completed;
    std::string m_id;
    unsigned long m_transferred;
    unsigned long m_totalSize;
    TTransferState m_transferState;
public:
    FileTransfer(const std::string& source_str, const std::string& destination_str, Arc::UserConfig& usercfg);
    virtual ~FileTransfer();

    TTransferState transferState();

    bool execute();
    void wait();
    Arc::DataStatus status();
    void completed(Arc::DataStatus res, std::string error);

    void cancel();

    bool isCompleted();

    QString id();
    QString sourceUrl();
    QString destUrl();

    void updateTransferStatus(unsigned long transferred, unsigned long totalSize);
    void getTransferStatus(unsigned long& transferred, unsigned long& totalSize);

Q_SIGNALS:
    void onProgress(FileTransfer* fileTransfer, unsigned long long bytesTransferred, unsigned long long bytesTotal);
    void onCompleted(FileTransfer* fileTransfer, bool success, QString error);
};

#endif // FILETRANSFER_H
