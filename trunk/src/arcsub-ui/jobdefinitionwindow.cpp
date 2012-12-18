#include "jobdefinitionwindow.h"
#include "ui_jobdefinitionwindow.h"

#include <QMessageBox>
#include <QProcess>
#include <QFileDialog>
#include <QInputDialog>

#include <sys/types.h>
#include <sys/stat.h>

#include <arc/Logger.h>
#include <arc/ArcLocation.h>

#include "utils.h"
#include "arcsubmitcontroller.h"
#include "arctools.h"

JobDefinitionWindow::JobDefinitionWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::JobDefinitionWindow),
    m_logStream(std::cerr)
{
    ui->setupUi(this);

    // Redirect standard output

    m_debugStream = new QDebugStream(std::cout, this);
    m_debugStream2 = new QDebugStream(std::cerr, this);

    ui->memoryEdit->setValidator(new QIntValidator(ui->memoryEdit));
    ui->notificationEmailEdit->setValidator(new QRegExpValidator(QRegExp(".*@.*"), this));

    m_logStream.setFormat(Arc::ShortFormat);
    Arc::Logger::getRootLogger().addDestination(m_logStream);
    Arc::Logger::getRootLogger().setThreshold(Arc::INFO);

    Arc::ArcLocation::Init("/usr");

    m_submitController = new ArcSubmitController();

    m_jobDefinition = new ShellScriptDefinition(this, "None");

    m_updatingTables = true;

    this->setData();

#ifdef __linux__
    ui->actionNewJobDefinition->setIcon(QIcon::fromTheme("document-new"));
    ui->actionOpenJobDefinition->setIcon(QIcon::fromTheme("document-open"));
    ui->actionSaveJobDefinition->setIcon(QIcon::fromTheme("document-save"));
    ui->actionSubmitJobDefinition->setIcon(QIcon::fromTheme("system-run"));
    ui->actionShowJobStatus->setIcon(QIcon::fromTheme("utilities-system-monitor"));
#endif

    QStringList defaultREs;

    defaultREs << "APPS/BIO/BLAST";
    defaultREs << "APPS/BIO/CLUSTALW";
    defaultREs << "APPS/BIO/HMMER";
    defaultREs << "APPS/BIO/MAFFT";
    defaultREs << "APPS/BIO/MUSCLE";
    defaultREs << "APPS/MATH/NUMPY";
    defaultREs << "APPS/MATH/OCTAVE";
    defaultREs << "APPS/STATISTICS/R";
    defaultREs << "ENV/ALIEN";
    defaultREs << "ENV/GCC";
    defaultREs << "ENV/INTEL-11.1";
    defaultREs << "ENV/JAVA/JRE";
    defaultREs << "ENV/JAVA/SDK";
    defaultREs << "ENV/LOCALDISK-18000";
    defaultREs << "ENV/MPI/OPENMPI";
    defaultREs << "ENV/MPI/OPENMPI";
    defaultREs << "ENV/MPI/OPENMPI/GCC";
    defaultREs << "ENV/NUMPY";
    defaultREs << "ENV/PROXY";
    defaultREs << "ENV/PYTHON-2.4";
    defaultREs << "ENV/PYTHON-2.5";
    defaultREs << "ENV/PYTHON-2.7";
    defaultREs << "ENV/PYTHON";
    defaultREs << "ENV/RUNTIME/PROXY";
    defaultREs << "ENV/SCIPY";

    ui->runtimeCombo->addItems(defaultREs);
    ui->runtimeCombo->setCurrentIndex(-1);

    connect(m_submitController, SIGNAL(onSubmissionStatus(int, int, QString)), this, SLOT(onSubmissionStatus(int, int, QString)));

}

JobDefinitionWindow::~JobDefinitionWindow()
{
    delete ui;
    delete m_debugStream;
    delete m_debugStream2;
    delete m_jobDefinition;
    delete m_submitController;
}

void JobDefinitionWindow::customEvent(QEvent * event)
{
    // When we get here, we've crossed the thread boundary and are now
    // executing in the Qt object's thread

    if(event->type() == DEBUG_STREAM_EVENT)
    {
        handleDebugStreamEvent(static_cast<DebugStreamEvent *>(event));
    }

    // use more else ifs to handle other custom events
}

