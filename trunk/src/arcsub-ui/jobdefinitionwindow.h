#ifndef JobDefinitionWindow_H
#define JobDefinitionWindow_H

#include <QMainWindow>
#include <QDebug>
#include <arc/client/JobDescription.h>

#include "qdebugstream.h"

#include "jobdefinitions.h"

namespace Ui {
class JobDefinitionWindow;
}

class JobDefinitionWindow : public QMainWindow
{
    Q_OBJECT
private:
    QDebugStream* m_debugStream;
    QDebugStream* m_debugStream2;

    Arc::JobDescription m_jobDescription;
    Arc::LogStream m_logStream;

    ShellScriptDefinition* m_jobDefinition;

    void handleDebugStreamEvent(const DebugStreamEvent *event);

    void setData();
    void getData();

public:
    explicit JobDefinitionWindow(QWidget *parent = 0);
    ~JobDefinitionWindow();

protected:
    void customEvent(QEvent * event);
    
private Q_SLOTS:

    void on_actionSaveJobDefinition_triggered();

    void on_scriptTab_currentChanged(QWidget *arg1);

    void on_addInputFileButton_clicked();

    void on_removeInputFileButton_clicked();

    void on_clearInputFilesButton_clicked();

    void on_addOutputFileButton_clicked();

    void on_removeOutpuFileButton_clicked();

    void on_clearOutputFileButton_clicked();

    void on_actionOpenJobDefinition_triggered();

    void on_addRuntimeButton_clicked();

    void on_removeRuntimeButton_clicked();

    void on_clearRuntimesButton_clicked();

    void on_addIdButton_clicked();

    void on_addSizeButton_clicked();

    void on_addJobNameButton_clicked();

    void on_sampleScriptCombo_currentIndexChanged(int index);

    void on_sampleScriptCombo_activated(int index);

    void on_actionExit_triggered();

    void on_actionSubmitJobDefinition_triggered();

private:
    Ui::JobDefinitionWindow *ui;
};

#endif // JobDefinitionWindow_H
