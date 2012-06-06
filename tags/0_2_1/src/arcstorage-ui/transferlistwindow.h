#ifndef TRANSFERLISTWINDOW_H
#define TRANSFERLISTWINDOW_H

#include <QMainWindow>
#include <QHash>

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

    QHash<QString, int> m_idToRowDict;

public Q_SLOTS:
    void onUpdateStatus(QString id);
    void onAddTransfer(QString id);
    void onRemoveTransfer(QString id);

};

#endif // TRANSFERLISTWINDOW_H
