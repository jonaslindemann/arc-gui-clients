#include "filetransferlist.h"

#include "arcstorage.h"

#include <iostream>
#include <string>
#include <sstream>

#include <QDebug>

FileTransferProcessingThread::FileTransferProcessingThread()
    :QThread()
{
    m_terminate = false;
}

void FileTransferProcessingThread::shutdown()
{
    m_terminate = true;
}

void FileTransferProcessingThread::run()
{
    while(!m_terminate)
    {
        FileTransferList::instance()->processTransfers();
        sleep(1);
    }
}

FileTransferList* FileTransferList::m_instance = 0;

template <typename T>
std::string convertPointerToStringAddress(const T* obj)
{
  int address(reinterpret_cast<int>(obj));
  std::stringstream ss;
  ss << address;
  return ss.str();
}

FileTransferList::FileTransferList()
{
    m_maxTransfers = 5;
}

void FileTransferList::processTransfers()
{
    m_accessMutex.lock();
    for (int i=0; i<m_transferList.length(); i++)
    {
        FileTransfer* xfr = m_transferList.at(i);
        if (xfr->transferState()==TS_IDLE)
        {
            if (m_activeTransferList.length()<m_maxTransfers)
            {
                m_activeTransferList.append(xfr);
                m_activeTransferDict[xfr->id()] = xfr;
                xfr->execute();
            }
        }
    }
    m_accessMutex.unlock();
}

void FileTransferList::addTransfer(FileTransfer* fileTransfer)
{
    m_accessMutex.lock();
    m_transferList.append(fileTransfer);
    m_transferDict[fileTransfer->id()] = fileTransfer;
    Q_EMIT onAddTransfer(fileTransfer->id());
    m_accessMutex.unlock();
}

void FileTransferList::removeTransfer(FileTransfer* fileTransfer)
{
    m_accessMutex.lock();
    m_transferList.removeOne(fileTransfer);
    m_transferDict.remove(fileTransfer->id());
    m_activeTransferList.removeOne(fileTransfer);
    m_activeTransferDict.remove(fileTransfer->id());
    Q_EMIT onRemoveTransfer(fileTransfer->id());
    m_accessMutex.unlock();
}

FileTransfer* FileTransferList::getTransfer(int i)
{
    m_accessMutex.lock();
    FileTransfer* xfr = m_transferList.at(i);
    m_accessMutex.unlock();
    return xfr;
}

int FileTransferList::getTransferCount()
{
    int count = m_transferList.count();
    return count;
}

FileTransfer* FileTransferList::getTransfer(QString id)
{
    FileTransfer* xfr = 0;
    if (m_transferDict.contains(id))
        xfr = m_transferDict[id];
    return xfr;
}

void FileTransferList::updateStatus(QString id, unsigned long transferred, unsigned long totalSize)
{
    FileTransfer* xfr = this->getTransfer(id);
    xfr->updateTransferStatus(transferred, totalSize);
    Q_EMIT onUpdateStatus(id);
}

