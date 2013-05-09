#include "filepropertyinspector.h"
#include "ui_filepropertyinspector.h"

FilePropertyInspector::FilePropertyInspector(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FilePropertyInspector)
{
    ui->setupUi(this);
}

FilePropertyInspector::~FilePropertyInspector()
{
    delete ui;
}

void FilePropertyInspector::setProperties(QMap<QString,QString>& properties)
{
    ui->filePropertyTable->clear();
    ui->filePropertyTable->setRowCount(properties.count());
    QStringList headers;
    headers << "Property" << "Value";
    ui->filePropertyTable->setHorizontalHeaderLabels(headers);

    int row = 0;

    QMapIterator<QString, QString> i(properties);
    while (i.hasNext())
    {
        i.next();
        ui->filePropertyTable->setItem(row, 0, new QTableWidgetItem(i.key()));
        ui->filePropertyTable->setItem(row, 1, new QTableWidgetItem(i.value()));
        row++;
    }
}


void FilePropertyInspector::on_closeButton_clicked()
{
    this->close();
}
