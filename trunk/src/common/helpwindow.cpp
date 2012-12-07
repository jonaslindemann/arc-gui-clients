#include "helpwindow.h"
#include "ui_helpwindow.h"

#include <QFile>
#include <QTextStream>
#include <QMessageBox>

HelpWindow::HelpWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::HelpWindow)
{
    ui->setupUi(this);

    /*
    QFile docFile("/home/jonas/wxGlade-0.6.5/docs/index.html");
    if(!docFile.open(QIODevice::ReadOnly)) {
        QMessageBox::information(0, "error", docFile.errorString());
    }

    QTextStream in(&docFile);

    while (!in.atEnd()) {
        QString line = in.readLine();
        ui->textEdit->append(line);
    }

    docFile.close();
    */

    ui->textBrowser->setSource(QUrl("/home/jonas/wxGlade-0.6.5/docs/index.html"));
}

HelpWindow::~HelpWindow()
{
    delete ui;
}

void HelpWindow::on_actionBack_triggered()
{

}

void HelpWindow::on_actionForward_triggered()
{

}

void HelpWindow::on_actionHome_triggered()
{

}
