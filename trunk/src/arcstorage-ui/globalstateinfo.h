#ifndef GLOBALSTATEINFO_H
#define GLOBALSTATEINFO_H

#include <QList>

#include "mainwindow.h"
#include "transferlistwindow.h"

class GlobalStateInfo : public QObject
{
    Q_OBJECT
private:
    TransferListWindow* m_transferListWindow;
    QList<MainWindow*> m_childWindows;
    MainWindow* m_mainWindow;
public:
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

    void setMainWindow(MainWindow* window);
    void addChildWindow(MainWindow* window);
    void removeChildWindow(MainWindow* window);
    void closeChildWindows();
    void showTransferWindow();
    void hideTransferWindow();
    int childWindowCount();
    MainWindow* getChildWindow(int idx);

    void updateWindowList(QMenu* menu);

private:

    GlobalStateInfo();

    static GlobalStateInfo* m_instance;
};

#endif // GLOBALSTATEINFO_H
