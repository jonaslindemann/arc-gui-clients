#ifndef DRAGDROPABLETREEWIDGET_H
#define DRAGDROPABLETREEWIDGET_H

#include <QTreeWidget>
#include <QtGui>
#include <QList>

class MainWindow;

/** This class describes extends the standard QTreeWidget to provide both
  * drag and drop functionality. The dragging and dropping is done in
  * both directions: From the outside to this application and from this
  * application and to the outside
 */

class DragDropableTreeWidget : public QTreeWidget
{
public:
    DragDropableTreeWidget(QWidget *parent = 0);
    ~DragDropableTreeWidget();
    void setMainWindow(MainWindow *mw) { mainWindow = mw; }

    void resetSelection();

private:
    MainWindow *mainWindow;
    QPoint dragStartPos;
    QList<QTreeWidgetItem*> m_selectedItems;


protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent* event);
};

#endif // DRAGDROPABLETREEWIDGET_H
