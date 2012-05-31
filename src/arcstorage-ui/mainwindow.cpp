#include <QVector>
#include <QString>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMessageBox>
#include <QFileDialog>
#include <QVariant>
#include <QInputDialog>
#include <QSpacerItem>
#include <iostream>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "filelister.h"
#include "localfileserver.h"
#include "filepropertiesdialog.h"
#include "fileserverfactory.h"
#include "srmsettingsdialog.h"
#include "settings.h"
#include "qdebugstream.h"

#include "arcstorage.h"

#include <arc/Logger.h>


const QString MainWindow::COPY_TO_TEXT            = QString("Copy to...");
const QString MainWindow::COPY_TEXT               = QString("Copy");
const QString MainWindow::DELETE_TEXT             = QString("Delete");
const QString MainWindow::CHANGE_OWNER_TEXT       = QString("Change owner...");
const QString MainWindow::MAKEDIR_TEXT            = QString("Make dir...");
const QString MainWindow::CHANGE_PERMISSIONS_TEXT = QString("Change permissions...");


MainWindow::MainWindow(QWidget *parent, bool childWindow):
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    m_childWindow = childWindow;
    Settings::loadFromDisk();
    QVariant qvar = Settings::getValue("urlList");
    QList<QVariant> urlList = qvar.toList();

    m_folderWidgetBeingUpdated = NULL;
    m_currentUpdateFileListsMode = CUFLM_clickedBrowse;

    ui->setupUi(this);

    // Create and add the url combobox manually to the toolbar because QT Designer doesn't support it

    m_urlComboBox.setEditable(true);
    m_urlComboBox.setMaxVisibleItems(10);
    m_urlComboBox.setMaxCount(10);
    for (int i = 0; i < urlList.size(); ++i)
    {
        m_urlComboBox.addItem(urlList.at(i).toString());
    }
    m_urlComboBox.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_urlComboBox.repaint();
    ui->mainToolBar->addWidget(&m_urlComboBox);
    connect(m_urlComboBox.lineEdit(), SIGNAL(returnPressed()), this, SLOT(on_URLEdit_returnPressed()));  // When someone presses return in the url combobox...
    connect(&m_urlComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(on_m_urlComboBox_currentIndexChanged(int)));

    // Can't add empty space in toolbar, so we add a dummy widget instead.

    QWidget* w = new QWidget(ui->mainToolBar);
    w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    w->setMaximumSize(6,0);
    w->setMinimumSize(6,0);
    ui->mainToolBar->addWidget(w);

    ui->filesTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->filesTreeWidget, SIGNAL(customContextMenuRequested(const QPoint&)),  // Right click on files to show context menu
        this, SLOT(onContextMenu(const QPoint&)));
    connect(ui->actionSRM_Preferences, SIGNAL(triggered()), this, SLOT(onMenuItemSRMSettings()));
    connect(ui->actionUp, SIGNAL(triggered()), this, SLOT(on_upButton_clicked()));
    connect(ui->actionReload, SIGNAL(triggered()), this, SLOT(on_browseButton_clicked()));

    m_currentFileServer = FileServerFactory::getNewFileServer("", this);  // "" - default file server

    // Setup the headers in the file tree widget

    fileTreeHeaderLabels = m_currentFileServer->getFileInfoLabels();
    ui->filesTreeWidget->setColumnCount(fileTreeHeaderLabels.size());
    ui->filesTreeWidget->setHeaderLabels(fileTreeHeaderLabels);

    setBusyUI(true);
    qDebug() << "updateFileList called.";
    m_currentFileServer->updateFileList(QDir::homePath());
    setCurrentComboBoxURL(m_currentFileServer->getCurrentURL());

    ui->filesTreeWidget->setMainWindow(this);

    if (!m_childWindow) {
        m_debugStream = new QDebugStream(std::cout, ui->textOutput);
        m_debugStream2 = new QDebugStream(std::cerr, ui->textOutput);
    }

    ui->textOutput->clear();

    // Redirect ARC logging to std::cout

    if (!m_childWindow) {
        m_logStream = new Arc::LogStream(std::cout);
        m_logStream->setFormat(Arc::ShortFormat);
        Arc::Logger::getRootLogger().addDestination(*m_logStream);
        Arc::Logger::getRootLogger().setThreshold(Arc::INFO);
    }

    // Set splitter sizes

    ui->splitterHorisontal->setStretchFactor(1,1);
    ui->splitterVertical->setStretchFactor(0,1);
    ui->splitterVertical->setStretchFactor(1,0);

    if (m_childWindow)
        ui->textOutput->hide();

