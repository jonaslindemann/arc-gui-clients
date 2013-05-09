#ifndef FILEPROPERYINSPECTOR_H
#define FILEPROPERYINSPECTOR_H

#include <QDialog>
#include <QMap>

namespace Ui {
class FilePropertyInspector;
}

class FilePropertyInspector : public QDialog
{
    Q_OBJECT
public:
    explicit FilePropertyInspector(QWidget *parent = 0);
    ~FilePropertyInspector();

    void setProperties(QMap<QString,QString>& properties);
    
private Q_SLOTS:
    void on_closeButton_clicked();

private:
    Ui::FilePropertyInspector *ui;
};

#endif // FILEPROPERYINSPECTOR_H
