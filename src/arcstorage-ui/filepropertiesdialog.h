#ifndef FILEPROPERTIESDIALOG_H
#define FILEPROPERTIESDIALOG_H

#include <QDialog>

namespace Ui {
    class FilePermissionsDialog;
}

/**
  * This class is a QDialog for changing the permissions of a file. Currently
  * the standard QT permissions are supported so it only works with local
  * and ftp files. In order to get this in line with SRM files the best way
  * to go would probably be to make a more general "file properties" dialog
  */
class FilePermissionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FilePermissionsDialog(QWidget *parent = 0);
    ~FilePermissionsDialog();

    void setPermissions(unsigned int);
    unsigned int getPermissions();

private:
    Ui::FilePermissionsDialog *ui;
};

#endif // FILEPROPERTIESDIALOG_H
