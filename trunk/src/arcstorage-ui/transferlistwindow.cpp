#include "transferlistwindow.h"
#include "ui_transferlistwindow.h"

#include "arcstorage.h"
#include "filetransferlist.h"

#include <QDebug>
#include <QPushButton>
#include <QPainter>

TransferStatusDisplay::TransferStatusDisplay(QWidget *parent)
    :QWidget(parent)
{
    m_totalSize = 1000;
    m_transferred = 0;
}

void TransferStatusDisplay::setStatus(unsigned long transferred, unsigned long totalSize)
{
    m_totalSize = totalSize;
    m_transferred = transferred;
}

void TransferStatusDisplay::paintEvent(QPaintEvent *event)
{
    QPainter p(this);

    p.setBrush(Qt::red);

    // | ----W---- | ---R--- |
    // 0           x1        x2

    int x1 = int((double)(m_transferred/(double)m_totalSize) * (double)this->width()-1);
    int x2 = int(1.0 * (double)this->width()-1);

    p.setFont(QFont("Arial",8));
    p.setBrush(Qt::green);
    p.drawRect(0, 0, x1, this->height()-1);
    p.setBrush(Qt::gray);
    p.drawRect(x1, 0, x2-x1, this->height()-1);

    QString status = QString::number(m_transferred) + "kB/" + QString::number(m_totalSize) + "kB";

    p.drawText(8,3, this->width()-4, this->height()-4, Qt::AlignHCenter|Qt::AlignVCenter, status);

    p.end();
}

void TransferStatusDisplay::resizeEvent(QResizeEvent *event)
{

}

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
    labels << "ID" << "Transfer" << "State" << "Status" << "Action";
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

        ui->transferTable->cellWidget(row, 4)->setEnabled(false);

        TransferStatusDisplay* xfrDisplay = (TransferStatusDisplay*)ui->transferTable->cellWidget(row, 3);
        xfrDisplay->setStatus(transferred, totalSize);

        if (xfr->transferState()==TS_IDLE)
        {
            ui->transferTable->item(row, 2)->setText("Idle");
            ui->transferTable->cellWidget(row, 4)->setEnabled(true);
        }

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

void TransferListWindow::onCancelButtonClick()
{
    QPushButton* button = (QPushButton*)sender();
    QString id = button->objectName();
    FileTransfer* xfr = FileTransferList::instance()->getTransfer(id);
    FileTransferList::instance()->removeTransfer(xfr);
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

    // Status display

    TransferStatusDisplay* xfrDisplay = new TransferStatusDisplay();
    ui->transferTable->setCellWidget(row, 3, xfrDisplay);

    ui->transferTable->setItem(row, 0, new QTableWidgetItem(id));
    ui->transferTable->setItem(row, 1, new QTableWidgetItem(transfer));
    ui->transferTable->setItem(row, 2, new QTableWidgetItem());
    ui->transferTable->setItem(row, 3, new QTableWidgetItem(status));

    QPushButton* button = new QPushButton();
    button->setFlat(true);
    button->setText("");
    button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    button->setIcon(QIcon::fromTheme("stop"));
    button->setObjectName(id);
    connect(button, SIGNAL(clicked(void)), this, SLOT(onCancelButtonClick(void)));

    ui->transferTable->setCellWidget(row, 4, button);
    button->setEnabled(false);

    if (xfr->transferState()==TS_IDLE)
    {
        ui->transferTable->item(row, 2)->setText("Idle");
        button->setEnabled(true);
    }

    if (xfr->transferState()==TS_EXECUTED)
        ui->transferTable->item(row, 2)->setText("Running");

    if (xfr->transferState()==TS_FAILED)
        ui->transferTable->item(row, 2)->setText("Failed");

    if (xfr->transferState()==TS_COMPLETED)
        ui->transferTable->item(row, 2)->setText("Completed");

    //ui->transferTable->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
    ui->transferTable->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);
    ui->transferTable->horizontalHeader()->setResizeMode(2, QHeaderView::ResizeToContents);
    ui->transferTable->horizontalHeader()->setResizeMode(4, QHeaderView::ResizeToContents);

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

