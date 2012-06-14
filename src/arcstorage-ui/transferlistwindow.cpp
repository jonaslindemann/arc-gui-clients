#include "transferlistwindow.h"
#include "ui_transferlistwindow.h"

#include "arcstorage.h"
#include "filetransferlist.h"

#include <QDebug>

TransferListWindow::TransferListWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TransferListWindow)
{
    ui->setupUi(this);

    connect(FileTransferList::instance(), SIGNAL(onUpdateStatus(QString)), this, SLOT(onUpdateStatus(QString)));
    connect(FileTransferList::instance(), SIGNAL(onAddTransfer(QString)), this, SLOT(onAddTransfer(QString)));
    connect(FileTransferList::instance(), SIGNAL(onRemoveTransfer(QString)), this, SLOT(onRemoveTransfer(QString)));

    ui->transferTable->clear();

    QStringList labels;
    labels << "ID" << "Transfer" << "State" << "Status";
    ui->transferTable->setHorizontalHeaderLabels(labels);
    ui->transferTable->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);
    ui->transferTable->verticalHeader()->hide();
    ui->transferTable->setShowGrid(true);
    //ui->transferTable->setFrameStyle(QFrame::NoFrame);
}

TransferListWindow::~TransferListWindow()
{
    delete ui;
}

void TransferListWindow::onUpdateStatus(QString id)
{
    logger.msg(Arc::DEBUG, "onUpdateStatus(): "+id.toStdString());

    if (m_idToRowDict.contains(id))
    {
        m_accessLock.lock();

        FileTransfer* xfr = FileTransferList::instance()->getTransfer(id);

        int row = m_idToRowDict[id];

        unsigned long transferred, totalSize;
        xfr->getTransferStatus(transferred, totalSize);

        if (xfr->transferState()==TS_IDLE)
            ui->transferTable->item(row, 2)->setText("Idle");

        if (xfr->transferState()==TS_EXECUTED)
            ui->transferTable->item(row, 2)->setText("Running");

        if (xfr->transferState()==TS_FAILED)
            ui->transferTable->item(row, 2)->setText("Failed");

        if (xfr->transferState()==TS_COMPLETED)
            ui->transferTable->item(row, 2)->setText("Completed");

        QString status = QString::number(transferred) + "kB / " + QString::number(totalSize) + " kB";
        ui->transferTable->item(row, 3)->setText(status);

        m_accessLock.unlock();
    }
}

void TransferListWindow::onAddTransfer(QString id)
{
    logger.msg(Arc::DEBUG, "onAddTransfer(): "+id.toStdString());
    FileTransfer* xfr = FileTransferList::instance()->getTransfer(id);

    m_accessLock.lock();

    ui->transferTable->insertRow(ui->transferTable->rowCount());

    int row = ui->transferTable->rowCount()-1;
    m_idToRowDict[id] = row;

    unsigned long transferred, totalSize;
    xfr->getTransferStatus(transferred, totalSize);
    QString status = QString::number(transferred) + "kB / " + QString::number(totalSize) + " kB";
    QString transfer = xfr->sourceUrl() + " -> " + xfr->destUrl();

    ui->transferTable->setItem(row, 0, new QTableWidgetItem(id));
    ui->transferTable->setItem(row, 1, new QTableWidgetItem(transfer));
    ui->transferTable->setItem(row, 2, new QTableWidgetItem());
    ui->transferTable->setItem(row, 3, new QTableWidgetItem(status));

    if (xfr->transferState()==TS_IDLE)
        ui->transferTable->item(row, 2)->setText("Idle");

    if (xfr->transferState()==TS_EXECUTED)
        ui->transferTable->item(row, 2)->setText("Running");

    if (xfr->transferState()==TS_FAILED)
        ui->transferTable->item(row, 2)->setText("Failed");

    if (xfr->transferState()==TS_COMPLETED)
        ui->transferTable->item(row, 2)->setText("Completed");

    //ui->transferTable->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
    ui->transferTable->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);
    ui->transferTable->horizontalHeader()->setResizeMode(2, QHeaderView::ResizeToContents);

    m_accessLock.unlock();
}

void TransferListWindow::onRemoveTransfer(QString id)
{
    logger.msg(Arc::DEBUG, "onRemoveTransfer(): "+id.toStdString());

    if (m_idToRowDict.contains(id))
    {
        m_accessLock.lock();

        // Remove transfer from table

        int row = m_idToRowDict[id];
        ui->transferTable->removeRow(row);

        // Update id to row mapping

        m_idToRowDict.clear();

        for (int i=0; i<ui->transferTable->rowCount(); i++)
            m_idToRowDict[ui->transferTable->item(i,0)->text()] = i;

        m_accessLock.unlock();
    }
}

