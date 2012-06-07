#include "globalstateinfo.h"

GlobalStateInfo* GlobalStateInfo::m_instance = 0;

GlobalStateInfo::GlobalStateInfo()
{
    m_transferListWindow = 0;
    m_mainWindow = 0;
}

void GlobalStateInfo::setMainWindow(MainWindow* window)
{
    m_mainWindow = window;
}


void GlobalStateInfo::addChildWindow(MainWindow* window)
{
    m_childWindows.append(window);
}

void GlobalStateInfo::closeChildWindows()
{
    for (int i=0; i<m_childWindows.count(); i++)
    {
        m_childWindows.at(i)->close();
        delete m_childWindows.at(i);
    }
    m_childWindows.clear();
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
