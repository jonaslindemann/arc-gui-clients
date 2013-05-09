#ifndef STORAGESPLASH_H
#define STORAGESPLASH_H

#include <QDialog>

namespace Ui {
class StorageSplash;
}

class StorageSplash : public QDialog
{
    Q_OBJECT
    
public:
    explicit StorageSplash(QWidget *parent = 0);
    ~StorageSplash();
    
private:
    Ui::StorageSplash *ui;
};

#endif // STORAGESPLASH_H