#ifdef __linux__
    ui->actionQuit->setIcon(QIcon::fromTheme("application-exit"));
    ui->actionDelete->setIcon(QIcon::fromTheme("edit-delete"));
    ui->actionForward->setIcon(QIcon::fromTheme("forward"));
    ui->actionBack->setIcon(QIcon::fromTheme("back"));
    ui->actionReload->setIcon(QIcon::fromTheme("reload"));
    ui->actionStop->setIcon(QIcon::fromTheme("stop"));
    ui->actionUp->setIcon(QIcon::fromTheme("up"));
    ui->actionSRM_Preferences->setIcon(QIcon::fromTheme("preferences-other"));
    ui->actionNewWindow->setIcon(QIcon::fromTheme("window-new"));
    ui->actionCopyTo->setIcon(QIcon::fromTheme("document-save"));
    ui->actionUploadFiles->setIcon(QIcon::fromTheme("document-send"));
#endif
}

MainWindow::~MainWindow()
{
    // Disconnect log streams

    Arc::Logger::getRootLogger().removeDestinations();
    if (!m_childWindow)
    {
        delete m_logStream;
        delete m_debugStream;
        delete m_debugStream2;
    }
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    QList<QVariant> urlList;
//    QComboBox *comboBox = ui->m_urlComboBox;
    for (int i = 0; i < m_urlComboBox.count(); ++i)
    {
        urlList << m_urlComboBox.itemText(i);
    }
    Settings::setValue("urlList", urlList);

    Settings::saveToDisk();

    QMainWindow::closeEvent(e);
}

void MainWindow::copySelectedFiles()
{
    QList<QTreeWidgetItem *> selectedItems = ui->filesTreeWidget->selectedItems();

    if (selectedItems.size() != 0)
    {
        QString destDir = QFileDialog::getExistingDirectory(this, "Select destination", "");

        if (destDir != NULL)
        {
            int i;

            for (i=0; i<selectedItems.length(); i++)
            {
                QTreeWidgetItem *selectedItem = selectedItems.at(i);
                QString sourcePath = getURLOfItem(selectedItem);

                QString destPath = destDir + "/"+ selectedItem->text(0);

                qDebug() << "Copy from " << sourcePath << " to " << destPath;

                setBusyUI(true);
                m_currentFileServer->copyFromServer(sourcePath, destPath);
            }
        }
    }
}

void MainWindow::deleteSelectedFiles()
{

}

QString MainWindow::getURLOfItem(QTreeWidgetItem *item)
{
    QVariant dataQV = item->data(0, Qt::ToolTipRole);
    return dataQV.toString();
}

void MainWindow::setURLOfItem(QTreeWidgetItem *item, QString URL)
{
    QVariant dataQV = URL;
    item->setData(0, Qt::ToolTipRole, dataQV);
}


void MainWindow::on_upButton_clicked()
{
    setBusyUI(true);
    m_currentUpdateFileListsMode = CUFLM_clickedUp;
    if (m_currentFileServer->goUpOneFolder() == true)
    {
        setCurrentComboBoxURL(m_currentFileServer->getCurrentURL());
    }
    else
    {
        //ui->statusLabel->setText("Canot go up any further. Top of the tree!");
        setBusyUI(false);
    }
}


void MainWindow::on_browseButton_clicked()
{
    QString url = getCurrentComboBoxURL();

    setBusyUI(true);
    m_currentUpdateFileListsMode = CUFLM_clickedBrowse;

    m_currentFileServer = FileServerFactory::getNewFileServer(url, this);

    // Setup the headers in the file tree widget (in case it's a new file server)
    fileTreeHeaderLabels = m_currentFileServer->getFileInfoLabels();
    ui->filesTreeWidget->setColumnCount(fileTreeHeaderLabels.size());
    ui->filesTreeWidget->setHeaderLabels(fileTreeHeaderLabels);

    while (url.endsWith('/')) { url = url.left(url.length() - 1); }  // Get rid of trailing /

    m_currentFileServer->updateFileList(url);

    setCurrentComboBoxURL(m_currentFileServer->getCurrentURL());
}