void JobDefinitionWindow::handleDebugStreamEvent(const DebugStreamEvent *event)
{
    // Now you can safely do something with your Qt objects.
    // Access your custom data using event->getCustomData1() etc.

    ui->logText->append(event->getOutputText());
}

void JobDefinitionWindow::setData()
{
    m_updatingTables = true;

    ui->jobNameEdit->setText(m_jobDefinition->name());
    ui->walltimeEdit->setText(QString::number(m_jobDefinition->walltime()));
    ui->memoryEdit->setText(QString::number(m_jobDefinition->memory()));
    ui->paramSweepValue->setValue(m_jobDefinition->paramSize());
    ui->notificationEmailEdit->setText(m_jobDefinition->email());

    ui->inputFileTable->clear();
    ui->inputFileTable->setRowCount(m_jobDefinition->inputFileCount());
    ui->inputFileTable->setColumnCount(2);


    QStringList labels;
    labels << "Filename" << "Source URL";
    ui->inputFileTable->setHorizontalHeaderLabels(labels);
    ui->inputFileTable->horizontalHeader()->setResizeMode(0, QHeaderView::Interactive);
    ui->inputFileTable->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);
    ui->inputFileTable->verticalHeader()->hide();
    ui->inputFileTable->setShowGrid(true);
    ui->inputFileTable->setFrameStyle(QFrame::NoFrame);
    ui->inputFileTable->setSelectionBehavior(QTableWidget::SelectRows);
    //ui->inputFileTable->setSelectionMode(QAbstractItemView::MultiSelection);


    for (int i=0; i<m_jobDefinition->inputFileCount(); i++)
    {
        ui->inputFileTable->setItem(i, 0, new QTableWidgetItem(m_jobDefinition->inputFileAt(i)));
        ui->inputFileTable->setItem(i, 1, new QTableWidgetItem(m_jobDefinition->inputFileSourceAt(i)));
        //ui->inputFileTable->addItem(m_jobDefinition->inputFileAt(i));
    }

    ui->outputFileTable->clear();

    labels.clear();
    labels << "Filename" << "Target URL";
    ui->outputFileTable->setHorizontalHeaderLabels(labels);
    ui->outputFileTable->horizontalHeader()->setResizeMode(0, QHeaderView::Interactive);
    ui->outputFileTable->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);
    ui->outputFileTable->verticalHeader()->hide();
    ui->outputFileTable->setShowGrid(true);
    ui->outputFileTable->setFrameStyle(QFrame::NoFrame);
    ui->outputFileTable->setSelectionBehavior(QTableWidget::SelectRows);
    //ui->outputFileTable->setSelectionMode(QAbstractItemView::MultiSelection);

    ui->outputFileTable->setRowCount(m_jobDefinition->outputFileCount());

    for (int i=0; i<m_jobDefinition->outputFileCount(); i++)
    {
        ui->outputFileTable->setItem(i, 0, new QTableWidgetItem(m_jobDefinition->outputFileAt(i)));
        ui->outputFileTable->setItem(i, 1, new QTableWidgetItem(m_jobDefinition->outputFileTargetAt(i)));
        //ui->outputFileTable->addItem(m_jobDefinition->outputFileAt(i));
    }

    ui->descriptionText->clear();
    ui->descriptionText->setText(m_jobDefinition->xrslString());

    ui->runtimesList->clear();

    for (int i=0; i<m_jobDefinition->runtimeCount(); i++)
        ui->runtimesList->addItem(m_jobDefinition->runtimeAt(i));

    ui->scriptEditor->clear();
    ui->scriptEditor->append(m_jobDefinition->script());

    m_updatingTables = false;
}

void JobDefinitionWindow::getData()
{
    m_jobDefinition->setName(ui->jobNameEdit->text());
    m_jobDefinition->setWalltime(ui->walltimeEdit->text().toInt());
    m_jobDefinition->setMemory(ui->memoryEdit->text().toInt());
    m_jobDefinition->setParamSize(ui->paramSweepValue->value());
    m_jobDefinition->setEmail(ui->notificationEmailEdit->text());
    m_jobDefinition->setScript(ui->scriptEditor->document()->toPlainText());
}

