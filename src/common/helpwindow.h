#ifndef HELPWINDOW_H
#define HELPWINDOW_H

#include <QMainWindow>

namespace Ui {
class HelpWindow;
}

class HelpWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit HelpWindow(QWidget *parent = 0);
    ~HelpWindow();
    
private Q_SLOTS:
    void on_actionBack_triggered();

    void on_actionForward_triggered();

    void on_actionHome_triggered();

    void on_webView_loadStarted();

    void on_webView_loadProgress(int progress);

    void on_webView_loadFinished(bool arg1);

    void on_webView_statusBarMessage(const QString &text);

    void on_actionStop_triggered();

private:
    Ui::HelpWindow *ui;
};

#endif // HELPWINDOW_H
