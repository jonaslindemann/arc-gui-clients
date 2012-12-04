#ifndef ARCPROXYUTILS_H
#define ARCPROXYUTILS_H

#include <string>

#include <QObject>
#include <QString>
#include <QList>

#include <arc/Logger.h>
#include <arc/UserConfig.h>

class ProxyWindow;
class QApplication;

#define ARC_VERSION_2 1

class VomsListEntry : public QObject
{
private:
    QString m_alias;
    QString m_machine;
    QString m_port;
    QString m_hostDn;
    QString m_officialName;
public:
    VomsListEntry();

    void setAlias(QString alias);
    QString alias();

    void setMachine(QString machine);
    QString machine();

    void setPort(QString port);
    QString port();

    void setHostDN(QString dn);
    QString hostDN();

    QString officialName();
};

class VomsList : public QObject
{
private:
    QList<VomsListEntry*> m_vomsList;
public:
    VomsList();
    virtual ~VomsList();
    void clear();
    bool read();
    bool write();
    VomsListEntry* at(int idx);
    int count();
};

class ArcProxyController : public QObject
{
    Q_OBJECT
public:
    enum TCertStatus { CS_PATH_EMPTY, CS_NOT_FOUND, CS_INVALID_CONFIG, CS_CADIR_NOT_FOUND, CS_VALID };
    enum TProxyStatus { PS_PATH_EMPTY, PS_NOT_FOUND, PS_EXPIRED, PS_NOT_VALID, PS_VALID };
    enum TReturnStatus { RS_OK, RS_FAILED };
private:

    std::string proxy_path;
    std::string cert_path;
    std::string key_path;
    std::string ca_dir;
    std::string vomses_path;
    std::string voms_dir;
    std::list<std::string> vomslist;
    std::list<std::string> orderlist;
    std::string user_name; //user name to MyProxy server
    std::string retrievable_by_cert; //if use empty passphrase to myproxy server
    std::string myproxy_server; //url of MyProxy server
    std::string myproxy_command; //command to myproxy server
    std::list<std::string> constraintlist;
    std::string conffile;
    std::string debug;

    bool use_gsi_comm;
    bool use_gsi_proxy;
    bool info;
    bool remove_proxy;
    bool use_empty_passphrase; //if use empty passphrase to myproxy server
    int timeout;
    bool version;
    bool use_http_comm;
    bool use_nssdb;

    Arc::Logger logger;
    Arc::LogStream logCerr;
    Arc::User user;

    QString m_passphrase;

    VomsList m_vomsList;

    ProxyWindow* m_proxyWindow;
    QApplication* m_application;

    TReturnStatus m_uiReturnStatus;
public:
    ArcProxyController();
    virtual ~ArcProxyController();

    int initialize();
    int printInformation();
    int generateProxy();
    int removeProxy();

    void setUiReturnStatus(TReturnStatus status);
    TReturnStatus getUiReturnStatus();

    TReturnStatus showProxyUI();

    void showProxyUIAppLoop();

    TCertStatus checkCert();
    TProxyStatus checkProxy();

    void setPassphrase(const QString& passphrase);
    void setValidityPeriod(int seconds);
    void setUseGSIProxy(bool flag);
    bool getUseGSIProxy();

    void addVomsServer(const QString& server, const QString& role);

    QString getIdentity();

    VomsList& vomsList();

    /*
    private Q_SLOTS:

    Q_SIGNALS:
    */
};

#endif // ARCPROXYUTILS_H