void JobDefinitionWindow::onSubmissionStatus(int currentJobId, int totalJobs, QString text)
{
    QString message = text + " - " + QString::number(currentJobId)+"/"+QString::number(totalJobs);
    ui->statusBar->showMessage(message);
}


void JobDefinitionWindow::on_actionSaveJobDefinition_triggered()
{
    qDebug() << "printing job description...";

    // nordugrid:xrsl
    // egee:jdl
    // nordugrid:jsdl
    // emies:adl

    //m_jobDescription.Identification.JobName = "Hello";

    /*
    m_jobDescription.Identification.JobName = "myjob";
    m_jobDescription.DataStaging.InputFiles.push_back(Arc::InputFileType());
    m_jobDescription.DataStaging.InputFiles.push_back(Arc::InputFileType());
    m_jobDescription.DataStaging.InputFiles.push_back(Arc::InputFileType());
    m_jobDescription.Application.Executable.Path = "executable";
    m_jobDescription.Application.Executable.Argument.push_back("arg1");
    m_jobDescription.Application.Executable.Argument.push_back("arg2");
    m_jobDescription.Application.Executable.Argument.push_back("arg3");
    m_jobDescription.DataStaging.OutputFiles.push_back(Arc::OutputFileType());
    m_jobDescription.Resources.TotalWallTime.range = 2400;
    m_jobDescription.Resources.RunTimeEnvironment.add(Arc::Software("SOFTWARE/HELLOWORLD-1.0.0"), Arc::Software::GREATERTHAN);

    // Needed by the XRSLParser.
    std::ofstream f("executable", std::ifstream::trunc);
    f << "executable";
    f.close();

    //m_jobDescription.SaveToStream(std::cout, "userlong");

    std::string xrsl;
    m_jobDescription.UnParse(xrsl, "nordugrid:xrsl");
    std::cout << xrsl << std::endl;
    */

    this->getData();

    QString jobDefName = QInputDialog::getText(this, "Job definition", "Name", QLineEdit::Normal, m_jobDefinition->name());

    if (jobDefName.length()!=0)
    {
        m_jobDefinition->setName(jobDefName);
        this->setData();
    }
    else
    {
        QMessageBox::information(this, "Job definition", "No name given");
        return;
    }

    QString createDir = QFileDialog::getExistingDirectory();

    if (createDir.length()!=0)
        m_jobDefinition->save(createDir);

    this->setData();
}

void JobDefinitionWindow::on_scriptTab_currentChanged(QWidget *arg1)
{
    qDebug() << "currentChanged";

    this->getData();
    this->setData();
}

void JobDefinitionWindow::on_addInputFileButton_clicked()
{
    QStringList filenames = QFileDialog::getOpenFileNames(this, "Add input files");

    if (filenames.count()>0)
    {
        for (int i=0; i<filenames.count(); i++)
            m_jobDefinition->addInputFile(filenames[i]);

    }
    this->setData();
}

void JobDefinitionWindow::on_removeInputFileButton_clicked()
{
    int selectedItem = ui->inputFileTable->currentRow();
    m_jobDefinition->removeInputFile(selectedItem);
    this->setData();
}

void JobDefinitionWindow::on_clearInputFilesButton_clicked()
{
    m_jobDefinition->clearInputFiles();
    this->setData();
}

void JobDefinitionWindow::on_addOutputFileButton_clicked()
{
    QString outputFilename = QInputDialog::getText(this, "Output file", "Filename");

    if (outputFilename.length()!=0)
    {
        m_jobDefinition->addOutputFile(outputFilename);
        this->setData();
    }
}

void JobDefinitionWindow::on_removeOutpuFileButton_clicked()
{
    int selectedItem = ui->outputFileTable->currentRow();
    m_jobDefinition->removeOutputFile(selectedItem);
    this->setData();
}

void JobDefinitionWindow::on_clearOutputFileButton_clicked()
{
    m_jobDefinition->clearOutputFiles();
    this->setData();
}

