#ifndef SRMSETTINGSDIALOG_H
#define SRMSETTINGSDIALOG_H

#include <QDialog>
#include <qabstractbutton.h>

namespace Ui {
    class SRMSettingsDialog;
}

class SRMSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SRMSettingsDialog(QWidget *parent = 0);
    ~SRMSettingsDialog();
    QString getConfigFilename();
    void setConfigFilename(QString s);

private Q_SLOTS:
    void on_buttonBox_clicked(QAbstractButton* button);

private:
    Ui::SRMSettingsDialog *ui;
    QString configFilename;
};

#endif // SRMSETTINGSDIALOG_H
