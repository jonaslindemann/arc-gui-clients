#include "arctools.h"

#include <QProcess>
#include <QDebug>
#include <QMessageBox>
#include <QApplication>

#include <arc/credential/Credential.h>

ARCTools* ARCTools::m_instance = 0;

ARCTools::ARCTools()
    :QObject()
{
    m_userConfig = 0;
    m_proxyController = new ArcProxyController();
}

bool ARCTools::initUserConfig()
{
    m_proxyController->initialize();

    bool prereqFound = true;

    qDebug() << "Creating ARC configuration.";
    m_userConfig = new Arc::UserConfig("", "", Arc::initializeCredentialsType(Arc::initializeCredentialsType::TryCredentials));

    ArcProxyController::TCertStatus certStatus = m_proxyController->checkCert();

    // enum TCertStatus { CS_PATH_EMPTY, CS_NOT_FOUND, CS_INVALID_CONFIG, CS_CADIR_NOT_FOUND, CS_VALID };

    if (certStatus == ArcProxyController::CS_NOT_FOUND)
    {
        QMessageBox::warning(0, "Proxy", "Could not find certificates");
        prereqFound = false;
    }
    else if (certStatus == ArcProxyController::CS_PATH_EMPTY)
    {
        QMessageBox::warning(0, "Proxy", "Certificate path empty. Please Check your configuration.");
        prereqFound = false;
    }
    else if (certStatus == ArcProxyController::CS_INVALID_CONFIG)
    {
        QMessageBox::warning(0, "Proxy", "Certificate configuration invalid.");
        prereqFound = false;
    }
    else if (certStatus == ArcProxyController::CS_CADIR_NOT_FOUND)
    {
        QMessageBox::warning(0, "Proxy", "CA directory not found. Please check configuration.");
        prereqFound = false;
    }
    else if (certStatus == ArcProxyController::CS_VALID)
        qDebug() << "Certificate is valid.";

    ArcProxyController::TProxyStatus proxyStatus = m_proxyController->checkProxy();

    // enum TProxyStatus { PS_PATH_EMPTY, PS_NOT_FOUND, PS_EXPIRED, PS_NOT_VALID, PS_VALID };

    if (proxyStatus == ArcProxyController::PS_NOT_FOUND)
        m_proxyController->showProxyUI();
    else if (proxyStatus == ArcProxyController::PS_PATH_EMPTY)
    {
        QMessageBox::warning(0, "Proxy", "Proxy path is empty. Please Check your configuration.");
        prereqFound = false;
    }
    else if (proxyStatus == ArcProxyController::PS_EXPIRED)
        m_proxyController->showProxyUI();
    else if (proxyStatus == ArcProxyController::PS_NOT_VALID)
        m_proxyController->showProxyUI();
    else if (proxyStatus == ArcProxyController::PS_VALID)
        qDebug() << "Proxy is valid.";


    if (m_proxyController->getUiReturnStatus()==ArcProxyController::RS_FAILED)
    {
        qDebug() << "GUI returns RS_FAILED.";
        prereqFound = false;
    }
    else
    {
        qDebug() << "GUI returns RS_OK";
        prereqFound = true;
    }

    if (!prereqFound)
        return false;

    delete m_userConfig;

    m_userConfig = new Arc::UserConfig("", "", Arc::initializeCredentialsType(Arc::initializeCredentialsType::TryCredentials));
    m_userConfig->UtilsDirPath(Arc::UserConfig::ARCUSERDIRECTORY);

    return true;
}

Arc::UserConfig* ARCTools::currentUserConfig()
{
    if (m_userConfig == 0)
        this->initUserConfig();
    return m_userConfig;
}

bool ARCTools::hasValidProxy()
{
    return m_proxyController->checkProxy() == ArcProxyController::PS_VALID;
}

void ARCTools::setJobListFile(QString filename)
{
    m_jobListFile = filename;
}

QString ARCTools::jobListFile()
{
    return m_jobListFile;
}


void ARCTools::proxyCertificateTool()
{
    QProcess process;
    process.startDetached("arcproxy-ui");
}

void ARCTools::certConversionTool()
{
    QProcess process;
    process.startDetached("arccert-ui");
}

void ARCTools::jobManagerTool()
{
    QProcess process;
    if (m_jobListFile.length()==0)
        process.startDetached("/home/jonas/Development/arc-gui-clients-build/src/arcstat-ui/arcstat-ui");
    else
        process.startDetached("/home/jonas/Development/arc-gui-clients-build/src/arcstat-ui/arcstat-ui "+m_jobListFile);
}

void ARCTools::submissionTool()
{
    QProcess process;
    process.startDetached("arcsub-ui");
}

void ARCTools::storageTool()
{
    QProcess process;
    process.startDetached("arcstorage-ui");
}
