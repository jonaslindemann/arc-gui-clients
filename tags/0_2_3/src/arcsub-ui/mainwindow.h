#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <arc/client/JobDescription.h>

#include "qdebugstream.h"

#include "jobdefinitions.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
private:
    QDebugStream* m_debugStream;
    QDebugStream* m_debugStream2;

    Arc::JobDescription m_jobDescription;
    Arc::LogStream m_logStream;

    JobDefinitionBase* m_jobDefinition;

    void handleDebugStreamEvent(const DebugStreamEvent *event);

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void customEvent(QEvent * event);
    
private Q_SLOTS:

    void on_actionSaveJobDefinition_triggered();

    void on_scriptTab_currentChanged(QWidget *arg1);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
