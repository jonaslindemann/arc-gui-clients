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


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    Settings::loadFromDisk();
    QVariant qvar = Settings::getValue("urlList");
    QList<QVariant> urlList = qvar.toList();

    folderWidgetBeingUpdated = NULL;
    currentUpdateFileListsMode = CUFLM_clickedBrowse;

    ui->setupUi(this);

    // Create and add the url combobox manually to the toolbar because QT Designer doesn't support it
    urlComboBox.setEditable(true);
    urlComboBox.setMaxVisibleItems(10);
    urlComboBox.setMaxCount(10);
    for (int i = 0; i < urlList.size(); ++i)
    {
        urlComboBox.addItem(urlList.at(i).toString());
    }
    urlComboBox.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    urlComboBox.repaint();
    ui->mainToolBar->addWidget(&urlComboBox);
    connect(urlComboBox.lineEdit(), SIGNAL(returnPressed()), this, SLOT(on_URLEdit_returnPressed()));  // When someone presses return in the url combobox...
    connect(&urlComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(on_urlComboBox_currentIndexChanged(int)));

    // Can't add empty space in toolbar, so we add a dummy widget instead.

    QWidget* w = new QWidget(ui->mainToolBar);
    w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    w->setMaximumSize(6,0);
    w->setMinimumSize(6,0);
    ui->mainToolBar->addWidget(w);

//    ui->middleHorizontalLayout->addWidget(&filesTreeWidget);

    ui->filesTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->filesTreeWidget, SIGNAL(customContextMenuRequested(const QPoint&)),  // Right click on files to show context menu
        this, SLOT(onContextMenu(const QPoint&)));

//    connect(ui->urlComboBox->lineEdit(), SIGNAL(returnPressed()), this, SLOT(on_URLEdit_returnPressed()));  // Return pressed in URL combobox

    connect(ui->actionSRM_Preferences, SIGNAL(triggered()), this, SLOT(onMenuItemSRMSettings()));

    connect(ui->actionUp, SIGNAL(triggered()), this, SLOT(on_upButton_clicked()));
    connect(ui->actionReload, SIGNAL(triggered()), this, SLOT(on_browseButton_clicked()));

    currentFileServer = FileServerFactory::getFileServer("", this);  // "" - default file server

    // Setup the headers in the file tree widget
    fileTreeHeaderLabels = currentFileServer->getFileInfoLabels();
    ui->filesTreeWidget->setColumnCount(fileTreeHeaderLabels.size());
    ui->filesTreeWidget->setHeaderLabels(fileTreeHeaderLabels);

    setBusyUI(true);
    currentFileServer->updateFileList("");
    setCurrentComboBoxURL(currentFileServer->getCurrentURL());

    ui->filesTreeWidget->setMainWindow(this);

    m_debugStream = new QDebugStream(std::cout, ui->textOutput);
    m_debugStream2 = new QDebugStream(std::cerr, ui->textOutput);

    ui->textOutput->clear();

    // Redirect ARC logging to std::cout

    m_logStream = new Arc::LogStream(std::cout);
    m_logStream->setFormat(Arc::ShortFormat);
    Arc::Logger::getRootLogger().addDestination(*m_logStream);
    Arc::Logger::getRootLogger().setThreshold(Arc::INFO);

    // Set splitter sizes

    ui->splitterHorisontal->setStretchFactor(1,1);
    ui->splitterVertical->setStretchFactor(0,1);
    ui->splitterVertical->setStretchFactor(1,0);

#ifdef __linux__
    ui->actionQuit->setIcon(QIcon::fromTheme("application-exit"));
    ui->actionDelete->setIcon(QIcon::fromTheme("edit-delete"));
    ui->actionForward->setIcon(QIcon::fromTheme("forward"));
    ui->actionBack->setIcon(QIcon::fromTheme("back"));
    ui->actionReload->setIcon(QIcon::fromTheme("reload"));
    ui->actionStop->setIcon(QIcon::fromTheme("stop"));
    ui->actionUp->setIcon(QIcon::fromTheme("up"));
    ui->actionSRM_Preferences->setIcon(QIcon::fromTheme("preferences-other"));
#endif
}

MainWindow::~MainWindow()
{
    // Disconnect log streams

    Arc::Logger::getRootLogger().removeDestinations();
    delete m_logStream;
    delete m_debugStream;
    delete m_debugStream2;
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    QList<QVariant> urlList;
//    QComboBox *comboBox = ui->urlComboBox;
    for (int i = 0; i < urlComboBox.count(); ++i)
    {
        urlList << urlComboBox.itemText(i);
    }
    Settings::setValue("urlList", urlList);

    Settings::saveToDisk();

    QMainWindow::closeEvent(e);
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
    currentUpdateFileListsMode = CUFLM_clickedUp;
    if (currentFileServer->goUpOneFolder() == true)
    {
        setCurrentComboBoxURL(currentFileServer->getCurrentURL());
    }
    else
    {
        ui->statusLabel->setText("Canot go up any further. Top of the tree!");
        setBusyUI(false);
    }
}


