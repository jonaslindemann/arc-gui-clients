#include "arctools.h"

#include <QProcess>
#include <QDebug>

#include <arc/credential/Credential.h>

ARCTools* ARCTools::m_instance = 0;

ARCTools::ARCTools()
    :QObject()
{
    m_userConfig = 0;
}

void ARCTools::initUserConfig()
{
    qDebug() << "Creating ARC configuration.";
    m_userConfig = new Arc::UserConfig("", "", Arc::initializeCredentialsType(Arc::initializeCredentialsType::TryCredentials));

    if (m_userConfig->CredentialsFound())
        qDebug() << "Found credentials.";
    else
        qDebug() << "Credentials not found.";

    if (m_userConfig->InitializeCredentials(Arc::initializeCredentialsType(Arc::initializeCredentialsType::TryCredentials)))
        qDebug() << "Credentials initialised ok.";
    else
        qDebug() << "Credentials could not be initialised.";

    Arc::Credential userCredential(*m_userConfig, Arc::Credential::NoPassword());

    if (userCredential.InitProxyCertInfo())
        qDebug() << "Credentials are valid.";
    else
        qDebug() << "Credentials not valid.";

    time_t t = m_userConfig->CertificateLifeTime().GetPeriod();

    qDebug() << t;

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
