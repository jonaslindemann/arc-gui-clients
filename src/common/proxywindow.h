#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>

#include "arcproxy-utils.h"
#include "qdebugstream.h"

namespace Ui {
class ProxyWindow;
}

class ProxyWindow : public QMainWindow
{
    Q_OBJECT
private:
    ArcProxyController m_proxyController;
    QDebugStream* m_debugStream;
    QDebugStream* m_debugStream2;

    void handleDebugStreamEvent(const DebugStreamEvent *event);

public:
    explicit ProxyWindow(QWidget *parent = 0);
    ~ProxyWindow();

protected:
    void customEvent(QEvent * event);
    
private Q_SLOTS:
    void on_generateButton_clicked();

    void on_removeButton_clicked();

    void on_passphraseText_returnPressed();

    void on_proxyTypeCombo_currentIndexChanged(int index);

    void on_infoButton_clicked();

private:
    Ui::ProxyWindow *ui;
};

#endif // MAINWINDOW_H
