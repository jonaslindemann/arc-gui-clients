#ifndef JobDefinitionWindow_H
#define JobDefinitionWindow_H

#include <QMainWindow>
#include <QDebug>
#include <QTableWidget>

#include "arc-gui-config.h"

#if ARC_VERSION_MAJOR >= 3
#include <arc/compute/JobDescription.h>
#else
#include <arc/client/JobDescription.h>
#endif

#include "qdebugstream.h"

#include "jobdefinitions.h"
#include "arcsubmitcontroller.h"

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

    ArcSubmitController* m_submitController;

    int m_currentParam;

    bool m_updatingTables;

    void handleDebugStreamEvent(const DebugStreamEvent *event);

    void setData();
    void getData();

public:
    explicit JobDefinitionWindow(QWidget *parent = 0);
    ~JobDefinitionWindow();

protected:
    void customEvent(QEvent * event);
    
private Q_SLOTS:

    void onSubmissionStatus(int currentJobId, int totalJobs, QString text);

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

    void on_actionShowJobStatus_triggered();

    void on_singleInputMultipleOutputRadio_clicked();

    void on_multipleInputMultipleOutputRadio_clicked();

    void on_outputFileTable_itemChanged(QTableWidgetItem *item);

    void on_inputFileTable_itemChanged(QTableWidgetItem *item);

    void on_outputFileTable_currentItemChanged(QTableWidgetItem *current, QTableWidgetItem *previous);

    void on_inputFileTable_currentItemChanged(QTableWidgetItem *current, QTableWidgetItem *previous);

    void on_scriptParamSpin_valueChanged(int arg1);

    void on_addPerJobFileButton_clicked();

    void on_removePerJobFileButton_clicked();

    void on_clearPerJobFileButton_clicked();

    void on_perJobFileTable_itemChanged(QTableWidgetItem *item);

    void on_addPerFileButton_clicked();

    void on_adPerJobUrlButton_clicked();

    void on_addInputUrlButton_clicked();

    void on_addPerJobRowButton_clicked();

    void on_addInputFileRowButton_clicked();

    void on_addOutputFileRowButton_clicked();

    void on_pastePerJobFileButton_clicked();

    void on_pasteInputURLButton_clicked();

private:
    Ui::JobDefinitionWindow *ui;
};

#endif // JobDefinitionWindow_H
