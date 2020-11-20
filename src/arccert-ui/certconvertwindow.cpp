#include "certconvertwindow.h"
#include "ui_certconvertwindow.h"

#include <QMessageBox>
#include <QProcess>
#include <QFileDialog>
#include <QInputDialog>

#include <sys/types.h>
#include <sys/stat.h>

CertConvertWindow::CertConvertWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::CertConvertWindow)
{
    // Certificate Windows constructor

    ui->setupUi(this);

    // Redirect standard output

    m_debugStream = new QDebugStream(std::cout, this);
    m_debugStream2 = new QDebugStream(std::cerr, this);

    // Set default values

    QDir globusDir = QDir::homePath();
    globusDir.cd(".arc");

    QFileInfo userCertFile(globusDir.absolutePath()+"/"+"usercert.pem");
    if (userCertFile.exists())
        m_certificateFilename = userCertFile.absoluteFilePath();

    QFileInfo userKeyFile(globusDir.absolutePath()+"/"+"userkey.pem");
    if (userKeyFile.exists())
        m_keyFilename = userKeyFile.absoluteFilePath();

    m_pkcs12Filename = globusDir.absolutePath()+"/"+"usercert.p12";

    // Update user interface

    ui->usercertFileText->setText(m_certificateFilename);
    ui->userkeyFileText->setText(m_keyFilename);
    ui->pkcsFileText->setText(m_pkcs12Filename);
    ui->certKeyDirText->setText(globusDir.absolutePath());

}

CertConvertWindow::~CertConvertWindow()
{
    // Clean up after us

    delete ui;
    delete m_debugStream;
    delete m_debugStream2;
}

void CertConvertWindow::customEvent(QEvent * event)
{
    // When we get here, we've crossed the thread boundary and are now
    // executing in the Qt object's thread

    if(event->type() == DEBUG_STREAM_EVENT)
    {
        handleDebugStreamEvent(static_cast<DebugStreamEvent *>(event));
    }

    // use more else ifs to handle other custom events
}

void CertConvertWindow::handleDebugStreamEvent(const DebugStreamEvent *event)
{
    // Now you can safely do something with your Qt objects.
    // Access your custom data using event->getCustomData1() etc.

    ui->logText->append(event->getOutputText());
}

void CertConvertWindow::on_convertToPKCS12Button_clicked()
{
    // Check for existing files

    QFileInfo pkcs12FilenameInfo(m_pkcs12Filename);

    if (pkcs12FilenameInfo.exists())
    {
        int ret = QMessageBox::question(this, "Convert", m_pkcs12Filename+" already exists. Overwrite?", QMessageBox::Yes|QMessageBox::No);
        if (ret == QMessageBox::No)
            return;
    }

    bool ok;

    m_passin = "";
    m_passout = "";

    // Get private key passphrase

    m_passin = QInputDialog::getText(this, "Passphrase", "Private key passphrase", QLineEdit::Password, "", &ok);

    if (!ok)
        return;

    if (m_passin.isEmpty())
    {
        QMessageBox::warning(this, "Convert", "Empty private key passphrase given.");
        return;
    };

    // Get export passhprase

    m_passout = QInputDialog::getText(this, "Passphrase", "Export passphrase", QLineEdit::Password, "", &ok);

    if (!ok)
        return;

    if (m_passout.isEmpty())
    {
        QMessageBox::warning(this, "Convert", "Empty export passphrase given.");
        return;
    };

    // Construct command line

    QString opensslCmd = "openssl pkcs12 -export -in \"" + m_certificateFilename + "\" -inkey \"" + m_keyFilename + "\" -out \"" + m_pkcs12Filename + "\" -passout stdin -passin stdin";
    qDebug() << opensslCmd;

    // Execute OpenSSL command line

    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.start(opensslCmd);

    m_passin = m_passin + "\n";
    m_passout = m_passout + "\n";

    // Send passphrases as standard input

    p.write(m_passin.toAscii());
    p.write(m_passout.toAscii());
    p.waitForFinished();

    // Process output

    QString p_stdout = p.readAllStandardOutput();

    std::cout << "Exporting to PKCS12 format: " << p_stdout.toStdString() << std::endl;

    if (p.exitCode()==0)
        QMessageBox::information(this, "Convert", "Succesfully exported to PKCS12 format.");
    else
    {
        QMessageBox::information(this, "Convert", "Failed to export certificate to PKCS12 format.");
        return;
    }

    // Clear passphrases

    m_passin.clear();
    m_passout.clear();
}

void CertConvertWindow::on_selectCertFileButton_clicked()
{
    QDir globusDir = QDir::homePath();
    globusDir.cd(".arc");
    qDebug() << globusDir.absolutePath();

    m_certificateFilename = QFileDialog::getOpenFileName(this,
        tr("Open certificate"), globusDir.absolutePath(), tr("PEM Files (*.pem)"));

    ui->usercertFileText->setText(m_certificateFilename);
}

void CertConvertWindow::on_selectKeyButton_clicked()
{
    QDir globusDir = QDir::homePath();
    globusDir.cd(".arc");
    qDebug() << globusDir.absolutePath();

    m_keyFilename = QFileDialog::getOpenFileName(this,
        tr("Open private key"), globusDir.absolutePath(), tr("PEM Files (*.pem)"));

    ui->userkeyFileText->setText(m_keyFilename);
}

