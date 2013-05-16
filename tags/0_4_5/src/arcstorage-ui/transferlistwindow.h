#ifndef TRANSFERLISTWINDOW_H
#define TRANSFERLISTWINDOW_H

#include <QMainWindow>
#include <QHash>
#include <QMutex>

namespace Ui {
class TransferListWindow;
}

class TransferStatusDisplay : public QWidget
{
    Q_OBJECT
private:
    unsigned long m_transferred, m_totalSize;

public:
    TransferStatusDisplay(QWidget *parent = 0);

    void setStatus(unsigned long transferred, unsigned long totalSize);

public Q_SLOTS:

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
};

class TransferListWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit TransferListWindow(QWidget *parent = 0);
    ~TransferListWindow();
    
private:
    Ui::TransferListWindow *ui;

    QHash<QString, int> m_idToRowDict;
    QMutex m_accessLock;

public Q_SLOTS:
    void onUpdateStatus(QString id);
    void onAddTransfer(QString id);
    void onRemoveTransfer(QString id);

    void onCancelButtonClick();
};

#endif // TRANSFERLISTWINDOW_H
