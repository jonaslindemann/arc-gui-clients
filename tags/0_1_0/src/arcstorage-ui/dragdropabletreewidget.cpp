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
    std::cout << "DragDropableTreeWidget::dragEnterEvent()" << std::endl;

    event->acceptProposedAction();
/*    emit changed(event->mimeData()); */
}

void DragDropableTreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
    std::cout << "DragDropableTreeWidget::dragMoveEvent()" << std::endl;
/*    event->acceptProposedAction(); */
}

void DragDropableTreeWidget::dropEvent(QDropEvent *event)
{
    std::cout << "DragDropableTreeWidget::dropEvent()" << std::endl;
    const QMimeData *mimeData = event->mimeData();

    QList<QUrl> urlList = mimeData->urls();

    mainWindow->filesDroppedInFileListWidget(urlList);

    std::cout << "DragDropableTreeWidget::dropEvent(): mimeData url[0] " << urlList.at(0).path().toStdString() << std::endl;

/*  event->acceptProposedAction();*/
}


void DragDropableTreeWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
    std::cout << "DragDropableTreeWidget::dragLeaveEvent()" << std::endl;
/*    event->accept(); */
}

void DragDropableTreeWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        dragStartPos = event->pos();
    }

    QTreeWidget::mousePressEvent(event);
}

void DragDropableTreeWidget::mouseMoveEvent(QMouseEvent *event)
{
std::cout << " DragDropableTreeWidget::mouseMoveEvent()" << std::endl;
    if (event->buttons() & Qt::LeftButton)
    {
        int distance = (event->pos() - dragStartPos).manhattanLength();
        if (distance >= QApplication::startDragDistance())
        {
            QTreeWidgetItem *item = currentItem();
            if (item)
            {
		std::cout << " DragDropableTreeWidget::mouseMoveEvent() - DRAG!" << std::endl;

                mainWindow->setBusyUI(true);

                QVariant dataQV = item->data(0, Qt::ToolTipRole);
                QString filepath = dataQV.toString();

                QByteArray data;

                QFile file(filepath);
                file.open(QIODevice::ReadOnly);
                data = file.readAll();
                file.close();

                QFileInfo fi(filepath);
                QString filename = fi.fileName();

                QMimeData *mimeData = new QMimeData;
//                mimeData->setText(item->text(0));
                QString mimeType = "application/octet-stream";
                mimeData->setData(mimeType, data);

                std::cout << "mimeType " + mimeType.toStdString() + "   filename " + filename.toStdString() << std::endl;

//                QByteArray fileNameData(filename.toAscii());
//                mimeData->setData("File Name", fileNameData);

//                QUrl url = QUrl::fromLocalFile(filename);
//                QUrl url("test.txt");
//                QList<QUrl> *urlList = new QList<QUrl>();
//                urlList->append(url);
//                mimeData->setUrls(*urlList);

                // mimeData->setData("File name", "test.txt");

                QDrag *drag = new QDrag(this);
                drag->setMimeData(mimeData);
                drag->setPixmap(QPixmap(":/images/person.png"));
                if (drag->start(Qt::MoveAction) == Qt::MoveAction)
                {
                    delete item;
                }

                mainWindow->setBusyUI(false);
            }
        }
    }
    QTreeWidget::mouseMoveEvent(event);
}
