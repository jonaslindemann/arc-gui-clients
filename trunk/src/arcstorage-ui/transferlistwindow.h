#ifndef TRANSFERLISTWINDOW_H
#define TRANSFERLISTWINDOW_H

#include <QMainWindow>

namespace Ui {
class TransferListWindow;
}

class TransferListWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit TransferListWindow(QWidget *parent = 0);
    ~TransferListWindow();
    
private:
    Ui::TransferListWindow *ui;

public Q_SLOTS:
    void onUpdateStatus(QString id);
    void onAddTransfer(QString id);
    void onRemoveTransfer(QString id);

};

#endif // TRANSFERLISTWINDOW_H
