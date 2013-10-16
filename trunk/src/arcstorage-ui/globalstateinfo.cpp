#include "globalstateinfo.h"

#include <QMenu>

#include <arc/Logger.h>

GlobalStateInfo* GlobalStateInfo::m_instance = 0;

GlobalStateInfo::GlobalStateInfo()
{
    m_transferListWindow = 0;
    m_mainWindow = 0;

    m_retries = 3;
    m_timeout = 300;

    m_passive = true;
    m_notpassive = false;
    m_force = false;
    m_verbose = true;
    m_nocopy = false;
    m_secure = false;

    m_logLevel = LL_WARNING;
    m_redirectLog = true;

    m_maxTransfers = 5;

    m_transferThreadWakeupInterval = 1;

    m_rememberWindowsPositions = true;
    m_rememberStartupDirs = true;
    m_newWindowUrl = "";

    m_windowMenu = new QMenu();
}

void GlobalStateInfo::writeSettings()
{
    QSettings settings;
    settings.remove("Transfers");
    settings.beginGroup("Transfers");
    settings.setValue("max", m_maxTransfers);
    settings.setValue("passive", m_passive);
    settings.setValue("secure", m_secure);
    settings.setValue("retries", m_retries);
    settings.setValue("timeout", m_timeout);
    settings.endGroup();

    settings.remove("Windows");
    settings.beginGroup("Windows");
    settings.setValue("remember_pos", m_rememberWindowsPositions);
    settings.setValue("remember_url", m_rememberStartupDirs);
    settings.setValue("startup_url", m_newWindowUrl);
    settings.endGroup();

    settings.remove("Logging");
    settings.beginGroup("Logging");
    settings.setValue("log_level", m_logLevel);
    settings.setValue("redirect_log", m_redirectLog);
    settings.endGroup();

    settings.remove("Bookmarks");
    settings.beginGroup("Bookmarks");
    for (int i=0; i<this->bookmarkCount(); i++)
        settings.setValue("url"+QString::number(i), this->bookmark(i));
    settings.endGroup();
}

void GlobalStateInfo::readSettings()
{
    QSettings settings;

    if (settings.childGroups().contains("Transfers"))
    {
        settings.beginGroup("Transfers");
        m_maxTransfers = settings.value("max").toInt();
        m_passive = settings.value("passive").toBool();
        m_secure = settings.value("secure").toBool();
        m_retries = settings.value("retries").toInt();
        m_timeout = settings.value("timeout").toInt();
        settings.endGroup();
    }
    if (settings.childGroups().contains("Windows"))
    {
        settings.beginGroup("Windows");
        m_rememberWindowsPositions = settings.value("remember_pos").toBool();
        m_rememberStartupDirs = settings.value("remember_url").toBool();
        m_newWindowUrl = settings.value("startup_url").toString();
        settings.endGroup();
    }
    if (settings.childGroups().contains("Logging"))
    {
        settings.beginGroup("Logging");
        m_rememberWindowsPositions = settings.value("log_level").toInt();
        m_redirectLog = settings.value("redirect_log", true).toBool();
        settings.endGroup();
    }
    if (settings.childGroups().contains("Bookmarks"))
    {
        settings.beginGroup("Bookmarks");
        this->clearBookmarks();
        for (int i=0; i<100; i++)
        {
            QString urlKey = "url"+QString::number(i);
            if (settings.contains(urlKey))
            {
                QString url = settings.value(urlKey, "").toString();
                this->addBookmark(url);
            }
        }
        settings.endGroup();
    }
}


void GlobalStateInfo::setMainWindow(ArcStorageWindow* window)
{
    m_mainWindow = window;
}

ArcStorageWindow* GlobalStateInfo::mainWindow()
{
    return m_mainWindow;
}

QMenu* GlobalStateInfo::windowMenu()
{
    return m_windowMenu;
}

void GlobalStateInfo::addChildWindow(ArcStorageWindow* window)
{
    m_childWindows.append(window);
    this->enumerateWindows();
}

void GlobalStateInfo::removeChildWindow(ArcStorageWindow* window)
{
    m_childWindows.removeOne(window);
    delete window;
    this->enumerateWindows();
}

void GlobalStateInfo::closeChildWindows()
{
    for (int i=0; i<m_childWindows.count(); i++)
    {
        m_childWindows.at(i)->close();
    }
    m_childWindows.clear();
}

int GlobalStateInfo::childWindowCount()
{
    return m_childWindows.count();
}

void GlobalStateInfo::enumerateWindows()
{
    int counter = 0;
    for (int i=0; i<m_childWindows.count(); i++)
        m_childWindows.at(i)->setWindowId(counter++);
}


ArcStorageWindow* GlobalStateInfo::getChildWindow(int idx)
{
    if ((idx>=0)&&(idx<m_childWindows.count()))
        return m_childWindows.at(idx);
    else
        return 0;
}

void GlobalStateInfo::updateWindowList(QMenu* menu)
{
}

