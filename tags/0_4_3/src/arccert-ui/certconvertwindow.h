#ifndef CertConvertWindow_H
#define CertConvertWindow_H

#include <QMainWindow>
#include <QDebug>

#include "qdebugstream.h"

namespace Ui {
class CertConvertWindow;
}

class CertConvertWindow : public QMainWindow
{
    Q_OBJECT
private:
    QDebugStream* m_debugStream;
    QDebugStream* m_debugStream2;

    QString m_certificateFilename;
    QString m_keyFilename;
    QString m_pkcs12Filename;
    QString m_pkcs12ImportFilename;
    QString m_certOutputDir;
    QString m_passin;
    QString m_passout;

    void handleDebugStreamEvent(const DebugStreamEvent *event);

public:
    explicit CertConvertWindow(QWidget *parent = 0);
    ~CertConvertWindow();

protected:
    void customEvent(QEvent * event);
    
private Q_SLOTS:

    void on_convertToPKCS12Button_clicked();

    void on_selectCertFileButton_clicked();

    void on_selectKeyButton_clicked();

    void on_selectPKCS12FileButton_clicked();

    void on_selectPKCS12ImportFileButton_clicked();

    void on_selectCertKeyOutputDirButton_clicked();

    void on_convertToX509Button_clicked();

private:
    Ui::CertConvertWindow *ui;
};

#endif // CertConvertWindow_H
