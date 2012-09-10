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
#include "filepropertiesdialog.h"
#include "fileserverfactory.h"
#include "srmsettingsdialog.h"
#include "settings.h"
#include "qdebugstream.h"
#include "transferlistwindow.h"
#include "globalstateinfo.h"
#include "arcstorage.h"
#include "arctools.h"

#include "filetransferlist.h"

#include <arc/Logger.h>


const QString MainWindow::COPY_TO_TEXT            = QString("Copy to...");
const QString MainWindow::COPY_TEXT               = QString("Copy");
const QString MainWindow::DELETE_TEXT             = QString("Delete");
const QString MainWindow::CHANGE_OWNER_TEXT       = QString("Change owner...");
const QString MainWindow::MAKEDIR_TEXT            = QString("Make dir...");
const QString MainWindow::CHANGE_PERMISSIONS_TEXT = QString("Change permissions...");


MainWindow::MainWindow(QWidget *parent, bool childWindow, QString Url):
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    if (!m_childWindow)
        m_windowId = 0;
    qDebug() << "MainWindow()";

    GlobalStateInfo::instance()->setMainWindow(this);
    ARCTools::instance()->initUserConfig();

    m_childWindow = childWindow;
    Settings::loadFromDisk();
    QVariant qvar = Settings::getValue("urlList");
    QList<QVariant> urlList = qvar.toList();

    m_folderWidgetBeingUpdated = NULL;
    m_currentUpdateFileListsMode = CUFLM_clickedBrowse;
    m_transferWindow = 0;

    ui->setupUi(this);

    // Create and add the url combobox manually to the toolbar because QT Designer doesn't support it

    m_startUrl = Url;
    this->readSettings();

    m_urlComboBox.setEditable(true);
    m_urlComboBox.setMaxVisibleItems(10);
    m_urlComboBox.setMaxCount(10);
    for (int i = 0; i < urlList.size(); ++i)
        m_urlComboBox.addItem(urlList.at(i).toString());
    m_urlComboBox.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_urlComboBox.repaint();
    ui->mainToolBar->addWidget(&m_urlComboBox);
    connect(m_urlComboBox.lineEdit(), SIGNAL(returnPressed()), this, SLOT(onURLEditReturnPressed()));  // When someone presses return in the url combobox...
    connect(&m_urlComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onUrlComboBoxCurrentIndexChanged(int)));

    // Can't add empty space in toolbar, so we add a dummy widget instead.

    QWidget* w = new QWidget(ui->mainToolBar);
    w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    w->setMaximumSize(6,0);
    w->setMinimumSize(6,0);
    ui->mainToolBar->addWidget(w);

    ui->filesTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    // Get fileserver and wire it up

    qDebug() << "start url = " << m_startUrl;
    m_currentFileServer = new SRMFileServer();

    // So basically we handle everything using a SRMFileServer (...)

    SRMFileServer* srmFileServer = m_currentFileServer;
    connect(srmFileServer, SIGNAL(onFileListFinished(bool, QString)), this, SLOT(onFileListFinished(bool, QString)));
    connect(srmFileServer, SIGNAL(onError(QString)), this, SLOT(onError(QString)));
    connect(srmFileServer, SIGNAL(onCopyFromServerFinished(bool)), this, SLOT(onCopyFromServerFinished(bool)));
    connect(srmFileServer, SIGNAL(onDeleteFinished(bool)), this, SLOT(onDeleteFinished(bool)));
    connect(srmFileServer, SIGNAL(onMakeDirFinished(bool)), this, SLOT(onMakeDirFinished(bool)));
    connect(srmFileServer, SIGNAL(onCopyToServerFinished(bool, QList<QString>&)), this, SLOT(onCopyToServerFinished(bool, QList<QString>&)));

    // Setup the headers in the file tree widget

    fileTreeHeaderLabels = m_currentFileServer->getFileInfoLabels();
    ui->filesTreeWidget->setColumnCount(fileTreeHeaderLabels.size());
    ui->filesTreeWidget->setHeaderLabels(fileTreeHeaderLabels);
    ui->filesTreeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

    setBusyUI(true);

    // Update file list was here....

    ui->filesTreeWidget->setMainWindow(this);

    if (!m_childWindow) {
        m_debugStream = new QDebugStream(std::cout, ui->textOutput);
        m_debugStream2 = new QDebugStream(std::cerr, ui->textOutput);
    }

    ui->textOutput->clear();

    // Redirect ARC logging to std::cout (in main window)

    if (!m_childWindow) {
        m_logStream = new Arc::LogStream(std::cout);
        m_logStream->setFormat(Arc::ShortFormat);
        Arc::Logger::getRootLogger().addDestination(*m_logStream);
        Arc::Logger::getRootLogger().setThreshold(Arc::ERROR);
    }

    // Set splitter sizes

    ui->splitterHorisontal->setStretchFactor(1,1);
    ui->splitterVertical->setStretchFactor(0,1);
    ui->splitterVertical->setStretchFactor(1,0);

    // Only show log output in main window

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
    ui->actionCreateDir->setIcon(QIcon::fromTheme("folder-new"));
    ui->actionOpenNewLocation->setIcon(QIcon::fromTheme("computer"));
    ui->actionCreateProxyCert->setIcon(QIcon::fromTheme("security-high"));
