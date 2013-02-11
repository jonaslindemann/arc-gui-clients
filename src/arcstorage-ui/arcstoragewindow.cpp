#include <QVector>
#include <QString>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMessageBox>
#include <QFileDialog>
#include <QVariant>
#include <QInputDialog>
#include <QSpacerItem>
#include <QApplication>
#include <QProcess>
#include <iostream>
#include "arcstoragewindow.h"
#include "ui_arcstoragewindow.h"
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
#include "applicationsettings.h"

#include "filetransferlist.h"

#include <arc/Logger.h>


const QString ArcStorageWindow::COPY_TO_TEXT            = QString("Copy to...");
const QString ArcStorageWindow::COPY_TEXT               = QString("Copy");
const QString ArcStorageWindow::DELETE_TEXT             = QString("Delete");
const QString ArcStorageWindow::CHANGE_OWNER_TEXT       = QString("Change owner...");
const QString ArcStorageWindow::MAKEDIR_TEXT            = QString("Make dir...");
const QString ArcStorageWindow::CHANGE_PERMISSIONS_TEXT = QString("Change permissions...");


ArcStorageWindow::ArcStorageWindow(QWidget *parent, bool childWindow, QString Url):
    QMainWindow(parent),
    ui(new Ui::ArcStorageWindow)
{
    if (!childWindow)
    {
        // This is the main window.

        // Assign the main window property of the global state singleton

        GlobalStateInfo::instance()->setMainWindow(this);

        // The main window is responsible for initialising ARC user configuration

        ARCTools::instance()->initUserConfig();
    }

    m_childWindow = childWindow;

    if (!m_childWindow)
        m_windowId = -1;

    // Load settings from disk

    //Settings::loadFromDisk();
    //QVariant qvar = Settings::getValue("urlList");
    //QList<QVariant> urlList = qvar.toList();

    // Initialise state variables

    m_folderWidgetBeingUpdated = NULL;
    m_currentUpdateFileListsMode = CUFLM_clickedBrowse;
    m_transferWindow = 0;
    m_filePropertyInspector = 0;
    m_startUrl = Url;

    // Initialise Qt user interface

    ui->setupUi(this);

    // Load settings from disk

    this->readSettings();

    // Create and add the url combobox manually to the toolbar because QT Designer doesn't support it

    m_urlComboBox.setEditable(true);
    m_urlComboBox.setMaxVisibleItems(10);
    m_urlComboBox.setMaxCount(10);
    //for (int i = 0; i < urlList.size(); ++i)
    //    m_urlComboBox.addItem(urlList.at(i).toString());
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
        ui->textOutput->clear();
    }

    // Redirect ARC logging to std::cout (in main window)

    if (!m_childWindow) {
        m_logStream = new Arc::LogStream(std::cout);
        m_logStream->setFormat(Arc::ShortFormat);
        Arc::Logger::getRootLogger().addDestination(*m_logStream);
        Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

        logger.msg(Arc::INFO, "ARC Storage Explorer initialising...");
    }

    // Set splitter sizes

    ui->splitterHorisontal->setStretchFactor(1,1);
    ui->splitterVertical->setStretchFactor(0,1);
    ui->splitterVertical->setStretchFactor(1,0);

    // Only show log output in main window

    if (m_childWindow)
        ui->textOutput->hide();

    ui->actionBack->setDisabled(true);

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
    ui->actionDownloadSelected->setIcon(QIcon::fromTheme("document-save"));
    ui->actionUploadSelected->setIcon(QIcon(":/resources/icons/document-upload.png"));
#endif

    // Center main window

    // Start file processing thread from main window

    if (!m_childWindow)
    {
        logger.msg(Arc::INFO, "File transfer thread starting...");
        m_fileProcessingThread = new FileTransferProcessingThread();
        m_fileProcessingThread->start();
    }
}