void GlobalStateInfo::showTransferWindow()
{
    if (m_transferListWindow == 0)
        m_transferListWindow = new TransferListWindow(m_mainWindow);

    Qt::WindowFlags flags = m_transferListWindow->windowFlags();
    m_transferListWindow->setWindowFlags(flags | Qt::WindowStaysOnTopHint);
    m_transferListWindow->setGeometry(
        QStyle::alignedRect(
            Qt::LeftToRight,
            Qt::AlignCenter,
            m_transferListWindow->size(),
            qApp->desktop()->availableGeometry()
        ));
    m_transferListWindow->show();
}

void GlobalStateInfo::hideTransferWindow()
{
    if (m_transferListWindow!=0)
        m_transferListWindow->hide();
}

void GlobalStateInfo::setTransferTimeout(int timeout)
{
    m_timeout = timeout;
}

int GlobalStateInfo::transferTimeout()
{
    return m_timeout;
}

void GlobalStateInfo::setTransferRetries(int retries)
{
    m_retries = retries;
}

int GlobalStateInfo::transferRetries()
{
    return m_retries;
}

void GlobalStateInfo::setSecureTransfers(bool secure)
{
    m_secure = secure;
}

bool GlobalStateInfo::secureTransfers()
{
    return m_secure;
}

void GlobalStateInfo::setPassiveTransfers(bool passive)
{
    m_passive = passive;
}

bool GlobalStateInfo::passiveTransfers()
{
    return m_passive;
}

void GlobalStateInfo::setMaxTransfers(int maxTransfers)
{
    m_maxTransfers = maxTransfers;
}

int GlobalStateInfo::maxTransfers()
{
    return m_maxTransfers;
}

void GlobalStateInfo::setTransferThreadWakeUpInterval(int interval)
{
    m_transferThreadWakeupInterval = interval;
}

int GlobalStateInfo::transferThreadWakeUpInterval()
{
    return m_transferThreadWakeupInterval;
}

void GlobalStateInfo::setLogLevel(TLogLevel level)
{
    m_logLevel = level;
    if (m_logLevel == LL_VERBOSE)
        Arc::Logger::getRootLogger().setThreshold(Arc::VERBOSE);
    if (m_logLevel == LL_INFO)
        Arc::Logger::getRootLogger().setThreshold(Arc::INFO);
    if (m_logLevel == LL_WARNING)
        Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);
    if (m_logLevel == LL_ERROR)
        Arc::Logger::getRootLogger().setThreshold(Arc::ERROR);
}

GlobalStateInfo::TLogLevel GlobalStateInfo::logLevel()
{
    if (Arc::Logger::getRootLogger().getThreshold()==Arc::VERBOSE)
        m_logLevel = LL_VERBOSE;
    if (Arc::Logger::getRootLogger().getThreshold()==Arc::INFO)
        m_logLevel = LL_INFO;
    if (Arc::Logger::getRootLogger().getThreshold()==Arc::WARNING)
        m_logLevel = LL_WARNING;
    if (Arc::Logger::getRootLogger().getThreshold()==Arc::ERROR)
        m_logLevel = LL_ERROR;
    return m_logLevel;
}

void GlobalStateInfo::setRedirectLog(bool flag)
{
    m_redirectLog = flag;
}

bool GlobalStateInfo::redirectLog()
{
    return m_redirectLog;
}

void GlobalStateInfo::setRememberWindowPositions(bool flag)
{
    m_rememberWindowsPositions = flag;
}

bool GlobalStateInfo::rememberWindowPositions()
{
    return m_rememberWindowsPositions;
}

void GlobalStateInfo::setRememberStartupDirs(bool flag)
{
    m_rememberStartupDirs = flag;
}

bool GlobalStateInfo::rememberStartupDirs()
{
    return m_rememberStartupDirs;
}

void GlobalStateInfo::setNewWindowUrl(QString url)
{
    m_newWindowUrl = url;
}

QString GlobalStateInfo::newWindowUrl()
{
    return m_newWindowUrl;
}

void GlobalStateInfo::addBookmark(QString url)
{
    m_bookmarks.append(url);
    this->mainWindow()->updateBookmarkMenu();
    for (int i=0; i<m_childWindows.count(); i++)
        m_childWindows.at(i)->updateBookmarkMenu();
}

void GlobalStateInfo::removeBookmark(QString url)
{
    m_bookmarks.removeOne(url);
    this->mainWindow()->updateBookmarkMenu();
    for (int i=0; i<m_childWindows.count(); i++)
        m_childWindows.at(i)->updateBookmarkMenu();
}

void GlobalStateInfo::removeBookmark(int idx)
{
    m_bookmarks.removeAt(idx);
    this->mainWindow()->updateBookmarkMenu();
    for (int i=0; i<m_childWindows.count(); i++)
        m_childWindows.at(i)->updateBookmarkMenu();
}

QString GlobalStateInfo::bookmark(int idx)
{
    return m_bookmarks.at(idx);
}

int GlobalStateInfo::bookmarkCount()
{
    return m_bookmarks.count();
}

void GlobalStateInfo::clearBookmarks()
{
    m_bookmarks.clear();
    this->mainWindow()->updateBookmarkMenu();
    for (int i=0; i<m_childWindows.count(); i++)
        m_childWindows.at(i)->updateBookmarkMenu();
}


