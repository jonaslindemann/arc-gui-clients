#include "dragdropabletreewidget.h"
#include <iostream>
#include <QDragEnterEvent>
#include <QtGui>
#include <QDebug>

#include "arcstoragewindow.h"

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
    mainWindow->onFilesDroppedInFileListWidget(urlList);
}


void DragDropableTreeWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
}

void DragDropableTreeWidget::mousePressEvent(QMouseEvent *event)
{   
    QTreeWidget::mousePressEvent(event);

    if (event->button() == Qt::LeftButton)
    {
        // Remember selected items.

        m_selectedItems.clear();
        for (int i=0; i<this->selectedItems().length(); i++)
            m_selectedItems.append(this->selectedItems().at(i));
        dragStartPos = event->pos();
    }

}

void DragDropableTreeWidget::mouseMoveEvent(QMouseEvent *event)
{
    QTreeWidget::mouseMoveEvent(event);
    if (event->buttons() & Qt::LeftButton)
    {
        int distance = (event->pos() - dragStartPos).manhattanLength();
        if (distance >= QApplication::startDragDistance())
        {
            if (m_selectedItems.count()>0)
            {
                mainWindow->setBusyUI(true);

                QMimeData *mimeData = new QMimeData;
                QList<QUrl> *urlList = new QList<QUrl>();

                //item->setSelected(true);

                this->selectedItems().clear();
                for (int i=0; i<m_selectedItems.count(); i++)
                    m_selectedItems.at(i)->setSelected(true);

                for (int i=0; i<m_selectedItems.count(); i++)
                {
                    QTreeWidgetItem* item = m_selectedItems.at(i);
                    if (item->text(2)=="file")
                    {
                        QVariant dataQV = item->data(0, Qt::ToolTipRole);
                        QUrl url = QUrl::fromLocalFile(dataQV.toString());
                        urlList->append(url);
                    }
                    else
                    {
                        QVariant dataQV = item->data(0, Qt::ToolTipRole);
                        QUrl url = QUrl::fromLocalFile(dataQV.toString()+"/");
                        urlList->append(url);
                    }

                }
                mimeData->setUrls(*urlList);

                QDrag *drag = new QDrag(this);
                drag->setMimeData(mimeData);
                drag->setPixmap(QPixmap(":/images/person.png"));
                drag->start(Qt::MoveAction);

                mainWindow->setBusyUI(false);
            }
        }
    }
}

void DragDropableTreeWidget::mouseReleaseEvent(QMouseEvent* event)
{
    QTreeWidget::mouseReleaseEvent(event);
}

void DragDropableTreeWidget::resetSelection()
{
    for (int i=0; i<m_selectedItems.count(); i++)
        m_selectedItems.at(i)->setSelected(true);
    m_selectedItems.clear();
}
