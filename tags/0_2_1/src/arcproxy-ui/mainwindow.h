#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>

#include "arcproxy-utils.h"
#include "qdebugstream.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
private:
    ArcProxyController m_proxyController;
    QDebugStream* m_debugStream;
    QDebugStream* m_debugStream2;

    void handleDebugStreamEvent(const DebugStreamEvent *event);

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void customEvent(QEvent * event);
    
private Q_SLOTS:
    void on_generateButton_clicked();

    void on_removeButton_clicked();

    void on_passphraseText_returnPressed();

    void on_proxyTypeCombo_currentIndexChanged(int index);

    void on_infoButton_clicked();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
