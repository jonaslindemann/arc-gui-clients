#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "dragdropabletreewidget.h"

#include <QMainWindow>
#include <QTreeWidget>
#include <QModelIndex>
#include <arc/Logger.h>
#include <QComboBox>
#include "qdebugstream.h"


class FileServer;

namespace Ui {
    class MainWindow;
}

enum updateFileListsMode { CUFLM_noUpdate,
                           CUFLM_clickedBrowse,
                           CUFLM_clickedUp,
                           CUFLM_expandedFolder,
                           CUFLM_clickedFolder };

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void filesDroppedInFileListWidget(QList<QUrl> &urlList);

    void onFileListFinished(bool error, QString errorMsg);
    void onError(QString errorStr);
    void onNewStatus(QString errorStr);
    void onCopyFromServerFinished(bool error);
    void onDeleteFinished(bool error);
    void onMakeDirFinished(bool error);
    void onCopyToServerFinished(bool error, QList<QString> &failedFiles);
    void setBusyUI(bool busy);


private:
    Ui::MainWindow *ui;
    DragDropableTreeWidget filesTreeWidget;
    QComboBox urlComboBox;

    FileServer *currentFileServer;

    static const QString COPY_TO_TEXT;
    static const QString COPY_TEXT;
    static const QString DELETE_TEXT;
    static const QString CHANGE_OWNER_TEXT;
    static const QString MAKEDIR_TEXT;
    static const QString CHANGE_PERMISSIONS_TEXT;

    enum updateFileListsMode currentUpdateFileListsMode;
    QTreeWidgetItem *folderWidgetBeingUpdated;

    void updateFileTree();
    void updateFoldersTree();
    void expandFolderTreeWidget(QTreeWidgetItem *folderWidget);
    QString getURLOfItem(QTreeWidgetItem *item);
    void setURLOfItem(QTreeWidgetItem *item, QString URL);

    QString getCurrentComboBoxURL();
    void    setCurrentComboBoxURL(QString url);

    QStringList fileTreeHeaderLabels;

    Arc::LogStream* m_logStream;
    QDebugStream* m_debugStream;
    QDebugStream* m_debugStream2;

protected:
    void closeEvent( QCloseEvent *e );

private Q_SLOTS:
    void on_actionDelete_triggered();
    void on_actionQuit_triggered();
    void on_upButton_clicked();
    void on_foldersTreeWidget_itemExpanded(QTreeWidgetItem* item);
    void on_URLEdit_returnPressed();
    void on_foldersTreeWidget_clicked(QModelIndex index);
    void on_foldersTreeWidget_itemClicked(QTreeWidgetItem* item, int column);
    void on_foldersTreeWidget_expanded(QModelIndex index);
    void on_browseButton_clicked();
    void onContextMenu(const QPoint& pos);
    void onMenuItemSRMSettings();
    void on_actionAbout_ARC_File_Navigator_triggered();
    void on_actionUp_triggered();
    void on_urlComboBox_currentIndexChanged(int index);
};

#endif // MAINWINDOW_H