void CertConvertWindow::on_selectPKCS12FileButton_clicked()
{
    QDir globusDir = QDir::homePath();
    globusDir.cd(".arc");
    qDebug() << globusDir.absolutePath();

    QFileDialog dlg(this);
    dlg.setDefaultSuffix("p12");
    dlg.setNameFilter("*.p12");
    dlg.setDirectory(globusDir.absolutePath());
    dlg.setAcceptMode(QFileDialog::AcceptSave);

    if (dlg.exec())
    {
        ui->pkcsFileText->setText(dlg.selectedFiles()[0]);
        m_pkcs12Filename = dlg.selectedFiles()[0];
    }
}

void CertConvertWindow::on_selectPKCS12ImportFileButton_clicked()
{
    QDir globusDir = QDir::homePath();
    globusDir.cd(".arc");
    qDebug() << globusDir.absolutePath();

    m_pkcs12ImportFilename = QFileDialog::getOpenFileName(this,
        tr("Open PKCS12 file"), globusDir.absolutePath(), tr("PKCS12 Files (*.p12)"));

    ui->pkcsImportFileText->setText(m_pkcs12ImportFilename);
}

void CertConvertWindow::on_selectCertKeyOutputDirButton_clicked()
{
    QDir globusDir = QDir::homePath();
    globusDir.cd(".arc");
    qDebug() << globusDir.absolutePath();

    m_certOutputDir = QFileDialog::getExistingDirectory(this, "Convert", globusDir.absolutePath());

    ui->certKeyDirText->setText(m_certOutputDir);
}

void CertConvertWindow::on_convertToX509Button_clicked()
{
    bool ok;

    m_passin = "";
    m_passout = "";
    m_certOutputDir = ui->certKeyDirText->text();

    // Get private key passphrase

    m_passin = QInputDialog::getText(this, "Passphrase", "Import passphrase", QLineEdit::Password, "", &ok);

    if (!ok)
        return;

    if (m_passin.isEmpty())
    {
        QMessageBox::warning(this, "Convert", "Empty import passphrase given.");
        return;
    };

    // Get export passhprase

    m_passout = QInputDialog::getText(this, "Ppassphrase", "Private key passphrase", QLineEdit::Password, "", &ok);

    if (!ok)
        return;

    if (m_passout.isEmpty())
    {
        QMessageBox::warning(this, "Convert", "Empty private key passphrase given.");
        return;
    };

    // Construct command line

    // openssl pkcs12 -nocerts -in cert.p12 -out userkey.pem
    // openssl pkcs12 -clcerts -nokeys -in cert.p12 -out usercert.pem

    QString certFilename = m_certOutputDir + "/usercert.pem";
    QString keyFilename = m_certOutputDir + "/userkey.pem";

    // Check for existing files

    QFileInfo certFilenameInfo(certFilename);
    QFileInfo keyFilenameInfo(keyFilename);

    if (certFilenameInfo.exists())
    {
        int ret = QMessageBox::question(this, "Convert", "usercert.pem already exists. Overwrite?", QMessageBox::Yes|QMessageBox::No);
        if (ret == QMessageBox::No)
            return;
    }

    if (keyFilenameInfo.exists())
    {
        int ret = QMessageBox::question(this, "Convert", "userkey.pem already exists. Overwrite?", QMessageBox::Yes|QMessageBox::No);
        if (ret == QMessageBox::No)
            return;
    }

    QString opensslCmd1 = "openssl pkcs12 -nocerts -in \"" + m_pkcs12ImportFilename + "\" -out \"" + keyFilename + "\" -passin stdin -passout stdin";
    QString opensslCmd2 = "openssl pkcs12 -clcerts -nokeys -in \"" + m_pkcs12ImportFilename + "\" -out \"" + certFilename + "\" -passin stdin";

    qDebug() << opensslCmd1;
    qDebug() << opensslCmd2;

    QString p_stdout;

    m_passin = m_passin + "\n";
    m_passout = m_passout + "\n";

    // Execute OpenSSL command line

    QProcess p1;
    p1.setProcessChannelMode(QProcess::MergedChannels);
    p1.start(opensslCmd1);

    // Send passphrases as standard input

    p1.write(m_passin.toAscii());
    p1.write(m_passout.toAscii());
    p1.waitForFinished(-1);

    // Process output

    p_stdout = p1.readAllStandardOutput();

    if (p1.exitCode()==0)
        QMessageBox::information(this, "Convert", "Succesfully imported userkey.pem");
    else
    {
        QMessageBox::information(this, "Convert", "Failed to import userkey.pem.");
        return;
    }

    p1.close();

    std::cout << "Extracting private key: " << p_stdout.toStdString();

    // Execute OpenSSL command line 2

    QProcess p2;
    p2.setProcessChannelMode(QProcess::MergedChannels);
    p2.start(opensslCmd2);

    // Send passphrases as standard input

    p2.write(m_passin.toAscii());
    p2.waitForFinished(-1);

    // Process output

    p_stdout = p2.readAllStandardOutput();

    std::cout << "Extracting public key: " << p_stdout.toStdString();

    if (p2.exitCode()==0)
        QMessageBox::information(this, "Convert", "Succesfully imported usercert.pem");
    else
    {
        QMessageBox::information(this, "Convert", "Failed to import usercert.pem.");
        return;
    }


    p2.close();

    // Clear passphrases

    m_passin.clear();
    m_passout.clear();

    // Change permissions

    qDebug() << "chmod 400 '" + keyFilename + "'";
    chmod(keyFilename.toAscii(), 0400);

}
