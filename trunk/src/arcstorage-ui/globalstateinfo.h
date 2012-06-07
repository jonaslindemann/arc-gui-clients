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
    void closeChildWindows();
    void showTransferWindow();

private:

    GlobalStateInfo();

    GlobalStateInfo(const GlobalStateInfo &); // hide copy constructor
    GlobalStateInfo& operator=(const GlobalStateInfo &); // hide assign op
    // we leave just the declarations, so the compiler will warn us
    // if we try to use those two functions by accident

    static GlobalStateInfo* m_instance;
};

#endif // GLOBALSTATEINFO_H
