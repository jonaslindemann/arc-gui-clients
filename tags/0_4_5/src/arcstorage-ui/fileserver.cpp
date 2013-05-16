#include "fileserver.h"

FileServer::FileServer()
{
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

void FileServer::clearFileList()
{
    int i;

    for (i=0; i<fileList.size(); i++)
        delete fileList.at(i);

    fileList.clear();
}