void MainWindow::on_foldersTreeWidget_expanded(QModelIndex index)
{
    logger.msg(Arc::INFO, "on_foldersTreeWidget_expanded() %d ", index.row());
}


void MainWindow::on_foldersTreeWidget_itemExpanded(QTreeWidgetItem* item)
{
    logger.msg(Arc::INFO, "on_foldersTreeWidget_itemExpanded() %s", item->text(0).toStdString());

    m_currentUpdateFileListsMode = CUFLM_expandedFolder;

    while (item->childCount() > 0)  // Clear all children before adding new ones
    {
        item->removeChild(item->child(0));
    }
    QString newURL = getURLOfItem(item);
    setBusyUI(true);
    m_folderWidgetBeingUpdated = item;
    m_currentFileServer->updateFileList(newURL);

//    ui->URLEdit->setText(m_currentFileServer->getCurrentURL());
}

void MainWindow::on_foldersTreeWidget_itemClicked(QTreeWidgetItem* item, int column)
{
    QString clickedString = item->text(0);
    logger.msg(Arc::INFO, "on_foldersTreeWidget_itemClicked() %s", clickedString.toStdString());
    setBusyUI(true);
    m_currentUpdateFileListsMode = CUFLM_clickedFolder;
    QString newURL = getURLOfItem(item);
    m_folderWidgetBeingUpdated = item;
    m_currentFileServer->updateFileList(newURL);
    setCurrentComboBoxURL(m_currentFileServer->getCurrentURL());
}


void MainWindow::on_foldersTreeWidget_clicked(QModelIndex index)
{
}


void MainWindow::on_URLEdit_returnPressed()
{
    on_browseButton_clicked();  // Pressing Return in URL should have same effect as clicking Browse button
}


void MainWindow::onFileListFinished(bool error, QString errorMsg)
{
    qDebug() << "onFileListFinished-->";
    setBusyUI(false);

    if (error == true)
    {
        qDebug() << "FileListFinished error.";
        m_currentUpdateFileListsMode = CUFLM_noUpdate;
        QMessageBox::information(this, tr("ArcFTP"), errorMsg);
    }
    else
    {
       switch (m_currentUpdateFileListsMode)
       {
       case CUFLM_noUpdate:
           qDebug() << "noUpdate";
           break;
       case CUFLM_clickedBrowse:
           qDebug() << "clickedBrowse";
           updateFoldersTree();
           updateFileTree();
           break;
       case CUFLM_clickedFolder:
           qDebug() << "clickedFolder";
           updateFileTree();
           break;
       case CUFLM_clickedUp:
           qDebug() << "clickedUp";
           updateFoldersTree();
           updateFileTree();
           break;
       case CUFLM_expandedFolder:
           qDebug() << "expandedFolder";
           expandFolderTreeWidget(m_folderWidgetBeingUpdated);
           m_folderWidgetBeingUpdated = NULL;
           break;
       default:
           qDebug() << "shouldn't happen.";
           break;
       }
    }

    m_currentUpdateFileListsMode = CUFLM_noUpdate;
    qDebug() << "onFileListFinished<--";
}

void MainWindow::onCopyFromServerFinished(bool error)
{
    qDebug() << "onCopyFromServerFinished";
    setBusyUI(false);
    if (error == true)
    {
        QMessageBox::information(this, tr("ArcFTP"), "An error occured while trying to copy the file");
    }
    else
    {
        //ui->statusLabel->setText("Copy complete!");
    }
}

void MainWindow::onDeleteFinished(bool error)
{
    m_currentUpdateFileListsMode = CUFLM_clickedFolder;   // Update the listview displaying the folder...
    onFileListFinished(false, "");                      // ... so that the deleted file is removed

    setBusyUI(false);
    if (error == true)
    {
        QMessageBox::information(this, tr("ArcFTP"), "Delete failed");
    }
    else
    {
        //ui->statusLabel->setText("File deleted!");
    }
}


