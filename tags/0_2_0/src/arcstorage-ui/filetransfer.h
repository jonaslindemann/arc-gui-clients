#ifndef FILETRANSFER_H
#define FILETRANSFER_H

#include <iostream>
#include <string>

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

/** Jonas Lindemann should document this class :)
  *
  */
class FileTransfer
{
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
public:
    FileTransfer(const std::string& source_str, const std::string& destination_str, Arc::UserConfig& usercfg);
    virtual ~FileTransfer();

    bool execute();
    void wait();
    Arc::DataStatus status();
    void completed();

    bool isCompleted();
};

#endif // FILETRANSFER_H
