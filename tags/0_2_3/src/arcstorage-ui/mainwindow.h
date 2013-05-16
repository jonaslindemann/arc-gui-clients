#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "dragdropabletreewidget.h"

#include <QMainWindow>
#include <QTreeWidget>
#include <QModelIndex>
#include <arc/Logger.h>
#include <QComboBox>
#include "qdebugstream.h"
#include "transferlistwindow.h"
#include "filetransferlist.h"


class FileServer;

namespace Ui {
    class MainWindow;
}

enum updateFileListsMode { CUFLM_noUpdate,
                           CUFLM_clickedBrowse,
                           CUFLM_clickedUp,
                           CUFLM_expandedFolder,
                           CUFLM_clickedFolder,
                           CUFLM_doubleClickedFolder
                         };

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0, bool childWindow = false, QString Url="");
    ~MainWindow();

    void onNewStatus(QString errorStr);

    void setBusyUI(bool busy);
    void deleteSelectedFiles();
    void createDir();
    QString getCurrentURL();
    QMenu* getWindowListMenu();

private:
    Ui::MainWindow *ui;
    DragDropableTreeWidget m_filesTreeWidget;
    QComboBox m_urlComboBox;

    FileServer *m_currentFileServer;

    static const QString COPY_TO_TEXT;
    static const QString COPY_TEXT;
    static const QString DELETE_TEXT;
    static const QString CHANGE_OWNER_TEXT;
    static const QString MAKEDIR_TEXT;
    static const QString CHANGE_PERMISSIONS_TEXT;

    enum updateFileListsMode m_currentUpdateFileListsMode;
    QTreeWidgetItem *m_folderWidgetBeingUpdated;

    void updateFileTree();
    void updateFoldersTree();
    void updateFoldersTreeBelow();
    void expandFolderTreeWidget(QTreeWidgetItem *folderWidget);
    QString getURLOfItem(QTreeWidgetItem *item);
    void setURLOfItem(QTreeWidgetItem *item, QString URL);

    QString getCurrentComboBoxURL();
    void    setCurrentComboBoxURL(QString url);

    QStringList fileTreeHeaderLabels;

    Arc::LogStream* m_logStream;
    QDebugStream* m_debugStream;
    QDebugStream* m_debugStream2;

    bool m_childWindow;

    TransferListWindow* m_transferWindow;

    FileTransferProcessingThread* m_fileProcessingThread;

protected:
    void closeEvent( QCloseEvent *e );

private Q_SLOTS:
    void onURLEditReturnPressed();
    void onContextMenu(const QPoint& pos);
    void onUrlComboBoxCurrentIndexChanged(int index);

    void on_actionDelete_triggered();
    void on_actionQuit_triggered();
    void on_foldersTreeWidget_itemExpanded(QTreeWidgetItem* item);
    void on_foldersTreeWidget_clicked(QModelIndex index);
    void on_foldersTreeWidget_itemClicked(QTreeWidgetItem* item, int column);
    void on_foldersTreeWidget_expanded(QModelIndex index);
    void on_actionAbout_ARC_File_Navigator_triggered();
    void on_actionUp_triggered();
    void on_actionNewWindow_triggered();
    void on_filesTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_actionClearSelection_triggered();
    void on_actionSelectAllFiles_triggered();
    void on_actionCreateDir_triggered();
    void on_actionShowTransferList_triggered();
    void on_actionOpenNewLocation_triggered();
    void on_actionSRM_Preferences_triggered();
    void on_actionReload_triggered();

    void on_actionCreateProxyCert_triggered();

    void on_actionConvertCertificates_triggered();

    void on_actionJobManager_triggered();

    void on_actionJobSubmissionTool_triggered();

public Q_SLOTS:
    void onFilesDroppedInFileListWidget(QList<QUrl> &urlList);
    void onFileListFinished(bool error, QString errorMsg);
    void onError(QString errorStr);
    void onCopyFromServerFinished(bool error);
    void onDeleteFinished(bool error);
    void onMakeDirFinished(bool error);
    void onCopyToServerFinished(bool error, QList<QString> &failedFiles);

};

#endif // MAINWINDOW_H