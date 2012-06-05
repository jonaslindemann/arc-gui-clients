#include "filetransferlist.h"

#include "arcstorage.h"

#include <iostream>
#include <string>
#include <sstream>

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
}

void FileTransferList::addTransfer(FileTransfer* fileTransfer)
{
    //logger.msg(Arc::INFO, "Adding "+fileTransfer->id().toStdString()+" to transfer list.");
    m_accessMutex.lock();
    m_transferList.append(fileTransfer);
    m_transferDict[fileTransfer->id()] = fileTransfer;
    Q_EMIT onAddTransfer(fileTransfer->id());
    m_accessMutex.unlock();
}

void FileTransferList::removeTransfer(FileTransfer* fileTransfer)
{
    //logger.msg(Arc::INFO, "Removing "+fileTransfer->id().toStdString()+" from transfer list.");
    m_accessMutex.lock();
    m_transferList.removeOne(fileTransfer);
    m_transferDict.remove(fileTransfer->id());
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
    m_accessMutex.lock();
    int count = m_transferList.count();
    m_accessMutex.unlock();
    return count;
}

FileTransfer* FileTransferList::getTransfer(QString id)
{
    m_accessMutex.lock();
    FileTransfer* xfr = 0;
    if (m_transferDict.contains(id))
        xfr = m_transferDict[id];
    m_accessMutex.unlock();
    return xfr;
}

void FileTransferList::updateStatus(QString id, unsigned long transferred, unsigned long totalSize)
{
    FileTransfer* xfr = this->getTransfer(id);
    m_accessMutex.lock();
    xfr->updateTransferStatus(transferred, totalSize);
    Q_EMIT onUpdateStatus(id);
    m_accessMutex.unlock();
}

