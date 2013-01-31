#include "filetransfer.h"
#include "arcstorage.h"
#include "filetransferlist.h"
#include "globalstateinfo.h"

#include <iostream>
#include <string>
#include <sstream>

#include <QDebug>


template <typename T>
std::string convertPointerToStringAddress(const T* obj)
{
  int address(reinterpret_cast<size_t>(obj));
  std::stringstream ss;
  ss << address;
  return ss.str();
}

static void _onProgress(FILE *o, const char* prefix, unsigned int,
                     unsigned long long int all, unsigned long long int max,
                     double, double)
{
    //logger.msg(Arc::DEBUG, "ID%s: Transferred: %llu kB of %llu", prefix, all / 1024, max / 1024);
    QString id = prefix;
    FileTransferList::instance()->updateStatus(id, all / 1024, max / 1024);
}

static void _onDataMoveCompleted(Arc::DataMover* mover, Arc::DataStatus res, void* args)
{
    using namespace std;

    // clean up joblinks created during cache procedure

    if (!res.Passed())
    {
        if (!res.GetDesc().empty())
            logger.msg(Arc::ERROR, "Transfer FAILED: %s - %s", std::string(res), res.GetDesc());
        else
            logger.msg(Arc::ERROR, "Transfer FAILED: %s", std::string(res));
        if (res.Retryable())
            logger.msg(Arc::ERROR, "This seems like a temporary error, please try again later");
    }
    else
        logger.msg(Arc::INFO, "Transfer completed ok");

    FileTransfer* xfr = (FileTransfer*)args;
    xfr->completed(res, res.GetDesc());
}

FileTransfer::FileTransfer(const std::string& source_str, const std::string& destination_str, Arc::UserConfig& usercfg)
    : m_sourceUrl(source_str),
      m_destUrl(destination_str),
      m_sourceHandle(m_sourceUrl, usercfg),
      m_destHandle(m_destUrl, usercfg)
{
    m_config = &usercfg;

    // Retrieve global settings

    m_retries = GlobalStateInfo::instance()->transferRetries();
    m_timeout = GlobalStateInfo::instance()->transferTimeout();
    m_passive = GlobalStateInfo::instance()->passiveTransfers();
    m_secure = GlobalStateInfo::instance()->secureTransfers();

    /*
    m_retries = 3;
    m_timeout = 300;
    m_passive = false;
    m_notpassive = false;
    m_secure = false;
    */

    m_force = false;
    m_verbose = true;
    m_nocopy = false;
    m_recursion = 0;

    m_mover = 0;
    m_cache = 0;
    m_urlMap = 0;

    m_id = convertPointerToStringAddress(this);
    m_transferred = 0;
    m_totalSize = 0;
    m_transferState = TS_IDLE;
}

FileTransfer::~FileTransfer()
{
    if (m_cache!=0)
        delete m_cache;

    if (m_mover!=0)
        delete m_mover;

    if (m_urlMap!=0)
        delete m_urlMap;
}

TTransferState FileTransfer::transferState()
{
    return m_transferState;
}


QString FileTransfer::id()
{
    QString id = m_id.c_str();
    return id;
}

QString FileTransfer::sourceUrl()
{
    QString sourceUrl = m_sourceUrl.str().c_str();
    return sourceUrl;
}

QString FileTransfer::destUrl()
{
    QString destUrl = m_destUrl.str().c_str();
    return destUrl;
}

void FileTransfer::updateTransferStatus(unsigned long transferred, unsigned long totalSize)
{
    m_transferred = transferred;
    m_totalSize = totalSize;
}

void FileTransfer::getTransferStatus(unsigned long& transferred, unsigned long& totalSize)
{
    transferred = m_transferred;
    totalSize = m_totalSize;
}

bool FileTransfer::execute()
{
    using namespace std;

    m_transferState = TS_EXECUTED;

    logger.msg(Arc::INFO, "File transfer initiating.");

    m_completed = false;

    if ((!m_secure) && (!m_notpassive))
        m_passive = true;

    if (!m_sourceUrl)
    {
        m_transferState = TS_FAILED;
        logger.msg(Arc::ERROR, "Invalid URL: %s", m_sourceUrl.str());
        return false;
    }

    if (!m_destUrl)
    {
        m_transferState = TS_FAILED;
        logger.msg(Arc::ERROR, "Invalid URL: %s", m_destUrl.str());
        return false;
    }

    // Make sure credentials are ok if one of the protocols are secure

#if ARC_VERSION_MAJOR >= 3
    if (m_sourceHandle->RequiresCredentials() || m_destHandle->RequiresCredentials())
#else
    if (m_sourceUrl.IsSecureProtocol() || m_destUrl.IsSecureProtocol())
#endif
    {
        m_config->InitializeCredentials(Arc::initializeCredentialsType::TryCredentials);
        if (!Arc::Credential::IsCredentialsValid(*m_config))
        {
            m_transferState = TS_FAILED;
            logger.msg(Arc::ERROR, "Unable to copy file %s: No valid credentials found", m_sourceUrl.str());
            return false;
        }
    }

    if (!m_sourceHandle)
    {
        m_transferState = TS_FAILED;
        logger.msg(Arc::ERROR, "Unsupported source url: %s", m_sourceUrl.str());
        return false;
    }
    if (!m_destHandle)
    {
        m_transferState = TS_FAILED;
        logger.msg(Arc::ERROR, "Unsupported destination url: %s", m_destUrl.str());
        return false;
    }

    if (m_urlMap!=0)
        delete m_urlMap;

    m_urlMap = new Arc::URLMap();

    if (m_mover!=0)
        delete m_mover;

    m_mover = new Arc::DataMover();

    m_mover->secure(m_secure);
    m_mover->passive(m_passive);
    m_mover->verbose(m_verbose);

    if (m_retries)   // 0 means default behavior
    {
        m_mover->retry(true); // go th/home/jonasrough all locations
        m_sourceHandle->SetTries(m_retries); // try all locations "tries" times
        m_destHandle->SetTries(m_retries);
    }

    if (m_cache!=0)
        delete m_cache;

    m_cache = new Arc::FileCache();

    // Add callback for progress information.

    if (m_verbose)
        m_mover->set_progress_indicator(&_onProgress);

    // Do the actual transfer. Attach callback onDataMoveCompleted for notification of completion.

    logger.msg(Arc::INFO, "Transfer process started.");

    Arc::DataStatus res = m_mover->Transfer(*m_sourceHandle, *m_destHandle, *m_cache, *m_urlMap, &_onDataMoveCompleted, this, m_id.c_str());
    return true;
}

void FileTransfer::wait()
{
    // Wait for transfer to complete.

    m_cond.wait();
}

void FileTransfer::cancel()
{
    if (m_mover!=0)
        delete m_mover;
}

void FileTransfer::completed(Arc::DataStatus res, std::string error)
{
    m_transferState = TS_COMPLETED;
    logger.msg(Arc::INFO, "FileTransfer completed.");

    // Release waiting signal

    m_cond.signal();
    m_completed = true;

    QString errorMessage = error.c_str();

    logger.msg(Arc::DEBUG, "FileTransfer::completed -> sending signal onCompleted().");
    Q_EMIT onCompleted(this, true, errorMessage);
}

Arc::DataStatus FileTransfer::status()
{
    return m_status;
}


bool FileTransfer::isCompleted()
{
    return m_completed;
}
