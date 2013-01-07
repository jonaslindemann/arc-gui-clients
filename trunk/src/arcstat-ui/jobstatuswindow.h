#ifndef JobStatusWindow_H
#define JobStatusWindow_H

#include <QMainWindow>
#include "arcjobcontroller.h"
#include "qdebugstream.h"

namespace Ui {
class JobStatusWindow;
}

class JobStatusWindow : public QMainWindow
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
    explicit JobStatusWindow(QWidget *parent = 0);
    ~JobStatusWindow();

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

    void on_actionOpenJobList_triggered();

    void on_actionSelectAll_triggered();

    void on_actionClearSelection_triggered();

    void on_actionCleanSelected_triggered();

    void on_actionKillSelected_triggered();

    void on_actionDownloadSelected_triggered();

    void on_actionExit_triggered();

    void on_actionResubmitSelected_triggered();

    void on_actionRemoveJobList_triggered();

    void on_actionShowFiles_triggered();

private:
    Ui::JobStatusWindow *ui;
};

#endif // JobStatusWindow_H
