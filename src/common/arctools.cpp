#include "arctools.h"

#include <QProcess>
#include <QDebug>
#include <QMessageBox>
#include <QApplication>
#include <QDesktopWidget>

#include <arc/credential/Credential.h>

#define ARCGUI_DEVEL

#ifdef ARCGUI_DEVEL
#define ARCGUI_BUILDDIR /home/jonas/Development/arc-gui-clients-build
#else
#define ARCGUI_BUILDDIR
#endif

ARCTools* ARCTools::m_instance = 0;

ARCTools::ARCTools()
    :QObject()
{
    m_userConfig = 0;
    m_proxyController = new ArcProxyController();
    m_helpWindow = 0;
}

bool ARCTools::initUserConfig(bool showUi)
{
    m_proxyController->initialize();

    bool prereqFound = true;

    m_userConfig = new Arc::UserConfig("", "", Arc::initializeCredentialsType(Arc::initializeCredentialsType::TryCredentials));

    ArcProxyController::TCertStatus certStatus = m_proxyController->checkCert();

    // enum TCertStatus { CS_PATH_EMPTY, CS_NOT_FOUND, CS_INVALID_CONFIG, CS_CADIR_NOT_FOUND, CS_VALID };

    if (certStatus == ArcProxyController::CS_NOT_FOUND)
    {
        //QMessageBox::warning(0, "Proxy", "Could not find certificates");
        prereqFound = false;
    }
    else if (certStatus == ArcProxyController::CS_PATH_EMPTY)
    {
        //QMessageBox::warning(0, "Proxy", "Certificate path empty. Please Check your configuration.");
        prereqFound = false;
    }
    else if (certStatus == ArcProxyController::CS_INVALID_CONFIG)
    {
        //QMessageBox::warning(0, "Proxy", "Certificate configuration invalid.");
        prereqFound = false;
    }
    else if (certStatus == ArcProxyController::CS_CADIR_NOT_FOUND)
    {
        QMessageBox::warning(0, "Proxy", "CA directory not found. Please check configuration.");
        prereqFound = false;
    }
    ArcProxyController::TProxyStatus proxyStatus = m_proxyController->checkProxy();

    if (proxyStatus == ArcProxyController::PS_NOT_FOUND)
        m_proxyController->showProxyUI();
    else if (proxyStatus == ArcProxyController::PS_PATH_EMPTY)
    {
        QMessageBox::warning(0, "Proxy", "Proxy path is empty. Please Check your configuration.");
        prereqFound = false;
        m_proxyController->showProxyUI();
    }
    else if (proxyStatus == ArcProxyController::PS_EXPIRED)
        m_proxyController->showProxyUI();
    else if (proxyStatus == ArcProxyController::PS_NOT_VALID)
        m_proxyController->showProxyUI();
    else if (showUi)
        m_proxyController->showProxyUI();

    if (m_proxyController->getUiReturnStatus()==ArcProxyController::RS_FAILED)
        prereqFound = false;
    else
        prereqFound = true;

    if (!prereqFound)
        return false;

    delete m_userConfig;


    //m_userConfig = new Arc::UserConfig("", "", Arc::initializeCredentialsType(Arc::initializeCredentialsType::TryCredentials));
    m_userConfig = new Arc::UserConfig("", Arc::initializeCredentialsType(Arc::initializeCredentialsType::TryCredentials));
#if ARC_VERSION_MAJOR >= 6
    m_userConfig->UtilsDirPath(Arc::UserConfig::ARCUSERDIRECTORY());
#else
    m_userConfig->UtilsDirPath(Arc::UserConfig::ARCUSERDIRECTORY);
#endif
    //m_userConfig->CACertificatePath("/etc/grid-security/certificates");
    m_userConfig->CACertificatesDirectory("/etc/grid-security/certificates");

    //if (!m_userConfig) {
    //  logger.msg(Arc::ERROR, "Failed configuration initialization");
    //  return 1;
    //}


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
#ifdef ARCGUI_DEVEL
    if (m_jobListFile.length()==0)
        process.startDetached("/home/jonas/Development/arc-gui-clients-build/src/arcstat-ui/arcstat-ui");
    else
        process.startDetached("/home/jonas/Development/arc-gui-clients-build/src/arcstat-ui/arcstat-ui "+m_jobListFile);
#else
    if (m_jobListFile.length()==0)
        process.startDetached("arcstat-ui");
    else
        process.startDetached("arcstat-ui "+m_jobListFile);
#endif
}

void ARCTools::submissionTool()
{
    QProcess process;
#ifdef ARCGUI_DEVEL
    process.startDetached("/home/jonas/Development/arc-gui-clients-build/src/arcsub-ui/arcsub-ui");
#else
    process.startDetached("arcsub-ui");
#endif
}

void ARCTools::storageTool()
{
    QProcess process;
#ifdef ARCGUI_DEVEL
    process.startDetached("/home/jonas/Development/arc-gui-clients-build/src/arcstorage-ui/arcstorage-ui");
#else
    process.startDetached("arcstorage-ui");
#endif
}

void ARCTools::showHelpWindow(QMainWindow* window)
{
    if (m_helpWindow == 0)
    {
        m_helpWindow = new HelpWindow(window);

        int x, y;
        int screenWidth;
        int screenHeight;

        QDesktopWidget *desktop = QApplication::desktop();

        screenWidth = desktop->width();
        screenHeight = desktop->height();

        x = (screenWidth - m_helpWindow->width()) / 2;
        y = (screenHeight - m_helpWindow->height()) / 2;

        m_helpWindow->setGeometry(x, y, m_helpWindow->width(), m_helpWindow->height());
    }

    m_helpWindow->show();
    m_helpWindow->raise();
}

void ARCTools::closeHelpWindow()
{
    if (m_helpWindow!=0)
    {
        m_helpWindow->close();
        delete m_helpWindow;
    }
}
