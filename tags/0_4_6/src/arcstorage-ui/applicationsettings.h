#ifndef APPLICATIONSETTINGS_H
#define APPLICATIONSETTINGS_H

#include <QDialog>

namespace Ui {
class ApplicationSettings;
}

class ApplicationSettings : public QDialog
{
    Q_OBJECT
    
public:
    explicit ApplicationSettings(QWidget *parent = 0);
    ~ApplicationSettings();

protected:
    void showEvent(QShowEvent *e);
    
private Q_SLOTS:
    void on_okButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::ApplicationSettings *ui;
};

#endif // APPLICATIONSETTINGS_H
