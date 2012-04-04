#include "filetransfer.h"

#include "arcstorage.h"

static void onProgress(FILE *o, const char*, unsigned int,
                     unsigned long long int all, unsigned long long int max,
                     double, double)
{
    logger.msg(Arc::INFO, "Transferred: %llu kB", all / 1024);
}

static void onDataMoveCompleted(Arc::DataMover* mover, Arc::DataStatus res, void* args)
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
    xfr->completed();
}

FileTransfer::FileTransfer(const std::string& source_str, const std::string& destination_str, Arc::UserConfig& usercfg)
    : m_sourceUrl(source_str),
      m_destUrl(destination_str),
      m_sourceHandle(m_sourceUrl, usercfg),
      m_destHandle(m_destUrl, usercfg)
{
    m_config = &usercfg;
    m_retries = 1;
    m_timeout = 300;

    m_passive = false;
    m_notpassive = false;
    m_force = false;
    m_verbose = true;
    m_nocopy = false;
    m_secure = false;
    m_recursion = 0;

    m_mover = 0;
    m_cache = 0;
    m_urlMap = 0;

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

bool FileTransfer::execute()
{
    using namespace std;

    logger.msg(Arc::INFO, "File transfer initiating.");

    m_completed = false;

    if ((!m_secure) && (!m_notpassive))
        m_passive = true;

    if (!m_sourceUrl)
    {
        logger.msg(Arc::ERROR, "Invalid URL: %s", m_sourceUrl.str());
        return false;
    }

    if (!m_destUrl)
    {
        logger.msg(Arc::ERROR, "Invalid URL: %s", m_destUrl.str());
        return false;
    }

    // Make sure credentials are ok if one of the protocols are secure

    if (m_sourceUrl.IsSecureProtocol() || m_destUrl.IsSecureProtocol())
    {
        m_config->InitializeCredentials();
        if (!Arc::Credential::IsCredentialsValid(*m_config))
        {
            logger.msg(Arc::ERROR, "Unable to copy file %s: No valid credentials found", m_sourceUrl.str());
            return false;
        }
    }

    if (!m_sourceHandle)
    {
        logger.msg(Arc::ERROR, "Unsupported source url: %s", m_sourceUrl.str());
        return false;
    }
    if (!m_destHandle)
    {
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
    //mover.force_to_meta(m_force_meta);

    if (m_retries)   // 0 means default behavior
    {
        m_mover->retry(true); // go through all locations
        m_sourceHandle->SetTries(m_retries); // try all locations "tries" times
        m_destHandle->SetTries(m_retries);
    }

    if (m_cache!=0)
        delete m_cache;

    m_cache = new Arc::FileCache();

    // Add callback for progress information.

    if (m_verbose)
        m_mover->set_progress_indicator(&onProgress);

    // Do the actual transfer. Attach callback onDataMoveCompleted for notification of completion.

    logger.msg(Arc::INFO, "Transfer process started.");

    Arc::DataStatus res = m_mover->Transfer(*m_sourceHandle, *m_destHandle, *m_cache, *m_urlMap, &onDataMoveCompleted, this);
    return true;
}

void FileTransfer::wait()
{
    // Wait for transfer to complete.

    m_cond.wait();
}

void FileTransfer::completed()
{
    // Release waiting signal

    m_cond.signal();
    m_completed = true;
}

Arc::DataStatus FileTransfer::status()
{
    return m_status;
}


bool FileTransfer::isCompleted()
{
    return m_completed;
}