#endif

    // Center main window

    // Start file processing thread from main window

    if (!m_childWindow)
    {
        m_fileProcessingThread = new FileTransferProcessingThread();
        m_fileProcessingThread->start();
    }
}

MainWindow::~MainWindow()
{
    // Shut down file transfer process thread

    if (!m_childWindow)
        m_fileProcessingThread->shutdown();

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

void MainWindow::setWindowId(int id)
{
    m_windowId = id;
}

int MainWindow::getWindowId()
{
    return m_windowId;
}

void MainWindow::writeSettings()
{
    QSettings settings;

    settings.clear();
    settings.sync();

    if (!m_childWindow)
    {
        settings.beginGroup("MainWindow");
        settings.setValue("size", size());
        settings.setValue("pos", pos());
        settings.setValue("url", this->m_currentFileServer->getCurrentURL());
        settings.endGroup();

        int i;

        for (i=0; i<GlobalStateInfo::instance()->childWindowCount(); i++)
        {
            MainWindow* window = GlobalStateInfo::instance()->getChildWindow(i);
            settings.beginGroup("ChildWindow"+QString::number(window->getWindowId()));
            settings.setValue("size", window->size());
            settings.setValue("pos", window->pos());
            settings.setValue("url", window->getCurrentURL());
            settings.endGroup();
        }
    }
}

void MainWindow::readSettings()
{
    QSettings settings;

    int i;

    if (!m_childWindow)
    {
        if (!settings.childGroups().contains("MainWindow"))
        {
            this->setGeometry(
                        QStyle::alignedRect(
                            Qt::LeftToRight,
                            Qt::AlignCenter,
                            this->size(),
                            qApp->desktop()->availableGeometry()
                            ));
            m_startUrl = QDir::homePath();
        }
        else
        {
            settings.beginGroup("MainWindow");
            resize(settings.value("size", QSize(this->width(), this->height())).toSize());
            move(settings.value("pos", QPoint(this->x(), this->y())).toPoint());
            m_startUrl = settings.value("url", "").toString();
            settings.endGroup();
        }


        for (i=0; i<10; i++)
        {
            QString windowName = "ChildWindow"+QString::number(i);
            if (settings.childGroups().contains(windowName))
            {
                qDebug() << "Found child window " << windowName;
                settings.beginGroup(windowName);
                QString url = settings.value("url", "").toString();
                MainWindow* window = new MainWindow(0, true, url);
                window->resize(settings.value("size", QSize(this->width(), this->height())).toSize());
                window->move(settings.value("pos", QPoint(this->x(), this->y())).toPoint());
                window->show();
                GlobalStateInfo::instance()->addChildWindow(window);
                settings.endGroup();
            }
        }
    }
    else
    {
        qDebug() << "Child window reading settings:";
        QString windowName = "ChildWindow"+QString::number(i);
        settings.beginGroup(windowName);
        m_startUrl = settings.value("url", m_startUrl).toString();
        qDebug() << "  start URL = " << m_startUrl;
        settings.endGroup();
    }
}

void MainWindow::showEvent(QShowEvent *e)
{
    QMainWindow::showEvent(e);

    m_currentFileServer->updateFileList(m_startUrl);
    setCurrentComboBoxURL(m_currentFileServer->getCurrentURL());
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    QList<QVariant> urlList;

    for (int i = 0; i < m_urlComboBox.count(); ++i)
        urlList << m_urlComboBox.itemText(i);

    Settings::setValue("urlList", urlList);
    Settings::saveToDisk();

    if (!m_childWindow)
        this->writeSettings();

    if (!m_childWindow)
        GlobalStateInfo::instance()->closeChildWindows();
    else
        GlobalStateInfo::instance()->removeChildWindow(this);

    QMainWindow::closeEvent(e);
}

void MainWindow::deleteSelectedFiles()
{
    logger.msg(Arc::INFO, "Delete selected files started.");

    // Delete selected file

    QList<QTreeWidgetItem *> selectedItems = ui->filesTreeWidget->selectedItems();

    if (selectedItems.size() != 0)
    {
        // Make sure the user wants to delete the files.

        QMessageBox::StandardButton reply;
        reply = QMessageBox::critical(this, tr("Delete"),
                                      "Are you sure you want to delete selected files ?",
                                      QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes)
        {
            QStringList deleteUrls;

            for (int i=0; i<selectedItems.length(); i++)
            {
                QTreeWidgetItem *selectedItem = selectedItems.at(i);
                QString path = getURLOfItem(selectedItem);
                deleteUrls.append(path);
            }
            setBusyUI(true);
            m_currentFileServer->deleteItems(deleteUrls);
        }
    }
}

void MainWindow::createDir()
{
    m_currentUpdateFileListsMode = CUFLM_clickedBrowse;   // Update the listview displaying the folder...

    bool ok;
    QString path = QInputDialog::getText(this, tr("ArcFTP"),
                                         tr("Folder name:"),
                                         QLineEdit::Normal,
                                         "New Folder",
                                         &ok);
    if (ok && path.isEmpty() != true)
        m_currentFileServer->makeDir(path);
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

void MainWindow::setBusyUI(bool busy)
{
    if (busy == true)
    {
        setCursor(Qt::WaitCursor);
        ui->filesTreeWidget->setEnabled(false);
        m_urlComboBox.setEnabled(false);
        ui->foldersTreeWidget->setEnabled(false);
    }
    else
    {
        setCursor(Qt::ArrowCursor);
        ui->filesTreeWidget->setEnabled(true);
        m_urlComboBox.setEnabled(true);
        ui->foldersTreeWidget->setEnabled(true);
    }
}

QString MainWindow::getCurrentURL()
{
    return m_currentFileServer->getCurrentURL();
}

QMenu* MainWindow::getWindowListMenu()
{
    return ui->menuWindow;
}


//"""
//Output number of bytes according to locale and with IEC binary prefixes
//"""
//if num_bytes is None:
//    print('File size unavailable.')
//    return
//KiB = 1024
//MiB = KiB * KiB
//GiB = KiB * MiB
//TiB = KiB * GiB
//PiB = KiB * TiB
//EiB = KiB * PiB
//ZiB = KiB * EiB
//YiB = KiB * ZiB
//locale.setlocale(locale.LC_ALL, '')
//output = locale.format("%d", num_bytes, grouping=True) + ' bytes'
//if num_bytes > YiB:
//    output += ' (%.3g YiB)' % (num_bytes / YiB)
//elif num_bytes > ZiB:
//    output += ' (%.3g ZiB)' % (num_bytes / ZiB)
//elif num_bytes > EiB:
//    output += ' (%.3g EiB)' % (num_bytes / EiB)
//elif num_bytes > PiB:
//    output += ' (%.3g PiB)' % (num_bytes / PiB)
//elif num_bytes > TiB:
//    output += ' (%.3g TiB)' % (num_bytes / TiB)
//elif num_bytes > GiB:
//    output += ' (%.3g GiB)' % (num_bytes / GiB)
//elif num_bytes > MiB:
//    output += ' (%.3g MiB)' % (num_bytes / MiB)
//elif num_bytes > KiB:
//    output += ' (%.3g KiB)' % (num_bytes / KiB)
//print(output)

QString convertToSizeWithUnit(qint64 num_bytes)
{
    double KiB = 1024;
    double MiB = KiB * KiB;
    double GiB = KiB * MiB;
    double TiB = KiB * GiB;
    double PiB = KiB * TiB;
    double EiB = KiB * PiB;
    double ZiB = KiB * EiB;
    double YiB = KiB * ZiB;

    double q;
    double doubleNumBytes = (double)num_bytes;

    QString unit = "";


    if (doubleNumBytes>YiB)
    {
        q = (double)doubleNumBytes/YiB;
        unit = "Y";
    }
    else if (doubleNumBytes>ZiB)
    {
        q = (double)doubleNumBytes/ZiB;
        unit = " ZB";

    }
    else if (doubleNumBytes>EiB)
    {
        q = (double)doubleNumBytes/EiB;
        unit = " EB";
    }
    else if (doubleNumBytes>PiB)
    {
        q = (double)doubleNumBytes/PiB;
        unit = " PB";
    }
    else if (doubleNumBytes>TiB)
    {
        q = (double)doubleNumBytes/TiB;
        unit = " TB";
    }
    else if (doubleNumBytes>GiB)
    {
        q = (double)doubleNumBytes/GiB;
        unit = " GB";
    }
    else if (doubleNumBytes>MiB)
    {
        q = (double)doubleNumBytes/MiB;
        unit = " MB";
    }
    else if (doubleNumBytes>KiB)
    {
        q = (double)doubleNumBytes/KiB;
        unit = " KB";
    }
    else
    {
        q = doubleNumBytes;
        unit = "";
    }

    QString finalString = QString::number(q, 'g', 5)+unit;
    return finalString;
}

void MainWindow::updateFileTree()
{
    logger.msg(Arc::INFO, "Updating file list.");

    QVector<ARCFileElement*> fileList = m_currentFileServer->getFileList();

    ui->filesTreeWidget->clear();
    ui->filesTreeWidget->setSortingEnabled(false);

    for (int i = 0; i < fileList.size(); ++i)
    {
        ARCFileElement* AFE = fileList.at(i);
        QTreeWidgetItem *item = new QTreeWidgetItem;
        item->setText(0, AFE->getFileName());
        if (AFE->getFileType()==ARCDir)
        {
            item->setIcon(0,QIcon::fromTheme("folder"));
            item->setText(1, "---");
        }
        else
        {
            item->setIcon(0,QIcon::fromTheme("document"));
            //item->setText(1, QString::number(AFE->getSize()));
            item->setText(1, convertToSizeWithUnit(AFE->getSize()));
        }
        if (AFE->getFileType()==ARCDir)
            item->setText(2, "folder");
        else
            item->setText(2, "file");
        item->setText(3, AFE->getLastModfied().toString());
        item->setText(4, AFE->getOwner());
        item->setText(5, AFE->getGroup());
        QString tmpStr;
        tmpStr.sprintf("%x", AFE->getPermissions());
        item->setText(6, tmpStr);
        item->setText(7, AFE->getLastRead().toString());
        setURLOfItem(item, AFE->getFilePath());
        ui->filesTreeWidget->addTopLevelItem(item);
    }

    ui->filesTreeWidget->setSortingEnabled(true);
    ui->filesTreeWidget->sortByColumn(2, Qt::DescendingOrder);
    ui->filesTreeWidget->header()->setResizeMode(QHeaderView::ResizeToContents);

    for (int i=0; i<8; i++)
        ui->filesTreeWidget->resizeColumnToContents(i);

    logger.msg(Arc::INFO, "File list update done.");
}


void MainWindow::updateFoldersTree()
{
    logger.msg(Arc::INFO, "Updating folder tree.");

    QVector<ARCFileElement*> fileList = m_currentFileServer->getFileList();

    ui->foldersTreeWidget->clear();

    for (int i = 0; i < fileList.size(); ++i)
    {
        ARCFileElement* AFE = fileList.at(i);
        if (AFE->getFileType() == ARCDir)  // If this item in the file list is a folder...
        {
            QTreeWidgetItem *item = new QTreeWidgetItem;
            item->setText(0, AFE->getFileName());
            item->setIcon(0, QIcon::fromTheme("folder"));
            setURLOfItem(item, AFE->getFilePath());
            // Create dummy child item so that the folder can be expanded, removed when folder is expanded
            QTreeWidgetItem *dummyItem = new QTreeWidgetItem;
            item->addChild(dummyItem);
            ui->foldersTreeWidget->addTopLevelItem(item);
        }
    }
    logger.msg(Arc::INFO, "Folder tree update done.");
}

void MainWindow::updateFoldersTreeBelow()
{
    logger.msg(Arc::INFO, "Update folders tree below.");

    m_currentFileServer->setNotifyParent(false);
    QString currentURL = m_currentFileServer->getCurrentURL();
    m_currentFileServer->goUpOneFolder();

    QVector<ARCFileElement*> fileList = m_currentFileServer->getFileList();

    ui->foldersTreeWidget->clear();

    for (int i = 0; i < fileList.size(); ++i)
    {
        ARCFileElement* AFE = fileList.at(i);
        if (AFE->getFileType() == ARCDir)  // If this item in the file list is a folder...
        {
            QTreeWidgetItem *item = new QTreeWidgetItem;
            item->setText(0, AFE->getFileName());
            item->setIcon(0, QIcon::fromTheme("folder"));
            setURLOfItem(item, AFE->getFilePath());
            // Create dummy child item so that the folder can be expanded, removed when folder is expanded
            QTreeWidgetItem *dummyItem = new QTreeWidgetItem;
            item->addChild(dummyItem);
            ui->foldersTreeWidget->addTopLevelItem(item);
        }
    }

    m_currentFileServer->updateFileList(currentURL);
    m_currentFileServer->setNotifyParent(true);
    logger.msg(Arc::INFO, "Folder tree update done.");
}

void MainWindow::expandFolderTreeWidget(QTreeWidgetItem *folderWidget)
{
    QVector<ARCFileElement*> fileList = m_currentFileServer->getFileList();

    for (int i = 0; i < fileList.size(); ++i)
    {
        ARCFileElement* AFE = fileList.at(i);
        if (AFE->getFileType() == ARCDir)  // If this item in the file list is a folder...
        {
            QTreeWidgetItem *item = new QTreeWidgetItem;
            item->setText(0, AFE->getFileName());
            item->setIcon(0, QIcon::fromTheme("folder"));
            setURLOfItem(item, AFE->getFilePath());
            // Create dummy child item so that the folder can be expanded, removed when folder is expanded
            QTreeWidgetItem *dummyItem = new QTreeWidgetItem;
            item->addChild(dummyItem);
            folderWidget->addChild(item);
        }
    }
}


QString MainWindow::getCurrentComboBoxURL()
{
    QString url = "";

    QLineEdit *le = m_urlComboBox.lineEdit();
    if (le != NULL)
    {
        url = le->text();
    }

    return url;
}

void MainWindow::setCurrentComboBoxURL(QString url)
{
    QLineEdit *le = m_urlComboBox.lineEdit();
    if (le != NULL)
    {
        le->setText(url);
    }
}

void MainWindow::onFilesDroppedInFileListWidget(QList<QUrl>& urlList)
{
    logger.msg(Arc::INFO, "Files dropped in window.");

    GlobalStateInfo::instance()->showTransferWindow();

    this->setBusyUI(true);

    int i;

    for (i=0; i<urlList.length(); i++)
        logger.msg(Arc::INFO, "file:" + urlList[i].toString().toStdString());

    FileTransferList::instance()->pauseProcessing();
    m_currentFileServer->copyToServer(urlList, m_currentFileServer->getCurrentPath());
    FileTransferList::instance()->resumeProcessing();
}

void MainWindow::onFileListFinished(bool error, QString errorMsg)
{
    setBusyUI(false);

    if (error == true)
    {
        logger.msg(Arc::ERROR, "Update of file list failed.");
        m_currentUpdateFileListsMode = CUFLM_noUpdate;
        QMessageBox::information(this, tr("ArcFTP"), errorMsg);
    }
    else
    {
        switch (m_currentUpdateFileListsMode)
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
            expandFolderTreeWidget(m_folderWidgetBeingUpdated);
            m_folderWidgetBeingUpdated = NULL;
            break;
        case CUFLM_doubleClickedFolder:
            updateFileTree();
            updateFoldersTreeBelow();
        default:
            qDebug() << "shouldn't happen.";
            break;
        }
    }

    m_currentUpdateFileListsMode = CUFLM_noUpdate;


}

void MainWindow::onCopyFromServerFinished(bool error)
{
    logger.msg(Arc::INFO, "Copy from file server finished.");
    updateFileTree();
    GlobalStateInfo::instance()->hideTransferWindow();
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
    logger.msg(Arc::INFO, "File deletion finished.");
    this->updateFileTree();

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
    logger.msg(Arc::INFO, "Make dir finished.");
    //m_currentUpdateFileListsMode = CUFLM_clickedBrowse;   // Update the listview displaying the folder...
    //onFileListFinished(false, "");                      // ... so that the deleted file is removed

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
    logger.msg(Arc::INFO, "Copy to server finished.");
    m_currentUpdateFileListsMode = CUFLM_clickedFolder;   // Update the listview displaying the folder...
    onFileListFinished(false, "");                      // ... so that the deleted file is removed

    setBusyUI(false);

    GlobalStateInfo::instance()->hideTransferWindow();

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

void MainWindow::onURLEditReturnPressed()
{
    on_actionReload_triggered();
}

void MainWindow::onUrlComboBoxCurrentIndexChanged(int index)
{
    logger.msg(Arc::DEBUG, "textEditChanged.");
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

void MainWindow::on_actionAbout_ARC_File_Navigator_triggered()
{
    QMessageBox msgBox;
    msgBox.setText("ARC Storage Explorer\n\nCopyright (C) 2011-2012 Lunarc, Lund University\n\nDeveloped by\n\nUser interface - Alexander Lapajne\nJonas Lindemann");
    msgBox.exec();
}

void MainWindow::on_actionUp_triggered()
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

void MainWindow::on_actionQuit_triggered()
{
    this->close();
}

void MainWindow::on_actionDelete_triggered()
{    
    this->deleteSelectedFiles();
}

void MainWindow::on_actionNewWindow_triggered()
{
    MainWindow* window = new MainWindow(0, true, this->getCurrentURL());
    QRect r = this->geometry();
    r.setLeft(this->geometry().left()+150);
    r.setTop(this->geometry().top()+150);
    r.setRight(this->geometry().right()+150);
    r.setBottom(this->geometry().bottom()+150);

    qDebug() << r;
    window->setGeometry(r);
    window->show();

    GlobalStateInfo::instance()->addChildWindow(window);
}

void MainWindow::on_filesTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    if (item->text(2)=="folder")
    {
        QString clickedString = item->text(0);
        logger.msg(Arc::INFO, "on_filesTreeWidget_itemDoubleClicked() %s", clickedString.toStdString());
        setBusyUI(true);
        m_currentUpdateFileListsMode = CUFLM_clickedBrowse  ;
        QString newURL = getURLOfItem(item);
        m_folderWidgetBeingUpdated = item;
        m_currentFileServer->updateFileList(newURL);
        setCurrentComboBoxURL(m_currentFileServer->getCurrentURL());
    }
}

void MainWindow::on_actionClearSelection_triggered()
{
    ui->filesTreeWidget->clearSelection();
    ui->foldersTreeWidget->clearSelection();
}

void MainWindow::on_actionSelectAllFiles_triggered()
{
    qDebug() << "on_actionSelectAllFiles_triggered()";

    ui->filesTreeWidget->selectAll();
    /*
    for (int i; i<ui->filesTreeWidget->topLevelItemCount(); i++)
    {
        QTreeWidgetItem* item = ui->filesTreeWidget->topLevelItem(i);
        qDebug() << item->text(2);
        if (item->text(2) == "file")
        {
            qDebug() << "Selecting file";
            ui->filesTreeWidget->topLevelItem(i)->setSelected(true);
        }
    }
    */

    /*
    QList<QTreeWidgetItem*> removeItems;

    for (int i; i<ui->filesTreeWidget->selectedItems().count(); i++)
    {
        QTreeWidgetItem* item = ui->filesTreeWidget->selectedItems().at(i);
        qDebug() << item->text(2);
        if (item->text(2) == "file")
        {
            qDebug() << "Selecting file";
        }
        else
        {
            removeItems.append(item);
        }
    }

    for (int i=0; i<removeItems.count(); i++)
        ui->filesTreeWidget
    */
}

void MainWindow::on_actionCreateDir_triggered()
{
    this->createDir();
}


void MainWindow::on_actionShowTransferList_triggered()
{
    GlobalStateInfo::instance()->showTransferWindow();
}

void MainWindow::on_actionOpenNewLocation_triggered()
{
    bool ok;
    QString url = QInputDialog::getText(this, tr("Open location"),
                                        tr("Url"), QLineEdit::Normal,
                                        "", &ok);
    if (ok && !url.isEmpty())
    {
        MainWindow* window = new MainWindow(0, true, url);
        QRect r = this->geometry();
        r.setLeft(this->geometry().left()+150);
        r.setTop(this->geometry().top()+150);
        r.setRight(this->geometry().right()+150);
        r.setBottom(this->geometry().bottom()+150);

        qDebug() << r;
        window->setGeometry(r);
        window->show();

        GlobalStateInfo::instance()->addChildWindow(window);
    }
}

void MainWindow::on_actionSRM_Preferences_triggered()
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

void MainWindow::openUrl(QString url)
{
    setBusyUI(true);
    m_currentUpdateFileListsMode = CUFLM_clickedBrowse;

    // Delete previous file server

    if (m_currentFileServer!=0)
        delete m_currentFileServer;

    // Create and wire up new file server.

    m_currentFileServer = new SRMFileServer();
    SRMFileServer* srmFileServer = m_currentFileServer;
    connect(srmFileServer, SIGNAL(onFileListFinished(bool, QString)), this, SLOT(onFileListFinished(bool, QString)));
    connect(srmFileServer, SIGNAL(onError(QString)), this, SLOT(onError(QString)));
    connect(srmFileServer, SIGNAL(onCopyFromServerFinished(bool)), this, SLOT(onCopyFromServerFinished(bool)));
    connect(srmFileServer, SIGNAL(onDeleteFinished(bool)), this, SLOT(onDeleteFinished(bool)));
    connect(srmFileServer, SIGNAL(onMakeDirFinished(bool)), this, SLOT(onMakeDirFinished(bool)));
    connect(srmFileServer, SIGNAL(onCopyToServerFinished(bool, QList<QString>&)), this, SLOT(onCopyToServerFinished(bool, QList<QString>&)));

    // Setup the headers in the file tree widget (in case it's a new file server)

    fileTreeHeaderLabels = m_currentFileServer->getFileInfoLabels();
    ui->filesTreeWidget->setColumnCount(fileTreeHeaderLabels.size());
    ui->filesTreeWidget->setHeaderLabels(fileTreeHeaderLabels);

    while (url.endsWith('/')) { url = url.left(url.length() - 1); }  // Get rid of trailing /

    m_currentFileServer->updateFileList(url);

    setCurrentComboBoxURL(m_currentFileServer->getCurrentURL());
}

void MainWindow::on_actionReload_triggered()
{
    QString url = getCurrentComboBoxURL();
    this->openUrl(url);
}

void MainWindow::on_actionCreateProxyCert_triggered()
{
    ARCTools::instance()->proxyCertificateTool();
}

void MainWindow::on_actionConvertCertificates_triggered()
{
    ARCTools::instance()->certConversionTool();
}

void MainWindow::on_actionJobManager_triggered()
{
    ARCTools::instance()->jobManagerTool();
}

void MainWindow::on_actionJobSubmissionTool_triggered()
{
    ARCTools::instance()->submissionTool();
}

void MainWindow::on_actionStop_triggered()
{
    FileTransferList::instance()->cancelAllTransfers();
}
