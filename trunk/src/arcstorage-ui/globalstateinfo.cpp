#include "globalstateinfo.h"

#include <QMenu>

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

    for (int i=0; i<m_childWindows.count(); i++)
        this->updateWindowList(m_childWindows.at(i)->getWindowListMenu());

    this->updateWindowList(m_mainWindow->getWindowListMenu());
}

void GlobalStateInfo::removeChildWindow(MainWindow* window)
{
    m_childWindows.removeOne(window);

    for (int i=0; i<m_childWindows.count(); i++)
        this->updateWindowList(m_childWindows.at(i)->getWindowListMenu());

    this->updateWindowList(m_mainWindow->getWindowListMenu());
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

int GlobalStateInfo::childWindowCount()
{
    return m_childWindows.count();
}

MainWindow* GlobalStateInfo::getChildWindow(int idx)
{
    if ((idx>=0)&&(idx<m_childWindows.count()))
        return m_childWindows.at(idx);
    else
        return 0;
}

void GlobalStateInfo::updateWindowList(QMenu* menu)
{
    menu->clear();
    menu->addMenu(m_mainWindow->getCurrentURL());
    for (int i=0; i<m_childWindows.count(); i++)
        menu->addMenu(m_childWindows.at(i)->getCurrentURL());
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
