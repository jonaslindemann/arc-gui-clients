#include "transferlistwindow.h"
#include "ui_transferlistwindow.h"

#include "arcstorage.h"
#include "filetransferlist.h"

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
    labels << "ID" << "Operation" << "Status";
    ui->transferTable->setHorizontalHeaderLabels(labels);
    ui->transferTable->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
    ui->transferTable->verticalHeader()->hide();
    ui->transferTable->setShowGrid(true);
    ui->transferTable->setFrameStyle(QFrame::NoFrame);
}

TransferListWindow::~TransferListWindow()
{
    delete ui;
}

void TransferListWindow::onUpdateStatus(QString id)
{
    logger.msg(Arc::INFO, "onUpdateStatus(): "+id.toStdString());
}

void TransferListWindow::onAddTransfer(QString id)
{
    logger.msg(Arc::INFO, "onAddTransfer(): "+id.toStdString());
}

void TransferListWindow::onRemoveTransfer(QString id)
{
    logger.msg(Arc::INFO, "onRemoveTransfer(): "+id.toStdString());
}

