#include "helpwindow.h"
#include "ui_helpwindow.h"

#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDebug>

HelpWindow::HelpWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::HelpWindow)
{
    ui->setupUi(this);
    ui->actionForward->setIcon(QIcon::fromTheme("forward"));
    ui->actionBack->setIcon(QIcon::fromTheme("back"));
    ui->actionHome->setIcon(QIcon::fromTheme("go-home"));
    ui->actionStop->setIcon(QIcon::fromTheme("stop"));
    ui->webView->load(QUrl("http://arc-gui-clients.sourceforge.net/docs/"));
}

HelpWindow::~HelpWindow()
{
    delete ui;
}

void HelpWindow::on_actionBack_triggered()
{
    ui->webView->back();
}

void HelpWindow::on_actionForward_triggered()
{
    ui->webView->forward();
}

void HelpWindow::on_actionHome_triggered()
{
    ui->webView->load(QUrl("http://arc-gui-clients.sourceforge.net/docs/"));
}

void HelpWindow::on_webView_loadStarted()
{
    ui->actionBack->setDisabled(true);
    ui->actionForward->setDisabled(true);
    ui->actionHome->setDisabled(true);
    ui->actionStop->setEnabled(true);
}

void HelpWindow::on_webView_loadProgress(int progress)
{
    ui->statusbar->showMessage("Loading "+QString::number(progress)+"%");
}

void HelpWindow::on_webView_loadFinished(bool arg1)
{
    ui->actionBack->setDisabled(false);
    ui->actionForward->setDisabled(false);
    ui->actionHome->setDisabled(false);
    ui->actionStop->setEnabled(false);
    ui->statusbar->clearMessage();
}

void HelpWindow::on_webView_statusBarMessage(const QString &text)
{
    ui->statusbar->clearMessage();
    ui->statusbar->showMessage(text);
}

void HelpWindow::on_actionStop_triggered()
{
    ui->webView->stop();
}
