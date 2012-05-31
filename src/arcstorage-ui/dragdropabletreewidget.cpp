#include "dragdropabletreewidget.h"
#include <iostream>
#include <QDragEnterEvent>
#include <QtGui>
#include "mainwindow.h"

DragDropableTreeWidget::DragDropableTreeWidget(QWidget *parent) : QTreeWidget(parent)
{
    mainWindow = NULL;

    setAcceptDrops(true);
}

DragDropableTreeWidget::~DragDropableTreeWidget()
{
}

void DragDropableTreeWidget::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void DragDropableTreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
}

void DragDropableTreeWidget::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    QList<QUrl> urlList = mimeData->urls();

    qDebug() << "dropEvent" << event->mimeData()->urls().count();
    mainWindow->onFilesDroppedInFileListWidget(urlList);
}


void DragDropableTreeWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
}

void DragDropableTreeWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        dragStartPos = event->pos();

    QTreeWidget::mousePressEvent(event);
}

void DragDropableTreeWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
    {
        int distance = (event->pos() - dragStartPos).manhattanLength();
        if (distance >= QApplication::startDragDistance())
        {
            QTreeWidgetItem *item = currentItem();
            if (item)
            {
                qDebug() << "Drag start.";

                mainWindow->setBusyUI(true);

                QMimeData *mimeData = new QMimeData;

                QVariant dataQV = item->data(0, Qt::ToolTipRole);
                QUrl url = QUrl::fromLocalFile(dataQV.toString());
                QList<QUrl> *urlList = new QList<QUrl>();
                urlList->append(url);
                mimeData->setUrls(*urlList);

                QDrag *drag = new QDrag(this);
                drag->setMimeData(mimeData);
                drag->setPixmap(QPixmap(":/images/person.png"));
                drag->start(Qt::MoveAction);

                mainWindow->setBusyUI(false);
            }
        }
    }
    QTreeWidget::mouseMoveEvent(event);
}
