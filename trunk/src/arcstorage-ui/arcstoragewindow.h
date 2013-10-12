#ifndef ArcStorageWindow_H
#define ArcStorageWindow_H

namespace Ui {
    class ArcStorageWindow;
}

#include "dragdropabletreewidget.h"

#include <QMainWindow>
#include <QTreeWidget>
#include <QModelIndex>
#include <arc/Logger.h>
#include <QComboBox>
#include <QStack>
#include <QProcess>
#include <QPushButton>

#include "qdebugstream.h"
#include "transferlistwindow.h"
#include "filetransferlist.h"
#include "arcfileserver.h"

#include "filepropertyinspector.h"



enum updateFileListsMode { CUFLM_noUpdate,
                           CUFLM_syncBoth,
                           CUFLM_clickedBrowse,
                           CUFLM_clickedUp,
                           CUFLM_expandedFolder,
                           CUFLM_clickedFolder,
                           CUFLM_doubleClickedFolder
                         };

class ArcStorageWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ArcStorageWindow(QWidget *parent = 0, bool childWindow = false, QString Url="");
    ~ArcStorageWindow();

    void onNewStatus(QString errorStr);

    void setBusyUI(bool busy);
    void deleteSelectedFiles();
    void createDir();
    QString getCurrentURL();
    QMenu* getWindowListMenu();

    void writeSettings();
    void readSettings();

    void openUrl(QString url);
    void setWindowId(int id);
    int getWindowId();

private:
    Ui::ArcStorageWindow *ui;
    DragDropableTreeWidget m_filesTreeWidget;
    QLineEdit m_urlEdit;
    QCompleter* m_urlCompleter;
    QPushButton m_urlCompleteButton;

    ArcFileServer *m_currentFileServer;
    QString m_folderListUrl;
    QString m_startUrl;

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

    QStringList m_breadCrumbItems;
    QStack<QString> m_backStack;
    QSet<QString> m_recent;

    void pushUrl(QString url);
    QString popUrl();

    Arc::LogStream* m_logStream;
    QDebugStream* m_debugStream;
    QDebugStream* m_debugStream2;

    bool m_childWindow;
    int m_windowId;

    TransferListWindow* m_transferWindow;
    FilePropertyInspector* m_filePropertyInspector;

    FileTransferProcessingThread* m_fileProcessingThread;

    QProcess* m_tarProcess;
    QString m_tarFilename;
    QString m_tarDestDir;

    QList<QUrl> m_filesToOpen;

protected:
    void showEvent(QShowEvent *e);
    void closeEvent( QCloseEvent *e );

private Q_SLOTS:
    void onURLEditReturnPressed();
    void onContextMenu(const QPoint& pos);
    void onUrlComboBoxCurrentIndexChanged(int index);
    void onUrlCompletePressed();
    void onEditTextChanged(const QString& text);
    void onBreadCrumbTriggered();

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

    void on_actionStop_triggered();

    void on_actionSettings_triggered();

    void on_filesTreeWidget_customContextMenuRequested(const QPoint &pos);

    void on_actionCopyURL_triggered();

    void on_filesTreeWidget_itemClicked(QTreeWidgetItem *item, int column);

    void on_actionCopyURLFilename_triggered();

    void on_actionShowFileProperties_triggered();

    void on_actionBack_triggered();

    void on_actionHelpContents_triggered();

    void on_actionDownloadSelected_triggered();

    void on_actionUploadSelected_triggered();

    void on_actionUploadDirectory_triggered();

    void on_actionUploadDirAndArchive_triggered();

    void onTarErrorOutput();
    void onTarStandardOutput();
    void onTarFinished(int exitCode, QProcess::ExitStatus exitStatus);

    void on_actionRename_triggered();

    void on_filesTreeWidget_itemChanged(QTreeWidgetItem *item, int column);

    void on_actionOpenURLExt_triggered();

public Q_SLOTS:
    void onFilesDroppedInFileListWidget(QList<QUrl> &urlList);
    void onFileListFinished(bool error, QString errorMsg);
    void onError(QString errorStr);
    void onCopyFromServerFinished(bool error);
    void onDeleteFinished(bool error);
    void onMakeDirFinished(bool error);
    void onCopyToServerFinished(bool error, QList<QString> &failedFiles);

};

#endif // ArcStorageWindow_H