void MainWindow::on_browseButton_clicked()
{
    QString url = getCurrentComboBoxURL();

    setBusyUI(true);
    currentUpdateFileListsMode = CUFLM_clickedBrowse;

    currentFileServer = FileServerFactory::getFileServer(url, this);

    // Setup the headers in the file tree widget (in case it's a new file server)
    fileTreeHeaderLabels = currentFileServer->getFileInfoLabels();
    ui->filesTreeWidget->setColumnCount(fileTreeHeaderLabels.size());
    ui->filesTreeWidget->setHeaderLabels(fileTreeHeaderLabels);

    while (url.endsWith('/')) { url = url.left(url.length() - 1); }  // Get rid of trailing /

    currentFileServer->updateFileList(url);

    setCurrentComboBoxURL(currentFileServer->getCurrentURL());
}


void MainWindow::on_foldersTreeWidget_expanded(QModelIndex index)
{
    logger.msg(Arc::INFO, "on_foldersTreeWidget_expanded() %d ", index.row());
}


void MainWindow::on_foldersTreeWidget_itemExpanded(QTreeWidgetItem* item)
{
    logger.msg(Arc::INFO, "on_foldersTreeWidget_itemExpanded() %s", item->text(0).toStdString());

    currentUpdateFileListsMode = CUFLM_expandedFolder;

    while (item->childCount() > 0)  // Clear all children before adding new ones
    {
        item->removeChild(item->child(0));
    }
    QString newURL = getURLOfItem(item);
    setBusyUI(true);
    folderWidgetBeingUpdated = item;
    currentFileServer->updateFileList(newURL);

//    ui->URLEdit->setText(currentFileServer->getCurrentURL());
}

void MainWindow::on_foldersTreeWidget_itemClicked(QTreeWidgetItem* item, int column)
{
    QString clickedString = item->text(0);
    logger.msg(Arc::INFO, "on_foldersTreeWidget_itemClicked() %s", clickedString.toStdString());
    setBusyUI(true);
    currentUpdateFileListsMode = CUFLM_clickedFolder;
    QString newURL = getURLOfItem(item);
    folderWidgetBeingUpdated = item;
    currentFileServer->updateFileList(newURL);
    setCurrentComboBoxURL(currentFileServer->getCurrentURL());
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
    setBusyUI(false);

    if (error == true)
    {
        currentUpdateFileListsMode = CUFLM_noUpdate;
        QMessageBox::information(this, tr("ArcFTP"), errorMsg);
    }
    else
    {
       switch (currentUpdateFileListsMode)
       {
       case CUFLM_noUpdate:
           break;
       case CUFLM_clickedBrowse:
           updateFoldersTree();
           updateFileTree();
           break;
       case CUFLM_clickedFolder:
           updateFileTree();
           break;
       case CUFLM_clickedUp:
           updateFoldersTree();
           updateFileTree();
           break;
       case CUFLM_expandedFolder:
           expandFolderTreeWidget(folderWidgetBeingUpdated);
           folderWidgetBeingUpdated = NULL;
           break;
       }
    }

    currentUpdateFileListsMode = CUFLM_noUpdate;
}

void MainWindow::onCopyFromServerFinished(bool error)
{
    setBusyUI(false);
    if (error == true)
    {
        QMessageBox::information(this, tr("ArcFTP"), "An error occured while trying to copy the file");
    }
    else
    {
        ui->statusLabel->setText("Copy complete!");
    }
}

void MainWindow::onDeleteFinished(bool error)
{
    currentUpdateFileListsMode = CUFLM_clickedFolder;   // Update the listview displaying the folder...
    onFileListFinished(false, "");                      // ... so that the deleted file is removed

    setBusyUI(false);
    if (error == true)
    {
        QMessageBox::information(this, tr("ArcFTP"), "Delete failed");
    }
    else
    {
        ui->statusLabel->setText("File deleted!");
    }
}


void MainWindow::onMakeDirFinished(bool error)
{
    currentUpdateFileListsMode = CUFLM_clickedFolder;   // Update the listview displaying the folder...
    onFileListFinished(false, "");                      // ... so that the deleted file is removed

    setBusyUI(false);
    if (error == true)
    {
        QMessageBox::information(this, tr("ArcFTP"), "Makedir failed");
    }
    else
    {
        ui->statusLabel->setText("Folder created!");
    }
}


void MainWindow::onCopyToServerFinished(bool error, QList<QString> &failedFiles)
{
    currentUpdateFileListsMode = CUFLM_clickedFolder;   // Update the listview displaying the folder...
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
        ui->statusLabel->setText("Copy files to server OK!");
    }
}


void MainWindow::onError(QString errorStr)
{
    setBusyUI(false);

    QMessageBox::information(this, tr("ArcFTP"), errorStr);
}