ArcStorageWindow::~ArcStorageWindow()
{
    // Shut down file transfer process thread (from Main Window)

    if (!m_childWindow)
        m_fileProcessingThread->shutdown();

    // Disconnect log streams

    if (!m_childWindow)
    {
        ARCTools::instance()->closeHelpWindow();
        Arc::Logger::getRootLogger().removeDestinations();
        delete m_logStream;
        delete m_debugStream;
        delete m_debugStream2;
    }

    delete ui;
}

void ArcStorageWindow::setWindowId(int id)
{
    m_windowId = id;
}

int ArcStorageWindow::getWindowId()
{
    return m_windowId;
}

void ArcStorageWindow::writeSettings()
{
    logger.msg(Arc::VERBOSE, "Writing settings (writeSettings)");

    // Get Qt settings singleton

    QSettings settings;

    settings.sync();

    if (!m_childWindow)
    {
        // Main window is responsible for saving _all_ settings

        GlobalStateInfo::instance()->writeSettings();

        settings.remove("MainWindow");
        settings.beginGroup("MainWindow");
        settings.setValue("size", size());
        settings.setValue("pos", pos());
        settings.setValue("url", this->m_currentFileServer->getCurrentURL());
        settings.endGroup();

        int i;

        for (i=0; i<10; i++)
            settings.remove("ChildWindow"+QString::number(i));

        for (i=0; i<GlobalStateInfo::instance()->childWindowCount(); i++)
        {
            ArcStorageWindow* window = GlobalStateInfo::instance()->getChildWindow(i);
            settings.remove("ChildWindow"+QString::number(window->getWindowId()));
            settings.beginGroup("ChildWindow"+QString::number(window->getWindowId()));
            settings.setValue("size", window->size());
            settings.setValue("pos", window->pos());
            settings.setValue("url", window->getCurrentURL());
            settings.endGroup();
        }
    }
}

