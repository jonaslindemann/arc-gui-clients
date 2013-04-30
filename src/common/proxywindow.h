#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDialog>
#include <QDebug>
#include <QCloseEvent>
#include <QModelIndex>
#include <QListWidgetItem>

class HelpWindow;

class ArcProxyController;

namespace Ui {
class ProxyWindow;
}

class ProxyWindow : public QDialog
{
    Q_OBJECT
private:
    ArcProxyController* m_proxyController;

    HelpWindow* m_helpWindow;

    bool m_configTableDirty;

    void writeSettings();
    void readSettings();

public:
    explicit ProxyWindow(QWidget *parent = 0, ArcProxyController* m_proxyController = 0);
    ~ProxyWindow();
    
private Q_SLOTS:
    void on_generateButton_clicked();

    void on_removeButton_clicked();

    void on_proxyTypeCombo_currentIndexChanged(int index);

    void on_addVomsServer_clicked();

    void on_removeVomsServer_clicked();

    void on_addVomsServerConfig_clicked();

    void on_removeVomsServerConfig_clicked();

    void on_modifyVomsConfigItem_clicked();

    void on_vomsList_clicked(const QModelIndex &index);

    void on_vomsConfigTable_cellChanged(int row, int column);

    void on_helpButton_clicked();

    void on_NSSProfileList_itemClicked(QListWidgetItem *item);

    void on_tabWidget_currentChanged(int index);

private:
    Ui::ProxyWindow *ui;
};

#endif // MAINWINDOW_H
