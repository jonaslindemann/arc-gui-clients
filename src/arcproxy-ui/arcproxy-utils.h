#ifndef ARCPROXYUTILS_H
#define ARCPROXYUTILS_H

#include <string>

#include <QObject>
#include <QString>
#include <QList>

#include <arc/Logger.h>
#include <arc/UserConfig.h>

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
    VomsListEntry* at(int idx);
    int count();
};

class ArcProxyController : public QObject
{
    Q_OBJECT
private:

    std::string m_proxy_path;
    std::string m_cert_path;
    std::string m_key_path;
    std::string m_ca_dir;
    std::string m_vomses_path;
    std::list<std::string> m_vomslist;
    std::list<std::string> m_orderlist;
    std::string m_user_name; //user name to MyProxy server
    std::string m_retrievable_by_cert; //if use empty passphrase to myproxy server
    std::string m_myproxy_server; //url of MyProxy server
    std::string m_myproxy_command; //command to myproxy server
    std::list<std::string> m_constraintlist;
    std::string m_conffile;
    std::string m_debug;

    bool m_use_gsi_comm;
    bool m_use_gsi_proxy;
    bool m_info;
    bool m_remove_proxy;
    bool m_use_empty_passphrase; //if use empty passphrase to myproxy server
    int m_timeout;
    bool m_version;

    Arc::Logger logger;
    Arc::LogStream logCerr;

    QString m_passphrase;

    VomsList m_vomsList;
public:
    ArcProxyController();
    virtual ~ArcProxyController();

    int initialize();
    int printInformation();
    int generateProxy();
    int removeProxy();

    void setPassphrase(const QString& passphrase);
    void setValidityPeriod(int seconds);
    void setUseGSIProxy(bool flag);
    bool getUseGSIProxy();

    QString getIdentity();

    VomsList& vomsList();

/*
private Q_SLOTS:

Q_SIGNALS:
*/
};

#endif // ARCPROXYUTILS_H