void ArcStorageWindow::readSettings()
{
    logger.msg(Arc::VERBOSE, "Reading settings (readSettings)");

    QSettings settings;

    int i;

    if (!m_childWindow)
    {
        // Main window is responsible for reading settings

        // Read global settings

        GlobalStateInfo::instance()->readSettings();

        // Read main window position and settings

        if (!settings.childGroups().contains("MainWindow"))
        {
            // If no settings found place window reasonable

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

        // Create child windows

        for (i=0; i<10; i++)
        {
            QString windowName = "ChildWindow"+QString::number(i);
            if (settings.childGroups().contains(windowName))
            {
                settings.beginGroup(windowName);
                QString url = settings.value("url", "").toString();
                ArcStorageWindow* window = new ArcStorageWindow(0, true, url);
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
        // Child window reads its start URL from settings

        QString windowName = "ChildWindow"+QString::number(i);
        settings.beginGroup(windowName);
        m_startUrl = settings.value("url", m_startUrl).toString();
        settings.endGroup();
    }
}

void ArcStorageWindow::pushUrl(QString url)
{
    if (url!="")
    {
        m_backStack.push_back(url);
        ui->actionBack->setEnabled(true);

        QVectorIterator<QString> i(m_backStack);

        /*
        qDebug() << "-->";
        while (i.hasNext())
            qDebug() << i.next();
        qDebug() << "<--";
        */
    }
}

QString ArcStorageWindow::popUrl()
{
    if (m_backStack.size()>0)
    {
        QString url = m_backStack.back();
        m_backStack.pop_back();

        if (m_backStack.size()==0)
            ui->actionBack->setDisabled(true);
        else
            ui->actionBack->setEnabled(true);

        return url;
    }
    else
    {
        ui->actionBack->setDisabled(true);
        return m_currentFileServer->getCurrentURL();
    }
}

void ArcStorageWindow::showEvent(QShowEvent *e)
{
    // This method is called when the window is visible

    QMainWindow::showEvent(e);

    // Update file list

    m_currentFileServer->updateFileList(m_startUrl);

    setCurrentComboBoxURL(m_currentFileServer->getCurrentURL());

    if (!m_childWindow)
    {
        // Check if we should open from command line arguments

        if (QApplication::argc()>1)
        {
            QString url = QApplication::arguments().at(1);
            logger.msg(Arc::INFO, "Opening url = ", url.toStdString());
            this->openUrl(url);
        }
    }

    this->updateFoldersTree();
}

void ArcStorageWindow::closeEvent(QCloseEvent *e)
{
    logger.msg(Arc::VERBOSE, "Received close event. (closeEvent)");
    // This method is called before window is closed.
    // Saves settings. Closes child windows.
    /*
    QList<QVariant> urlList;

    for (int i = 0; i < m_urlComboBox.count(); ++i)
        urlList << m_urlComboBox.itemText(i);

    Settings::setValue("urlList", urlList);
    Settings::saveToDisk();
    */

    if (!m_childWindow)
        this->writeSettings();

    if (!m_childWindow)
        GlobalStateInfo::instance()->closeChildWindows();
    else
        GlobalStateInfo::instance()->removeChildWindow(this);

    QMainWindow::closeEvent(e);
}

void ArcStorageWindow::deleteSelectedFiles()
{
    logger.msg(Arc::VERBOSE, "Delete selected files started. (deleteSelectedFiles)");

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

void ArcStorageWindow::createDir()
{
    logger.msg(Arc::VERBOSE, "Creating directory. (createDir)");
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


QString ArcStorageWindow::getURLOfItem(QTreeWidgetItem *item)
{
    QVariant dataQV = item->data(0, Qt::ToolTipRole);
    return dataQV.toString();
}

void ArcStorageWindow::setURLOfItem(QTreeWidgetItem *item, QString URL)
{
    QVariant dataQV = URL;
    item->setData(0, Qt::ToolTipRole, dataQV);
}

void ArcStorageWindow::setBusyUI(bool busy)
{
    if (busy == true)
    {
        logger.msg(Arc::VERBOSE, "Disable UI");
        ui->statusBar->showMessage("Processing...");
        ui->menuBar->setEnabled(false);
        ui->mainToolBar->setEnabled(false);
        ui->filesTreeWidget->setEnabled(false);
        m_urlComboBox.setEnabled(false);
        ui->foldersTreeWidget->setEnabled(false);
    }
    else
    {
        logger.msg(Arc::VERBOSE, "Enable UI");
        ui->statusBar->clearMessage();
        ui->menuBar->setEnabled(true);
        ui->mainToolBar->setEnabled(true);
        ui->filesTreeWidget->setEnabled(true);
        m_urlComboBox.setEnabled(true);
        ui->foldersTreeWidget->setEnabled(true);
    }
}

QString ArcStorageWindow::getCurrentURL()
{
    return m_currentFileServer->getCurrentURL();
}

QMenu* ArcStorageWindow::getWindowListMenu()
{
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

void ArcStorageWindow::updateFileTree()
{
    logger.msg(Arc::VERBOSE, "Updating file list. (updateFileTree)");

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
        setURLOfItem(item, AFE->getFilePath());
        ui->filesTreeWidget->addTopLevelItem(item);
    }

    ui->filesTreeWidget->setSortingEnabled(true);
    ui->filesTreeWidget->sortByColumn(2, Qt::DescendingOrder);
    ui->filesTreeWidget->header()->setResizeMode(QHeaderView::ResizeToContents);

    for (int i=0; i<8; i++)
        ui->filesTreeWidget->resizeColumnToContents(i);

    this->setCurrentComboBoxURL(m_currentFileServer->getCurrentURL());

    logger.msg(Arc::VERBOSE, "File list update done.");
}


void ArcStorageWindow::updateFoldersTree()
{
    logger.msg(Arc::VERBOSE, "Updating folder tree. (updateFolderTree)");

    QVector<ARCFileElement*> fileList = m_currentFileServer->getFileList();
    m_folderListUrl = m_currentFileServer->getCurrentURL();

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
    logger.msg(Arc::VERBOSE, "Folder tree update done.");
}

void ArcStorageWindow::updateFoldersTreeBelow()
{
    logger.msg(Arc::VERBOSE, "Update folders tree below. (updateFoldersTreeBelow)");

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

void ArcStorageWindow::expandFolderTreeWidget(QTreeWidgetItem *folderWidget)
{
    logger.msg(Arc::VERBOSE, "Expanding folder in tree. (expandFolderTreeWidget)");

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


QString ArcStorageWindow::getCurrentComboBoxURL()
{
    QString url = "";

    QLineEdit *le = m_urlComboBox.lineEdit();
    if (le != NULL)
    {
        url = le->text();
    }

    return url;
}

void ArcStorageWindow::setCurrentComboBoxURL(QString url)
{
    QLineEdit *le = m_urlComboBox.lineEdit();
    if (le != NULL)
    {
        ui->urlBreadCrumbToolbar->clear();
        m_breadCrumbItems.clear();

        QStringList urlParts = url.split("/");
        QString buildPath = "";

        if (urlParts.at(0).length()==0)
        {
            buildPath = "";

            for (int i=1; i<urlParts.count(); i++)
            {
                QAction* action = ui->urlBreadCrumbToolbar->addAction(urlParts.at(i));
                ui->urlBreadCrumbToolbar->addSeparator();
                buildPath += "/" + urlParts.at(i);
                m_breadCrumbItems.append(buildPath);
                action->setStatusTip(buildPath);
                action->setToolTip(buildPath);
                connect(action, SIGNAL(triggered()), this, SLOT(onBreadCrumbTriggered()));
            }
        }
        else
        {
            buildPath = urlParts.at(0)+"/";

            for (int i=2; i<urlParts.count(); i++)
            {
                QAction* action = ui->urlBreadCrumbToolbar->addAction(urlParts.at(i));
                ui->urlBreadCrumbToolbar->addSeparator();
                buildPath += "/" + urlParts.at(i);
                m_breadCrumbItems.append(buildPath);
                action->setStatusTip(buildPath);
                action->setToolTip(buildPath);
                connect(action, SIGNAL(triggered()), this, SLOT(onBreadCrumbTriggered()));
            }
        }
        le->setText(url);
    }
}

void ArcStorageWindow::onFilesDroppedInFileListWidget(QList<QUrl>& urlList)
{
    logger.msg(Arc::VERBOSE, "Files dropped in window. (onFilesDropped in file list widget)");

    GlobalStateInfo::instance()->showTransferWindow();

    this->setBusyUI(true);

    int i;

    for (i=0; i<urlList.length(); i++)
        logger.msg(Arc::VERBOSE, "Dropped file :" + urlList[i].toString().toStdString());

    FileTransferList::instance()->pauseProcessing();
    m_currentFileServer->copyToServer(urlList, m_currentFileServer->getCurrentPath());
    FileTransferList::instance()->resumeProcessing();
}

void ArcStorageWindow::onFileListFinished(bool error, QString errorMsg)
{
    logger.msg(Arc::VERBOSE, "File listing finished. (onFileListFinished)");

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
        case CUFLM_syncBoth:
            updateFoldersTree();
            updateFileTree();
            break;
        case CUFLM_clickedBrowse:
            //---updateFoldersTree();
            updateFileTree();
            break;
        case CUFLM_clickedFolder:
            updateFileTree();
            break;
        case CUFLM_clickedUp:
            //---updateFoldersTree();
            updateFileTree();

            if (m_folderListUrl.length()>m_currentFileServer->getCurrentURL().length())
                updateFoldersTree();

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

void ArcStorageWindow::onCopyFromServerFinished(bool error)
{
    logger.msg(Arc::VERBOSE, "Copy from file server finished. (onCopyFromServerFinished)");
    updateFileTree();
    GlobalStateInfo::instance()->hideTransferWindow();
    setBusyUI(false);
    if (error == true)
        QMessageBox::information(this, tr("ArcFTP"), "An error occured while trying to copy the file");
}

void ArcStorageWindow::onDeleteFinished(bool error)
{
    logger.msg(Arc::VERBOSE, "File deletion finished. (onDeleteFinished)");
    this->updateFileTree();

    setBusyUI(false);
    if (error == true)
        QMessageBox::information(this, tr("ArcFTP"), "Delete failed");
}


void ArcStorageWindow::onMakeDirFinished(bool error)
{
    logger.msg(Arc::VERBOSE, "Make dir finished. (onMakeDirFinished)");

    setBusyUI(false);

    if (error == true)
        QMessageBox::information(this, tr("ArcFTP"), "Makedir failed");
}


void ArcStorageWindow::onCopyToServerFinished(bool error, QList<QString> &failedFiles)
{
    logger.msg(Arc::VERBOSE, "Copy to server finished. (onCopyToServerFinished)");
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
}


void ArcStorageWindow::onError(QString errorStr)
{
    setBusyUI(false);

    QMessageBox::information(this, tr("ArcFTP"), errorStr);
}

void ArcStorageWindow::onNewStatus(QString statusStr)
{
    //ui->statusLabel->setText(statusStr);
}

void ArcStorageWindow::onContextMenu(const QPoint& pos)
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

void ArcStorageWindow::onURLEditReturnPressed()
{
    on_actionReload_triggered();
}

void ArcStorageWindow::onUrlComboBoxCurrentIndexChanged(int index)
{
}

void ArcStorageWindow::onBreadCrumbTriggered()
{
    QAction* action = (QAction*)sender();
    this->openUrl(action->toolTip());
}

void ArcStorageWindow::on_foldersTreeWidget_expanded(QModelIndex index)
{
    logger.msg(Arc::VERBOSE, "on_foldersTreeWidget_expanded() %d ", index.row());
}


void ArcStorageWindow::on_foldersTreeWidget_itemExpanded(QTreeWidgetItem* item)
{
    logger.msg(Arc::VERBOSE, "on_foldersTreeWidget_itemExpanded() %s", item->text(0).toStdString());

    m_currentUpdateFileListsMode = CUFLM_expandedFolder;

    while (item->childCount() > 0)  // Clear all children before adding new ones
        item->removeChild(item->child(0));

    QString newURL = getURLOfItem(item);
    setBusyUI(true);
    m_folderWidgetBeingUpdated = item;
    m_currentFileServer->updateFileList(newURL);
}

void ArcStorageWindow::on_foldersTreeWidget_itemClicked(QTreeWidgetItem* item, int column)
{
    QString clickedString = item->text(0);
    logger.msg(Arc::VERBOSE, "on_foldersTreeWidget_itemClicked() %s", clickedString.toStdString());
    setBusyUI(true);
    m_currentUpdateFileListsMode = CUFLM_clickedFolder;
    QString newURL = getURLOfItem(item);
    m_folderWidgetBeingUpdated = item;
    this->pushUrl(m_currentFileServer->getCurrentURL());
    m_currentFileServer->updateFileList(newURL); // --> startUpdateFileList
    setCurrentComboBoxURL(m_currentFileServer->getCurrentURL());
}


void ArcStorageWindow::on_foldersTreeWidget_clicked(QModelIndex index)
{
}

void ArcStorageWindow::on_actionAbout_ARC_File_Navigator_triggered()
{
    QMessageBox msgBox;
    msgBox.setText("ARC Storage Explorer\n\nCopyright (C) 2011-2013 Lunarc, Lund University\n\nDeveloped by\n\nUser interface - Alexander Lapajne\nJonas Lindemann");
    msgBox.exec();
}

void ArcStorageWindow::on_actionUp_triggered()
{
    setBusyUI(true);
    m_currentUpdateFileListsMode = CUFLM_clickedUp;
    this->pushUrl(m_currentFileServer->getCurrentURL());
    if (m_currentFileServer->goUpOneFolder() == true)
        setCurrentComboBoxURL(m_currentFileServer->getCurrentURL());
    else
        setBusyUI(false);
}

void ArcStorageWindow::on_actionQuit_triggered()
{
    this->close();
}

void ArcStorageWindow::on_actionDelete_triggered()
{    
    this->deleteSelectedFiles();
}

void ArcStorageWindow::on_actionNewWindow_triggered()
{
    ArcStorageWindow* window = new ArcStorageWindow(0, true, this->getCurrentURL());
    QRect r = this->geometry();
    r.setLeft(this->geometry().left()+150);
    r.setTop(this->geometry().top()+150);
    r.setRight(this->geometry().right()+150);
    r.setBottom(this->geometry().bottom()+150);

    window->setGeometry(r);
    window->show();

    GlobalStateInfo::instance()->addChildWindow(window);
}

void ArcStorageWindow::on_filesTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    if (item->text(2)=="folder")
    {
        QString clickedString = item->text(0);
        logger.msg(Arc::VERBOSE, "on_filesTreeWidget_itemDoubleClicked() %s", clickedString.toStdString());
        setBusyUI(true);
        m_currentUpdateFileListsMode = CUFLM_clickedBrowse  ;
        QString newURL = getURLOfItem(item);
        m_folderWidgetBeingUpdated = item;
        this->pushUrl(m_currentFileServer->getCurrentURL());
        m_currentFileServer->startUpdateFileList(newURL);
        setCurrentComboBoxURL(m_currentFileServer->getCurrentURL());
    }
}

void ArcStorageWindow::on_actionClearSelection_triggered()
{
    ui->filesTreeWidget->clearSelection();
    ui->foldersTreeWidget->clearSelection();
}

void ArcStorageWindow::on_actionSelectAllFiles_triggered()
{
    ui->filesTreeWidget->selectAll();
}

void ArcStorageWindow::on_actionCreateDir_triggered()
{
    this->createDir();
}


void ArcStorageWindow::on_actionShowTransferList_triggered()
{
    GlobalStateInfo::instance()->showTransferWindow();
}

void ArcStorageWindow::on_actionOpenNewLocation_triggered()
{
    bool ok;
    QString url = QInputDialog::getText(this, tr("Open location"),
                                        tr("Url"), QLineEdit::Normal,
                                        "", &ok);
    if (ok && !url.isEmpty())
    {
        ArcStorageWindow* window = new ArcStorageWindow(0, true, url);
        QRect r = this->geometry();
        r.setLeft(this->geometry().left()+150);
        r.setTop(this->geometry().top()+150);
        r.setRight(this->geometry().right()+150);
        r.setBottom(this->geometry().bottom()+150);

        window->setGeometry(r);
        window->show();

        GlobalStateInfo::instance()->addChildWindow(window);
    }
}

void ArcStorageWindow::on_actionSRM_Preferences_triggered()
{
    SRMSettingsDialog dialog(this);
    dialog.setConfigFilename(Settings::getStringValue("srmConfigFilename"));
    dialog.setModal(true);
    int dialogReturnValue = dialog.exec();
    if (dialogReturnValue != 0)
        Settings::setValue("srmConfigFilename", dialog.getConfigFilename());
}

void ArcStorageWindow::openUrl(QString url)
{
    setBusyUI(true);
    m_currentUpdateFileListsMode = CUFLM_syncBoth;

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

    this->pushUrl(m_currentFileServer->getCurrentURL());
    m_folderListUrl = url;
    m_currentFileServer->startUpdateFileList(url);
    setCurrentComboBoxURL(m_currentFileServer->getCurrentURL());
}

void ArcStorageWindow::on_actionReload_triggered()
{
    QString url = getCurrentComboBoxURL();
    this->openUrl(url);
}

void ArcStorageWindow::on_actionCreateProxyCert_triggered()
{
    ARCTools::instance()->proxyCertificateTool();
}

void ArcStorageWindow::on_actionConvertCertificates_triggered()
{
    ARCTools::instance()->certConversionTool();
}

void ArcStorageWindow::on_actionJobManager_triggered()
{
    ARCTools::instance()->jobManagerTool();
}

void ArcStorageWindow::on_actionJobSubmissionTool_triggered()
{
    ARCTools::instance()->submissionTool();
}

void ArcStorageWindow::on_actionStop_triggered()
{
    FileTransferList::instance()->cancelAllTransfers();
}

void ArcStorageWindow::on_actionSettings_triggered()
{
    ApplicationSettings dialog(this);
    dialog.setModal(true);
    dialog.exec();
}

void ArcStorageWindow::on_filesTreeWidget_customContextMenuRequested(const QPoint &pos)
{
    QMenu menu(this);
    QList<QAction*> actions;
    actions.append(ui->actionCopyURL);
    actions.append(ui->actionCopyURLFilename);
    actions.append(ui->actionShowFileProperties);
    menu.addActions(actions);
    QPoint globalPos = ui->filesTreeWidget->mapToGlobal(pos);
    menu.exec(globalPos);
}

void ArcStorageWindow::on_actionCopyURL_triggered()
{
    QList<QTreeWidgetItem *> selectedItems = ui->filesTreeWidget->selectedItems();

    if (selectedItems.size() != 0)
    {
        QApplication::clipboard()->clear();
        QStringList urls;

        for (int i=0; i<selectedItems.count(); i++)
            urls.append(getURLOfItem(selectedItems.at(i)));

        QApplication::clipboard()->setText(urls.join("\n"));
    }
}

void ArcStorageWindow::on_filesTreeWidget_itemClicked(QTreeWidgetItem *item, int column)
{
    QList<QTreeWidgetItem *> selectedItems = ui->filesTreeWidget->selectedItems();

    if (selectedItems.size() != 0)
    {
        if (m_filePropertyInspector != 0)
        {
            if (m_filePropertyInspector->isVisible())
            {
                QMap<QString, QString> propertyMap = m_currentFileServer->fileProperties(getURLOfItem(selectedItems.at(0)));
                m_filePropertyInspector->setProperties(propertyMap);
            }
        }
    }
}

void ArcStorageWindow::on_actionCopyURLFilename_triggered()
{
    qDebug() << "copyURL filename selected.";

    QList<QTreeWidgetItem *> selectedItems = ui->filesTreeWidget->selectedItems();

    if (selectedItems.size() != 0)
    {
        QTreeWidgetItem* item = selectedItems.at(0);
        QApplication::clipboard()->clear();

        QUrl url = getURLOfItem(item);

        QApplication::clipboard()->setText(url.path().split("/").last());
    }
}

void ArcStorageWindow::on_actionShowFileProperties_triggered()
{
    QList<QTreeWidgetItem *> selectedItems = ui->filesTreeWidget->selectedItems();

    if (selectedItems.size() != 0)
    {
        if (m_filePropertyInspector == 0)
            m_filePropertyInspector = new FilePropertyInspector(this);

        m_filePropertyInspector->show();
        QMap<QString, QString> propertyMap = m_currentFileServer->fileProperties(getURLOfItem(selectedItems.at(0)));
        m_filePropertyInspector->setProperties(propertyMap);
    }
}

void ArcStorageWindow::on_actionBack_triggered()
{
    QString url = this->popUrl();
    if (url!="")
        this->openUrl(url);
}

void ArcStorageWindow::on_actionHelpContents_triggered()
{
    if (!m_childWindow)
        ARCTools::instance()->showHelpWindow(this);
    else
        ARCTools::instance()->showHelpWindow(GlobalStateInfo::instance()->mainWindow());
}

void ArcStorageWindow::on_actionDownloadSelected_triggered()
{
    logger.msg(Arc::VERBOSE, "Downloading files to selected directory. (on_actionDownloadSelected_triggered)");

    QString dir = QFileDialog::getExistingDirectory(this, tr("Select destination directory"),
                                                    "/home", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    QString destDir = (QUrl::fromLocalFile(dir)).toString();

    QList<QTreeWidgetItem *> selectedItems = ui->filesTreeWidget->selectedItems();

    if (selectedItems.length()>0)
    {
        QList<QUrl> urlList;

        for (int i=0; i<selectedItems.length(); i++)
        {
            QTreeWidgetItem* item = selectedItems.at(i);
            if (item->text(2)=="file")
            {
                QVariant dataQV = item->data(0, Qt::ToolTipRole);
                QUrl url = QUrl::fromLocalFile(dataQV.toString());
                urlList.append(url);
            }
            else
            {
                QVariant dataQV = item->data(0, Qt::ToolTipRole);
                QUrl url = QUrl::fromLocalFile(dataQV.toString()+"/");
                urlList.append(url);
            }
        }

        GlobalStateInfo::instance()->showTransferWindow();

        this->setBusyUI(true);

        int i;

        FileTransferList::instance()->pauseProcessing();
        m_currentFileServer->copyToServer(urlList, destDir);
        FileTransferList::instance()->resumeProcessing();
    }
}

void ArcStorageWindow::on_actionUploadSelected_triggered()
{
    logger.msg(Arc::VERBOSE, "Uploading files to selected directory. (on_actionUploadSelected_triggered)");

    QStringList selectedFiles = QFileDialog::getOpenFileNames(this, "Select files to upload", "/home");

    if (selectedFiles.length()>0)
    {
        QList<QUrl> urlList;

        for (int i=0; i<selectedFiles.length(); i++)
        {
            QUrl url = QUrl::fromLocalFile(selectedFiles.at(i));
            urlList.append(url);
        }

        GlobalStateInfo::instance()->showTransferWindow();

        this->setBusyUI(true);

        int i;

        FileTransferList::instance()->pauseProcessing();
        m_currentFileServer->copyToServer(urlList, m_currentFileServer->getCurrentPath());
        FileTransferList::instance()->resumeProcessing();
    }
}

void ArcStorageWindow::on_actionUploadDirectory_triggered()
{
    logger.msg(Arc::VERBOSE, "Uploading directory to selected directory. (on_actionUploadSelected_triggered)");

    QString selectedDir = QFileDialog::getExistingDirectory(this, "Select directory to upload", "/home");

    if (selectedDir.length()>0)
    {
        QList<QUrl> urlList;

        QUrl url = QUrl::fromLocalFile(selectedDir+"/");
        urlList.append(url);

        GlobalStateInfo::instance()->showTransferWindow();

        this->setBusyUI(true);

        int i;

        FileTransferList::instance()->pauseProcessing();
        m_currentFileServer->copyToServer(urlList, m_currentFileServer->getCurrentPath());
        FileTransferList::instance()->resumeProcessing();
    }
}

void ArcStorageWindow::on_actionUploadDirAndArchive_triggered()
{
    logger.msg(Arc::VERBOSE, "Uploading directory to selected directory. (on_actionUploadSelected_triggered)");

    QString selectedDir = QFileDialog::getExistingDirectory(this, "Select directory to upload", "/home");

    QProcess process;
    process.start("ls -la");
    process.waitForFinished();

    /*
    if (selectedDir.length()>0)
    {
        QList<QUrl> urlList;

        QUrl url = QUrl::fromLocalFile(selectedDir+"/");
        urlList.append(url);

        GlobalStateInfo::instance()->showTransferWindow();

        this->setBusyUI(true);

        int i;

        FileTransferList::instance()->pauseProcessing();
        m_currentFileServer->copyToServer(urlList, m_currentFileServer->getCurrentPath());
        FileTransferList::instance()->resumeProcessing();
    }
    */
}
