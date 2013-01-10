#ifndef GLOBALSTATEINFO_H
#define GLOBALSTATEINFO_H

#include <QList>

#include "arcstoragewindow.h"
#include "transferlistwindow.h"

class GlobalStateInfo : public QObject
{
    Q_OBJECT
private:
    TransferListWindow* m_transferListWindow;
    QList<ArcStorageWindow*> m_childWindows;
    ArcStorageWindow* m_mainWindow;
public:
    enum TLogLevel {LL_VERBOSE, LL_INFO, LL_WARNING, LL_ERROR};
    static GlobalStateInfo* instance()
    {
        static QMutex mutex;
        if (!m_instance)
        {
            mutex.lock();

            if (!m_instance)
                m_instance = new GlobalStateInfo;

            mutex.unlock();
        }

        return m_instance;
    }

    static void drop()
    {
        static QMutex mutex;
        mutex.lock();
        delete m_instance;
        m_instance = 0;
        mutex.unlock();
    }

    void writeSettings();
    void readSettings();

    void setMainWindow(ArcStorageWindow* window);
    void addChildWindow(ArcStorageWindow* window);
    void removeChildWindow(ArcStorageWindow* window);
    void closeChildWindows();
    void showTransferWindow();
    void hideTransferWindow();
    int childWindowCount();
    ArcStorageWindow* getChildWindow(int idx);

    void updateWindowList(QMenu* menu);

    void enumerateWindows();

    void setTransferTimeout(int timeout);
    int transferTimeout();

    void setTransferRetries(int retries);
    int transferRetries();

    void setSecureTransfers(bool secure);
    bool secureTransfers();

    void setPassiveTransfers(bool passive);
    bool passiveTransfers();

    void setMaxTransfers(int maxTransfers);
    int maxTransfers();

    void setTransferThreadWakeUpInterval(int interval);
    int transferThreadWakeUpInterval();

    void setLogLevel(TLogLevel level);
    TLogLevel logLevel();

    void setRememberWindowPositions(bool flag);
    bool rememberWindowPositions();

    void setRememberStartupDirs(bool flag);
    bool rememberStartupDirs();

    void setNewWindowUrl(QString url);
    QString newWindowUrl();

private:

    GlobalStateInfo();
    static GlobalStateInfo* m_instance;

    // General properties

    bool m_rememberWindowsPositions;
    bool m_rememberStartupDirs;
    QString m_newWindowUrl;
    TLogLevel m_logLevel;

    // File transfer options

    int m_timeout;
    int m_retries;
    bool m_secure;
    bool m_passive;
    bool m_notpassive;
    bool m_force;
    bool m_verbose;
    bool m_nocopy;

    // Transferlist properties

    int m_maxTransfers;
    int m_transferThreadWakeupInterval;
};

#endif // GLOBALSTATEINFO_H
