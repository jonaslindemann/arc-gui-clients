// -*- indent-tabs-mode: nil -*-

#ifndef __arcproxy_manager_h__
#define __arcproxy_manager_h__

#define VERSION "5.x_LUNARC"
#define USE_GUI
#define HAVE_NSS

#include "arcproxy.h"

#ifdef USE_GUI
#include <QDir>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QtGui/QApplication>
#include <QStyle>
#include <QDesktopWidget>
#include <QInputDialog>
#endif

using namespace ArcCredential;

class PasswordSourceFile: public Arc::PasswordSource {
private:
    std::ifstream file_;
public:
    PasswordSourceFile(const std::string& filename):file_(filename.c_str()) {
    };
    virtual Result Get(std::string& password, int minsize, int maxsize) {
        if(!file_) return Arc::PasswordSource::NO_PASSWORD;
        std::getline(file_, password);
        return Arc::PasswordSource::PASSWORD;
    }
};

#ifdef USE_GUI
class PasswordSourceDialog: public Arc::PasswordSource {
private:
    std::string m_prompt;
    bool m_verify;
public:
    PasswordSourceDialog(const std::string& prompt, bool verify)
    {
        m_prompt = prompt;
        m_verify = verify;
    }

    virtual Result Get(std::string& password, int minsize, int maxsize)
    {
        bool ok;
        QString prompt;

        prompt = m_prompt.c_str();

        QString text = QInputDialog::getText(0, "Password prompt", prompt, QLineEdit::Password, "", &ok);
        if (ok && !text.isEmpty())
        {
            password = text.toStdString();
            return Arc::PasswordSource::PASSWORD;
        }
        else
            return Arc::PasswordSource::NO_PASSWORD;
    }
};
#endif


class ArcProxyManager {
private:

#ifdef USE_GUI
    QApplication m_qtApp;
#endif

    Arc::Logger logger;
    Arc::LogStream logcerr;
    Arc::User m_user;

    std::string m_proxy_path;
    std::string m_cert_path;
    std::string m_key_path;
    std::string m_ca_dir;
    std::string m_voms_dir;
    std::string m_vomses_path;
    std::list<std::string> m_vomslist;
    std::list<std::string> m_orderlist;
    bool m_use_gsi_comm;
    bool m_use_http_comm;
    bool m_use_gsi_proxy;
    bool m_info;
    std::list<std::string> m_infoitemlist;
    bool m_remove_proxy;
    std::string m_user_name; //user name to MyProxy server
    bool m_use_empty_passphrase; //if use empty passphrase to myproxy server
    std::string m_retrievable_by_cert; //if use empty passphrase to myproxy server
    std::string m_myproxy_server; //url of MyProxy server
    std::string m_myproxy_command; //command to myproxy server
    bool m_use_nssdb;
    std::list<std::string> m_constraintlist;
    std::list<std::string> m_passsourcelist;
    int m_timeout;
    std::string m_conffile;
    std::string m_debug;
    bool m_version;
    std::map<std::string,std::list<std::string> > m_vomscmdlist;

public:
    ArcProxyManager(int &argc, char **argv);

    std::string proxyPath();
    std::string certPath();
    std::string keyPath();
    std::string caDir();
    std::string vomsDir();
    std::string vomsesPath();
    bool useGsiComm();
    bool useHttpComm();
    bool useGsiProxy();

    bool showInfoOpt();
    bool removeProxyOpt();
    bool showVersionOpt();

    std::string userName(); //user name to MyProxy server
    bool useEmptyPassphrase(); //if use empty passphrase to myproxy server
    std::string retrievableByCert(); //if use empty passphrase to myproxy server
    std::string myproxyServer(); //url of MyProxy server
    std::string myproxyCommand(); //command to myproxy server

    bool useNssDB();
    int timeout();
    std::string conffile();
    std::string debugLevel();

    //std::list<std::string> m_vomslist;
    //std::list<std::string> m_orderlist;
    //std::list<std::string> m_infoitemlist;
    //std::list<std::string> m_constraintlist;
    //std::list<std::string> m_passsourcelist;
    //std::map<std::string,std::list<std::string> > m_vomscmdlist;

    bool init();
    int removeProxy();
    int showInfo();
    int showInfoList();
    int createProxy();

    bool hasValidProxy();
};

#endif
