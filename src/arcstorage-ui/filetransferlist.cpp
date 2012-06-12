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
    qDebug() << "File processing thread starting.";

    while(!m_terminate)
    {
        qDebug() << "Processing file transfers...";
        FileTransferList::instance()->processTransfers();
        qDebug() << "Sleeping for 5 s";
        sleep(5);
    }

    qDebug() << "File processing thread shutting down.";
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
                qDebug() << "Starting file transfer id = " << xfr->id();
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
    //m_accessMutex.lock();
    int count = m_transferList.count();
    //m_accessMutex.unlock();
    return count;
}

FileTransfer* FileTransferList::getTransfer(QString id)
{
    //m_accessMutex.lock();
    FileTransfer* xfr = 0;
    if (m_transferDict.contains(id))
        xfr = m_transferDict[id];
    //m_accessMutex.unlock();
    return xfr;
}

void FileTransferList::updateStatus(QString id, unsigned long transferred, unsigned long totalSize)
{
    FileTransfer* xfr = this->getTransfer(id);
    //m_accessMutex.lock();
    xfr->updateTransferStatus(transferred, totalSize);
    Q_EMIT onUpdateStatus(id);
    //m_accessMutex.unlock();
}

