#ifndef RENAMEDILAOG_H
#define RENAMEDILAOG_H

#include <QDialog>

namespace Ui {
class RenameDialog;
}

class RenameDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit RenameDialog(QWidget *parent = 0);
    ~RenameDialog();
    
private:
    Ui::RenameDialog *ui;
};

#endif // RENAMEDILAOG_H
