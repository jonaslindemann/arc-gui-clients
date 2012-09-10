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
    m_pause = false;
}

void FileTransferProcessingThread::shutdown()
{
    m_terminate = true;
}

void FileTransferProcessingThread::pause()
{
    m_pause = true;
}

void FileTransferProcessingThread::resume()
{
    m_pause = false;
}

void FileTransferProcessingThread::run()
{
    while(!m_terminate)
    {
        while(!m_pause)
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
    m_fileProcessingThread = 0;
}

void FileTransferList::setProcessingThread(FileTransferProcessingThread *processingThread)
{
    m_fileProcessingThread = processingThread;
}

void FileTransferList::pauseProcessing()
{
    if (m_fileProcessingThread!=0)
        m_fileProcessingThread->pause();
}

void FileTransferList::resumeProcessing()
{
    if (m_fileProcessingThread!=0)
        m_fileProcessingThread->resume();
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

void FileTransferList::cancelAllTransfers()
{
    m_accessMutex.lock();

    QList<FileTransfer*> removeTransferList;

    // Only remove idle transfers

    for (int i=0; i<m_transferList.length(); i++)
    {
        FileTransfer* xfr = m_transferList.at(i);

        if (xfr->transferState() == TS_IDLE)
            removeTransferList.append(xfr);
    }

    for (int i=0; i<removeTransferList.count(); i++)
    {
        FileTransfer* xfr = removeTransferList.at(i);
        m_transferList.removeOne(xfr);
        m_transferDict.remove(xfr->id());
        m_activeTransferList.removeOne(xfr);
        m_activeTransferDict.remove(xfr->id());
        Q_EMIT onRemoveTransfer(xfr->id());
        delete xfr;
    }
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

