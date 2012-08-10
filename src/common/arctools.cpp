#include "arctools.h"

#include <QProcess>
#include <QDebug>
#include <QMessageBox>

#include <arc/credential/Credential.h>

ARCTools* ARCTools::m_instance = 0;

ARCTools::ARCTools()
    :QObject()
{
    m_userConfig = 0;
    m_proxyController = new ArcProxyController();
}

void ARCTools::initUserConfig()
{
    m_proxyController->initialize();


    qDebug() << "Creating ARC configuration.";
    m_userConfig = new Arc::UserConfig("", "", Arc::initializeCredentialsType(Arc::initializeCredentialsType::TryCredentials));

    ArcProxyController::TCertStatus certStatus = m_proxyController->checkCert();

    // enum TCertStatus { CS_PATH_EMPTY, CS_NOT_FOUND, CS_INVALID_CONFIG, CS_CADIR_NOT_FOUND, CS_VALID };

    if (certStatus == ArcProxyController::CS_NOT_FOUND)
        QMessageBox::warning(0, "Proxy", "Could not certificates");
    else if (certStatus == ArcProxyController::CS_PATH_EMPTY)
        QMessageBox::warning(0, "Proxy", "Certificate path empty. Please Check your configuration.");
    else if (certStatus == ArcProxyController::CS_INVALID_CONFIG)
        QMessageBox::warning(0, "Proxy", "Certificate configuration invalid.");
    else if (certStatus == ArcProxyController::CS_CADIR_NOT_FOUND)
        QMessageBox::warning(0, "Proxy", "CA directory not found. Please check configuration.");
    else if (certStatus == ArcProxyController::CS_CADIR_NOT_FOUND)
        QMessageBox::warning(0, "Proxy", "CA directory not found. Please check configuration.");
    else if (certStatus == ArcProxyController::CS_VALID)
        qDebug() << "Certificate is valid.";

    ArcProxyController::TProxyStatus proxyStatus = m_proxyController->checkProxy();

    // enum TProxyStatus { PS_PATH_EMPTY, PS_NOT_FOUND, PS_EXPIRED, PS_NOT_VALID, PS_VALID };

    if (proxyStatus == ArcProxyController::PS_NOT_FOUND)
        m_proxyController->showProxyUI();
    else if (proxyStatus == ArcProxyController::PS_PATH_EMPTY)
        QMessageBox::warning(0, "Proxy", "Proxy path is empty. Please Check your configuration.");
    else if (proxyStatus == ArcProxyController::PS_EXPIRED)
        m_proxyController->showProxyUI();
    else if (proxyStatus == ArcProxyController::PS_NOT_VALID)
        m_proxyController->showProxyUI();
    else if (proxyStatus == ArcProxyController::PS_VALID)
        qDebug() << "Proxy is valid.";

    m_userConfig->UtilsDirPath(Arc::UserConfig::ARCUSERDIRECTORY);
}

Arc::UserConfig* ARCTools::currentUserConfig()
{
    return m_userConfig;
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
    process.startDetached("arcstat-ui");
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
