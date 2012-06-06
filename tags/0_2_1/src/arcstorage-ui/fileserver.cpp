#include "fileserver.h"

FileServer::FileServer(MainWindow *mw)
{
    mainWindow = mw;
    m_notifyParent = true;
}

void FileServer::setNotifyParent(bool flag)
{
    m_notifyParent = flag;
}

bool FileServer::getNotifyParent()
{
    return m_notifyParent;
}

