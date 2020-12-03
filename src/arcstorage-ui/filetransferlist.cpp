#include "filetransferlist.h"
#include "globalstateinfo.h"
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

    FileTransferList::instance()->setProcessingThread(this);
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
        if (!m_pause)
            FileTransferList::instance()->processTransfers();

        sleep(GlobalStateInfo::instance()->transferThreadWakeUpInterval());
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
    m_maxTransfers = GlobalStateInfo::instance()->maxTransfers();
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
    int idleCount = 0;
    for (int i=0; i<m_transferList.length(); i++)
    {
        auto xfr = m_transferList.at(i);
        if (xfr->transferState()==TS_IDLE)
        {
            idleCount++;
            if (m_activeTransferList.length()<m_maxTransfers)
            {
                qDebug() << "processTransfers: Executing " << xfr->id();
                m_activeTransferList.append(xfr);
                m_activeTransferDict[xfr->id()] = xfr;
                xfr->execute();
            }
        }
    }
    m_accessMutex.unlock();
}

void FileTransferList::addTransfer(std::shared_ptr<FileTransfer> fileTransfer)
{
    m_accessMutex.lock();
    m_transferList.append(fileTransfer);
    m_transferDict[fileTransfer->id()] = fileTransfer;
    Q_EMIT onAddTransfer(fileTransfer->id());
    connect(fileTransfer.get(), SIGNAL(onCompleted(FileTransfer*, bool, QString)), this, SLOT(onCompleted(FileTransfer*, bool, QString)));
    m_accessMutex.unlock();
}

void FileTransferList::removeTransfer(std::shared_ptr<FileTransfer> fileTransfer)
{
    m_accessMutex.lock();
    m_transferList.removeOne(fileTransfer);
    m_transferDict.remove(fileTransfer->id());
    m_activeTransferList.removeOne(fileTransfer);
    m_activeTransferDict.remove(fileTransfer->id());
    Q_EMIT onRemoveTransfer(fileTransfer->id());
    m_accessMutex.unlock();
}

void FileTransferList::removeTransfer(FileTransfer* fileTransfer)
{
    m_accessMutex.lock();

    for (int i=0; i<m_transferList.size(); i++)
    {
       if (m_transferList.at(i).get() == fileTransfer)
       {
           m_transferList.removeAt(i);
           break;
       }
    }

    m_transferDict.remove(fileTransfer->id());

    for (int i=0; i<m_activeTransferList.size(); i++)
    {
        if (m_activeTransferList.at(i).get() == fileTransfer)
        {
            m_activeTransferList.removeAt(i);
            break;
        }
    }

    //m_activeTransferList.removeOne(fileTransfer);
    m_activeTransferDict.remove(fileTransfer->id());
    Q_EMIT onRemoveTransfer(fileTransfer->id());
    m_accessMutex.unlock();
}

void FileTransferList::cancelAllTransfers()
{
    m_accessMutex.lock();

    QList<std::shared_ptr<FileTransfer>> removeTransferList;

    // Only remove idle transfers

    for (int i=0; i<m_transferList.length(); i++)
    {
        auto xfr = m_transferList.at(i);

        if (xfr->transferState() == TS_IDLE)
            removeTransferList.append(xfr);
    }

    for (int i=0; i<removeTransferList.count(); i++)
    {
        auto xfr = removeTransferList.at(i);
        m_transferList.removeOne(xfr);
        m_transferDict.remove(xfr->id());
        m_activeTransferList.removeOne(xfr);
        m_activeTransferDict.remove(xfr->id());
        Q_EMIT onRemoveTransfer(xfr->id());
    }
    m_accessMutex.unlock();
}

std::shared_ptr<FileTransfer> FileTransferList::getTransfer(int i)
{
    m_accessMutex.lock();
    auto xfr = m_transferList.at(i);
    m_accessMutex.unlock();
    return xfr;
}

int FileTransferList::getTransferCount()
{
    int count = m_transferList.count();
    return count;
}

std::shared_ptr<FileTransfer> FileTransferList::getTransfer(QString id)
{
    std::shared_ptr<FileTransfer> xfr;
    if (m_transferDict.contains(id))
        xfr = m_transferDict[id];
    return xfr;
}

void FileTransferList::updateStatus(QString id, unsigned long transferred, unsigned long totalSize)
{
    auto xfr = this->getTransfer(id);
    xfr->updateTransferStatus(transferred, totalSize);
    Q_EMIT onUpdateStatus(id);
}

void FileTransferList::startMeasuring()
{
    m_transferTime = 0;
    m_totalTransferred = 0;
    m_transferTimer.start();
}

void FileTransferList::stopMeasuring()
{
    m_transferTime = m_transferTimer.elapsed();
    double transferBandwidth = ((double)m_totalTransferred/1024.0)/((double)m_transferTime/1000.0);
    qDebug() << "Average transfer bandwidth = " << transferBandwidth << "MB/s";
}

unsigned long FileTransferList::totalTransferred()
{
    return m_totalTransferred;
}

double FileTransferList::transferTime()
{
    return (double)m_transferTime/1000.0;
}

int FileTransferList::maxTransfers()
{
    return m_maxTransfers;
}

void FileTransferList::setMaxTransfers(int maxTransfers)
{
    m_maxTransfers = maxTransfers;
}

void FileTransferList::onCompleted(FileTransfer* fileTransfer, bool success, QString error)
{
    qDebug() << "(FileTransferList) File transfer " << fileTransfer->id() << " completed.";

    if (success)
        m_totalTransferred += fileTransfer->totalSize();
}