void JobDefinitionWindow::on_actionOpenJobDefinition_triggered()
{
    QString jobDir = QFileDialog::getExistingDirectory(this, "Select job definition dir");

    if (jobDir.length()!=0)
    {
        m_jobDefinition->load(jobDir);
        this->setData();
    }
}

void JobDefinitionWindow::on_addRuntimeButton_clicked()
{
    m_jobDefinition->addRuntime(ui->runtimeCombo->currentText(), "");
    this->setData();
}

void JobDefinitionWindow::on_removeRuntimeButton_clicked()
{
    m_jobDefinition->removeRuntime(ui->runtimesList->currentRow());
    this->setData();
}

void JobDefinitionWindow::on_clearRuntimesButton_clicked()
{
    m_jobDefinition->clearRuntimes();
    this->setData();
}

void JobDefinitionWindow::on_addIdButton_clicked()
{
    ui->scriptEditor->insertPlainText("%1");
    ui->scriptEditor->setFocus();
}

void JobDefinitionWindow::on_addSizeButton_clicked()
{
    ui->scriptEditor->insertPlainText("%2");
    ui->scriptEditor->setFocus();
}

void JobDefinitionWindow::on_addJobNameButton_clicked()
{
    ui->scriptEditor->insertPlainText("%3");
    ui->scriptEditor->setFocus();
}

void JobDefinitionWindow::on_sampleScriptCombo_currentIndexChanged(int index)
{

}

void JobDefinitionWindow::on_sampleScriptCombo_activated(int index)
{

}

void JobDefinitionWindow::on_actionExit_triggered()
{
    this->close();
}

void JobDefinitionWindow::on_actionSubmitJobDefinition_triggered()
{
    QFile jobListFile(m_jobDefinition->jobDir() + QDir::separator() + m_jobDefinition->name() + ".xml");
    if (jobListFile.exists())
    {
        int result = QMessageBox::question(this, "Job submission", "Overwrite existing job list file?", QMessageBox::Yes, QMessageBox::No);

        if (result == QMessageBox::No)
            return;
    }
    for (int i=0; i<m_jobDefinition->paramSize(); i++)
        m_submitController->addJobDescription(m_jobDefinition->jobDescriptionParam(i));

    m_submitController->setJobListFilename(m_jobDefinition->jobDir() + QDir::separator() + m_jobDefinition->name() + ".xml");
    m_submitController->startSubmission();
}

void JobDefinitionWindow::on_actionShowJobStatus_triggered()
{
    QFile jobListFile(m_jobDefinition->jobDir() + QDir::separator() + m_jobDefinition->name() + ".xml");
    if (jobListFile.exists())
    {
        ARCTools::instance()->setJobListFile(m_jobDefinition->jobDir() + QDir::separator() + m_jobDefinition->name() + ".xml");
        ARCTools::instance()->jobManagerTool();
    }
}

void JobDefinitionWindow::on_singleInputMultipleOutputRadio_clicked()
{
    ui->paramSweepValue->setEnabled(true);
}

void JobDefinitionWindow::on_multipleInputMultipleOutputRadio_clicked()
{
    ui->paramSweepValue->setEnabled(false);
}

void JobDefinitionWindow::on_outputFileTable_itemChanged(QTableWidgetItem *item)
{
    if (!m_updatingTables)
    {
        qDebug() << "output file current item changed.";
        if (item->column()==0)
            m_jobDefinition->setOutputFileAt(item->row(), item->text());

        if (item->column()==1)
            m_jobDefinition->setOutputTargetAt(item->row(), item->text());
    }
}

void JobDefinitionWindow::on_inputFileTable_itemChanged(QTableWidgetItem *item)
{
    if (!m_updatingTables)
    {
        qDebug() << "input file current item changed.";
        if (item->column()==0)
            m_jobDefinition->setInputFileAt(item->row(), item->text());

        if (item->column()==1)
            m_jobDefinition->setInputSourceAt(item->row(), item->text());
    }
}

void JobDefinitionWindow::on_outputFileTable_currentItemChanged(QTableWidgetItem *current, QTableWidgetItem *previous)
{
}

void JobDefinitionWindow::on_inputFileTable_currentItemChanged(QTableWidgetItem *current, QTableWidgetItem *previous)
{
}
