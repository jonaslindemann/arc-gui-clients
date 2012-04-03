#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "arcjobcontroller.h"
#include "qdebugstream.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    ArcJobController* m_jobController;
    bool m_firstShow;

    QDebugStream* m_debugStream;
    QDebugStream* m_debugStream2;

    void disableActions();
    void enableActions();

    void handleDebugStreamEvent(const DebugStreamEvent *event);

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void showEvent ( QShowEvent * event );

protected:
    void customEvent(QEvent * event);
    
private Q_SLOTS:
    void on_actionRefresh_triggered();
    void onQueryJobStatusDone();
    void onQueryAllJobListStatusDone();
    void onDownloadJobsDone();
    void onKillJobsDone();
    void onCleanJobsDone();
    void onResubmitJobsDone();

    void on_tabWidget_currentChanged(int index);

    void on_actionOpenJobList_triggered();

    void on_actionSelectAll_triggered();

    void on_actionClearSelection_triggered();

    void on_actionCleanSelected_triggered();

    void on_actionKillSelected_triggered();

    void on_actionDownloadSelected_triggered();

    void on_actionExit_triggered();

    void on_actionResubmitSelected_triggered();

    void on_actionRemoveJobList_triggered();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
