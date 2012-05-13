#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>

#include "qdebugstream.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
private:
    QDebugStream* m_debugStream;
    QDebugStream* m_debugStream2;

    void handleDebugStreamEvent(const DebugStreamEvent *event);

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void customEvent(QEvent * event);
    
private Q_SLOTS:

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
