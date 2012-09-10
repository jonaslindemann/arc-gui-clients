#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QProcess>
#include <QFileDialog>
#include <QInputDialog>

#include <sys/types.h>
#include <sys/stat.h>

#include <arc/Logger.h>
#include <arc/ArcLocation.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
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
    m_jobDefinition->setExecutable("matlab");
    m_jobDefinition->addArgument("main.m");
    m_jobDefinition->setWalltime(3600);
    m_jobDefinition->setMemory(2000);
    m_jobDefinition->addInputFile("test1.dat");

    ui->jobNameEdit->setText(m_jobDefinition->getName());
    ui->walltimeEdit->setText(QString::number(m_jobDefinition->getWalltime()));
    ui->memoryEdit->setText(QString::number(m_jobDefinition->getMemory()));

}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_debugStream;
    delete m_debugStream2;
    delete m_jobDefinition;
}

void MainWindow::customEvent(QEvent * event)
{
    // When we get here, we've crossed the thread boundary and are now
    // executing in the Qt object's thread

    if(event->type() == DEBUG_STREAM_EVENT)
    {
        handleDebugStreamEvent(static_cast<DebugStreamEvent *>(event));
    }

    // use more else ifs to handle other custom events
}

void MainWindow::handleDebugStreamEvent(const DebugStreamEvent *event)
{
    // Now you can safely do something with your Qt objects.
    // Access your custom data using event->getCustomData1() etc.

    ui->logText->append(event->getOutputText());
}


void MainWindow::on_actionSaveJobDefinition_triggered()
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

    m_jobDefinition->print();
}

void MainWindow::on_scriptTab_currentChanged(QWidget *arg1)
{
    qDebug() << "currentChanged";
    if (arg1 == ui->generalTab)
    {
        ui->jobNameEdit->setText(m_jobDefinition->getName());
        ui->walltimeEdit->setText(QString::number(m_jobDefinition->getWalltime()));
        ui->memoryEdit->setText(QString::number(m_jobDefinition->getMemory()));
    }
    else
    {
        m_jobDefinition->setName(ui->jobNameEdit->text());
        m_jobDefinition->setWalltime(ui->walltimeEdit->text().toInt());
        m_jobDefinition->setMemory(ui->memoryEdit->text().toInt());
    }

    if (arg1 == ui->descriptionTab)
    {
        ui->descriptionText->clear();
        ui->descriptionText->setText(m_jobDefinition->xrslString());
    }
}