void MainWindow::onMakeDirFinished(bool error)
{
    m_currentUpdateFileListsMode = CUFLM_clickedFolder;   // Update the listview displaying the folder...
    onFileListFinished(false, "");                      // ... so that the deleted file is removed

    setBusyUI(false);
    if (error == true)
    {
        QMessageBox::information(this, tr("ArcFTP"), "Makedir failed");
    }
    else
    {
        //ui->statusLabel->setText("Folder created!");
    }
}


void MainWindow::onCopyToServerFinished(bool error, QList<QString> &failedFiles)
{
    m_currentUpdateFileListsMode = CUFLM_clickedFolder;   // Update the listview displaying the folder...
    onFileListFinished(false, "");                      // ... so that the deleted file is removed

    setBusyUI(false);

    if (error == true)
    {
        QString message = "Copy files to server failed\nThe following file(s) failed:\n";
        for (int i = 0; i < failedFiles.size(); ++i)
        {
            message += failedFiles.at(i) + "\n";
        }

        QMessageBox::information(this, tr("ArcFTP"), message);
    }
    else
    {
        //ui->statusLabel->setText("Copy files to server OK!");
    }
}


void MainWindow::onError(QString errorStr)
{
    setBusyUI(false);

    QMessageBox::information(this, tr("ArcFTP"), errorStr);
}


void MainWindow::onNewStatus(QString statusStr)
{
    //ui->statusLabel->setText(statusStr);
}

void MainWindow::onContextMenu(const QPoint& pos)
{
    QPoint globalPos = ui->filesTreeWidget->mapToGlobal(pos);

    QList<QTreeWidgetItem *> selectedItems = ui->filesTreeWidget->selectedItems();

    if (selectedItems.size() == 0)
    {
        // Nothing selected
        // Put commands here that execute when nothing was selected
        QMenu myMenu;
        myMenu.addAction(MAKEDIR_TEXT);
        QAction* selectedItem = myMenu.exec(globalPos);
        if (selectedItem != NULL)
        {
            if (selectedItem->text() == MAKEDIR_TEXT)
            {
                bool ok;
                QString path = QInputDialog::getText(this, tr("ArcFTP"),
                                                     tr("Folder name:"),
                                                     QLineEdit::Normal,
                                                     "New Folder",
                                                     &ok);
                if (ok && path.isEmpty() != true)
                {
                    m_currentFileServer->makeDir(path);
                }

            }
        }
    }
    else
    {
        QTreeWidgetItem *selectedItem = selectedItems.at(0);  // Only one item selected hopefully  //ALEX

        QMenu myMenu;
        myMenu.addAction(COPY_TEXT);
        myMenu.addAction(DELETE_TEXT);
        myMenu.addAction(COPY_TO_TEXT);
        myMenu.addAction(CHANGE_OWNER_TEXT);
        myMenu.addAction(CHANGE_PERMISSIONS_TEXT);

        QAction* selectedMenuItem = myMenu.exec(globalPos);
        if (selectedMenuItem != NULL)
        {
            if (selectedMenuItem->text() == COPY_TEXT)
            {
            }
            else if (selectedMenuItem->text() == COPY_TO_TEXT)
            {
                QString destination = QFileDialog::getSaveFileName(this,
                                                                   tr("Select destination"),
                                                                   selectedItem->text(0),
                                                                   "");
                if (destination != NULL)
                {
                    QString sourcePath = getURLOfItem(selectedItem);
                    setBusyUI(true);
                    m_currentFileServer->copyFromServer(sourcePath, destination);
                }
            }
            else if (selectedMenuItem->text() == DELETE_TEXT)
            {
                QString path = getURLOfItem(selectedItem);

                // Make sure the user wants to delete the file.

                QMessageBox::StandardButton reply;
                reply = QMessageBox::critical(this, tr("Delete file"),
                                                "Are you sure you want to delete, "+path+" ?",
                                                QMessageBox::Yes | QMessageBox::No);
                if (reply == QMessageBox::Yes)
                {
                    setBusyUI(true);
                    m_currentFileServer->deleteItem(path);
                }
            }
            else if (selectedMenuItem->text() == CHANGE_OWNER_TEXT)
            {
            }
            else if (selectedMenuItem->text() == CHANGE_PERMISSIONS_TEXT)
            {
                // This needs to be changed to using callbacks
                QString path = getURLOfItem(selectedItem);
                unsigned int filePermissions = m_currentFileServer->getFilePermissions(path);
                FilePermissionsDialog dialog(this);
                dialog.setPermissions(filePermissions);
                dialog.setModal(true);
                int dialogReturnValue = dialog.exec();
                if (dialogReturnValue != 0)
                {
                    filePermissions = dialog.getPermissions();
                    m_currentFileServer->setFilePermissions(path, filePermissions);
                }
            }
            else
            {
                // Unchecked menu item!?
            }
        }
    }
}


