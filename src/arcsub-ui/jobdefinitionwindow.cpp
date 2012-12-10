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

    m_jobDefinition = new JobDefinitionBase(this, "Test");

    this->setData();
}

JobDefinitionWindow::~JobDefinitionWindow()
{
    delete ui;
    delete m_debugStream;
    delete m_debugStream2;
    delete m_jobDefinition;
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
    ui->jobNameEdit->setText(m_jobDefinition->name());
    ui->walltimeEdit->setText(QString::number(m_jobDefinition->walltime()));
    ui->memoryEdit->setText(QString::number(m_jobDefinition->memory()));

    ui->inputFilesList->clear();

    for (int i=0; i<m_jobDefinition->inputFileCount(); i++)
        ui->inputFilesList->addItem(m_jobDefinition->inputFileAt(i));

    ui->outputFilesList->clear();

    for (int i=0; i<m_jobDefinition->outputFileCount(); i++)
        ui->outputFilesList->addItem(m_jobDefinition->outputFileAt(i));

    ui->descriptionText->clear();
    ui->descriptionText->setText(m_jobDefinition->xrslString());
}

void JobDefinitionWindow::getData()
{
    m_jobDefinition->setName(ui->jobNameEdit->text());
    m_jobDefinition->setWalltime(ui->walltimeEdit->text().toInt());
    m_jobDefinition->setMemory(ui->memoryEdit->text().toInt());
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
}

void JobDefinitionWindow::on_scriptTab_currentChanged(QWidget *arg1)
{
    qDebug() << "currentChanged";
    if (arg1 == ui->generalTab)
    {
        this->setData();
    }
    else
    {
        this->getData();
    }

    if (arg1 == ui->descriptionTab)
    {
        this->setData();
    }
}

void JobDefinitionWindow::on_addInputFileButton_clicked()
{
    QStringList filenames = QFileDialog::getOpenFileNames(this, "Add input files");

    if (filenames.count()>0)
    {
        for (int i=0; i<filenames.count(); i++)
            m_jobDefinition->addInputFile(filenames[i]);

        this->setData();
    }
}

void JobDefinitionWindow::on_removeInputFileButton_clicked()
{
    int selectedItem = ui->inputFilesList->currentRow();
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

}

void JobDefinitionWindow::on_removeOutpuFileButton_clicked()
{
    int selectedItem = ui->outputFilesList->currentRow();
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