void MainWindow::onNewStatus(QString statusStr)
{
    ui->statusLabel->setText(statusStr);
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
                    currentFileServer->makeDir(path);
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
                    currentFileServer->copyFromServer(sourcePath, destination);
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
                    currentFileServer->deleteItem(path);
                }
            }
            else if (selectedMenuItem->text() == CHANGE_OWNER_TEXT)
            {
            }
            else if (selectedMenuItem->text() == CHANGE_PERMISSIONS_TEXT)
            {
                // This needs to be changed to using callbacks
                QString path = getURLOfItem(selectedItem);
                unsigned int filePermissions = currentFileServer->getFilePermissions(path);
                FilePermissionsDialog dialog(this);
                dialog.setPermissions(filePermissions);
                dialog.setModal(true);
                int dialogReturnValue = dialog.exec();
                if (dialogReturnValue != 0)
                {
                    filePermissions = dialog.getPermissions();
                    currentFileServer->setFilePermissions(path, filePermissions);
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
        setCursor(Qt::WaitCursor);
        ui->filesTreeWidget->setEnabled(false);
//        ui->urlComboBox->setEnabled(false);
        urlComboBox.setEnabled(false);
        ui->foldersTreeWidget->setEnabled(false);
//        ui->browseButton->setEnabled(false);
//        ui->upButton->setEnabled(false);
    }
    else
    {
        setCursor(Qt::ArrowCursor);
        ui->filesTreeWidget->setEnabled(true);
//        ui->urlComboBox->setEnabled(true);
        urlComboBox.setEnabled(true);
        ui->foldersTreeWidget->setEnabled(true);
//        ui->browseButton->setEnabled(true);
//        ui->upButton->setEnabled(true);
    }
}


void MainWindow::updateFileTree()
{
    QVector<ARCFileElement> fileList = currentFileServer->getFileList();

    ui->filesTreeWidget->clear();

    for (int i = 0; i < fileList.size(); ++i)
    {
        ARCFileElement AFE = fileList.at(i);
        QTreeWidgetItem *item = new QTreeWidgetItem;
//        DraggableQTreeWidget *item = new DraggableQTreeWidget;
        item->setText(0, AFE.getFileName());
        item->setText(1, QString::number(AFE.getSize()));
        item->setText(2, AFE.getOwner());
        item->setText(3, AFE.getGroup());
        QString tmpStr;
        tmpStr.sprintf("%x", AFE.getPermissions());
        item->setText(4, tmpStr);
        item->setText(5, AFE.getLastRead().toString());
        item->setText(6, AFE.getLastModfied().toString());
        setURLOfItem(item, AFE.getFilePath());
        ui->filesTreeWidget->addTopLevelItem(item);
    }
}


void MainWindow::updateFoldersTree()
{
    QVector<ARCFileElement> fileList = currentFileServer->getFileList();

    ui->foldersTreeWidget->clear();

    for (int i = 0; i < fileList.size(); ++i)
    {
        ARCFileElement AFE = fileList.at(i);
        if (AFE.getFileType() == ARCDir)  // If this item in the file list is a folder...
        {
            QTreeWidgetItem *item = new QTreeWidgetItem;
            item->setText(0, AFE.getFileName());
            setURLOfItem(item, AFE.getFilePath());
            // Create dummy child item so that the folder can be expanded, removed when folder is expanded
            QTreeWidgetItem *dummyItem = new QTreeWidgetItem;
            item->addChild(dummyItem);
            ui->foldersTreeWidget->addTopLevelItem(item);
        }
    }
}


void MainWindow::expandFolderTreeWidget(QTreeWidgetItem *folderWidget)
{
    QVector<ARCFileElement> fileList = currentFileServer->getFileList();

    for (int i = 0; i < fileList.size(); ++i)
    {
        ARCFileElement AFE = fileList.at(i);
        if (AFE.getFileType() == ARCDir)  // If this item in the file list is a folder...
        {
            QTreeWidgetItem *item = new QTreeWidgetItem;
            item->setText(0, AFE.getFileName());
            setURLOfItem(item, AFE.getFilePath());
            // Create dummy child item so that the folder can be expanded, removed when folder is expanded
            QTreeWidgetItem *dummyItem = new QTreeWidgetItem;
            item->addChild(dummyItem);
            folderWidget->addChild(item);
        }
    }
}


void MainWindow::filesDroppedInFileListWidget(QList<QUrl> &urlList)
{
    this->setBusyUI(true);
    currentFileServer->copyToServer(urlList, currentFileServer->getCurrentPath());
}


QString MainWindow::getCurrentComboBoxURL()
{
    QString url = "";

//    QLineEdit *le = ui->urlComboBox->lineEdit();
    QLineEdit *le = urlComboBox.lineEdit();
    if (le != NULL)
    {
        url = le->text();
    }

    return url;
}

void MainWindow::setCurrentComboBoxURL(QString url)
{
//    QLineEdit *le = ui->urlComboBox->lineEdit();
    QLineEdit *le = urlComboBox.lineEdit();
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
            currentFileServer->deleteItem(path);
        }
    }
}

void MainWindow::on_urlComboBox_currentIndexChanged(int index)
{
    logger.msg(Arc::DEBUG, "textEditChanged.");
}