void MainWindow::setBusyUI(bool busy)
{
    if (busy == true)
    {
        qDebug() << "Disable UI.";
        setCursor(Qt::WaitCursor);
        ui->filesTreeWidget->setEnabled(false);
        m_urlComboBox.setEnabled(false);
        ui->foldersTreeWidget->setEnabled(false);
    }
    else
    {
        qDebug() << "Enable UI.";
        setCursor(Qt::ArrowCursor);
        ui->filesTreeWidget->setEnabled(true);
        m_urlComboBox.setEnabled(true);
        ui->foldersTreeWidget->setEnabled(true);
    }
}


void MainWindow::updateFileTree()
{
    QVector<ARCFileElement> fileList = m_currentFileServer->getFileList();

    ui->filesTreeWidget->clear();
    ui->filesTreeWidget->setSortingEnabled(false);

    for (int i = 0; i < fileList.size(); ++i)
    {
        ARCFileElement AFE = fileList.at(i);
        QTreeWidgetItem *item = new QTreeWidgetItem;
        item->setText(0, AFE.getFileName());
        if (AFE.getFileType()==ARCDir)
        {
            item->setIcon(0,QIcon::fromTheme("folder"));
            item->setText(1, "---");
        }
        else
        {
            item->setIcon(0,QIcon::fromTheme("document"));
            item->setText(1, QString::number(AFE.getSize()));
        }
        if (AFE.getFileType()==ARCDir)
            item->setText(2, "folder");
        else
            item->setText(2, "file");
        item->setText(3, AFE.getLastModfied().toString());
        item->setText(4, AFE.getOwner());
        item->setText(5, AFE.getGroup());
        QString tmpStr;
        tmpStr.sprintf("%x", AFE.getPermissions());
        item->setText(6, tmpStr);
        item->setText(7, AFE.getLastRead().toString());
        setURLOfItem(item, AFE.getFilePath());
        ui->filesTreeWidget->addTopLevelItem(item);
    }

    ui->filesTreeWidget->setSortingEnabled(true);
    ui->filesTreeWidget->sortByColumn(2, Qt::DescendingOrder);
    ui->filesTreeWidget->header()->setResizeMode(QHeaderView::ResizeToContents);

    for (int i=0; i<8; i++)
        ui->filesTreeWidget->resizeColumnToContents(i);
}


void MainWindow::updateFoldersTree()
{
    qDebug() << "Update folders tree-->";
    QVector<ARCFileElement> fileList = m_currentFileServer->getFileList();

    ui->foldersTreeWidget->clear();

    for (int i = 0; i < fileList.size(); ++i)
    {
        ARCFileElement AFE = fileList.at(i);
        if (AFE.getFileType() == ARCDir)  // If this item in the file list is a folder...
        {
            QTreeWidgetItem *item = new QTreeWidgetItem;
            item->setText(0, AFE.getFileName());
            item->setIcon(0, QIcon::fromTheme("folder"));
            setURLOfItem(item, AFE.getFilePath());
            // Create dummy child item so that the folder can be expanded, removed when folder is expanded
            QTreeWidgetItem *dummyItem = new QTreeWidgetItem;
            item->addChild(dummyItem);
            ui->foldersTreeWidget->addTopLevelItem(item);
        }
    }
    qDebug() << "Update folders tree<--";
}


void MainWindow::expandFolderTreeWidget(QTreeWidgetItem *folderWidget)
{
    QVector<ARCFileElement> fileList = m_currentFileServer->getFileList();

    for (int i = 0; i < fileList.size(); ++i)
    {
        ARCFileElement AFE = fileList.at(i);
        if (AFE.getFileType() == ARCDir)  // If this item in the file list is a folder...
        {
            QTreeWidgetItem *item = new QTreeWidgetItem;
            item->setText(0, AFE.getFileName());
            item->setIcon(0, QIcon::fromTheme("folder"));
            setURLOfItem(item, AFE.getFilePath());
            // Create dummy child item so that the folder can be expanded, removed when folder is expanded
            QTreeWidgetItem *dummyItem = new QTreeWidgetItem;
            item->addChild(dummyItem);
            folderWidget->addChild(item);
        }
    }
}


void MainWindow::onFilesDroppedInFileListWidget(QList<QUrl>& urlList)
{
    this->setBusyUI(true);
    qDebug() << "filesDropped:";
    qDebug() << "urlList length = " << urlList.length();

    int i;

    for (i=0; i<urlList.length(); i++)
    {
        qDebug() << "file: " << urlList[i];
    }

    m_currentFileServer->copyToServer(urlList, m_currentFileServer->getCurrentPath());

    this->setBusyUI(false);
}


QString MainWindow::getCurrentComboBoxURL()
{
    QString url = "";

//    QLineEdit *le = ui->m_urlComboBox->lineEdit();
    QLineEdit *le = m_urlComboBox.lineEdit();
    if (le != NULL)
    {
        url = le->text();
    }

    return url;
}

void MainWindow::setCurrentComboBoxURL(QString url)
{
//    QLineEdit *le = ui->m_urlComboBox->lineEdit();
    QLineEdit *le = m_urlComboBox.lineEdit();
    if (le != NULL)
    {
        le->setText(url);
    }
}


void MainWindow::onMenuItemSRMSettings()
{
    SRMSettingsDialog dialog(this);
    dialog.setConfigFilename(Settings::getStringValue("srmConfigFilename"));
    dialog.setModal(true);
    int dialogReturnValue = dialog.exec();
    if (dialogReturnValue != 0)
    {
        Settings::setValue("srmConfigFilename", dialog.getConfigFilename());
    }

}

void MainWindow::on_actionAbout_ARC_File_Navigator_triggered()
{
    QMessageBox msgBox;
    msgBox.setText("ARC Storage Explorer\n\nCopyright (C) 2011 Lunarc, Lund University\n\nDeveloped by\n\nUser interface - Alexander Lapajne\nARC library interface - Jonas Lindemann");
    msgBox.exec();
}

void MainWindow::on_actionUp_triggered()
{

}

void MainWindow::on_actionQuit_triggered()
{
    this->close();
}

void MainWindow::on_actionDelete_triggered()
{
    // Delete selected file

    QList<QTreeWidgetItem *> selectedItems = ui->filesTreeWidget->selectedItems();
    if (selectedItems.size() != 0)
    {
        QTreeWidgetItem *selectedItem = selectedItems.at(0);  // Only one item selected hopefully  //ALEX
        QString path = getURLOfItem(selectedItem);

        // Make sure the user wants to delete the file.

        QMessageBox::StandardButton reply;
        reply = QMessageBox::critical(this, tr("Delete file"),
                                        "Are you sure you want to delete, "+path+" ?",
                                        QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes)
        {
            setBusyUI(true);
            m_currentFileServer->deleteItem(path);
        }
    }
}

void MainWindow::on_urlComboBox_currentIndexChanged(int index)
{
    logger.msg(Arc::DEBUG, "textEditChanged.");
}


void MainWindow::on_actionNewWindow_triggered()
{
    MainWindow* window = new MainWindow(0, true);
    QRect r = this->geometry();
    r.setLeft(this->geometry().left()+150);
    r.setTop(this->geometry().top()+150);
    r.setRight(this->geometry().right()+150);
    r.setBottom(this->geometry().bottom()+150);

    qDebug() << r;
    window->setGeometry(r);
    window->show();
}

void MainWindow::on_actionUploadFiles_triggered()
{

}

void MainWindow::on_actionCopyTo_triggered()
{
    this->copySelectedFiles();
}
