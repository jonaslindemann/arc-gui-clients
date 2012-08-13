#include "arcproxy-utils.h"

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <stdexcept>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glibmm/stringutils.h>
#include <glibmm/fileutils.h>
#include <glibmm.h>
#include <unistd.h>

#include <arc/ArcLocation.h>
#include <arc/DateTime.h>
#include <arc/delegation/DelegationInterface.h>
#include <arc/OptionParser.h>
#include <arc/StringConv.h>
#include <arc/User.h>
#include <arc/Utils.h>
#include <arc/UserConfig.h>
#include <arc/client/ClientInterface.h>
#include <arc/credential/VOMSAttribute.h>
#include <arc/credential/VOMSUtil.h>
#include <arc/credential/Credential.h>
#include <arc/credential/CertUtil.h>
#include <arc/credentialstore/CredentialStore.h>
#include <arc/crypto/OpenSSL.h>
#include <arc/FileUtils.h>

#include <openssl/ui.h>

#include <QDir>
#include <QDebug>
#include <QFileInfo>
#include <QMessageBox>
#include <QtGui/QApplication>
#include <QStyle>
#include <QDesktopWidget>


#include "proxywindow.h"

#undef HAVE_NSS

#ifdef HAVE_NSS
#include <arc/credential/NSSUtil.h>
#endif

VomsListEntry::VomsListEntry()
{
    m_alias = "";
    m_machine = "";
    m_port = "";
    m_hostDn = "";
    m_officialName = "";
}

void VomsListEntry::setAlias(QString alias)
{
    m_alias = alias;
}

QString VomsListEntry::alias()
{
    return m_alias;
}

void VomsListEntry::setMachine(QString machine)
{
    m_machine = machine;
}

QString VomsListEntry::machine()
{
    return m_machine;
}

void VomsListEntry::setPort(QString port)
{
    m_port = port;
}

QString VomsListEntry::port()
{
    return m_port;
}

void VomsListEntry::setHostDN(QString dn)
{
    m_hostDn = dn;
}

QString VomsListEntry::hostDN()
{
    return m_hostDn;
}

QString VomsListEntry::officialName()
{
    return m_alias;
}

VomsList::VomsList()
{
    this->read();
}

VomsList::~VomsList()
{
    this->clear();
}

void VomsList::clear()
{
    int i;

    for (i=0; i<this->count(); i++)
        delete this->at(i);

    m_vomsList.clear();
}

bool VomsList::read()
{
    // ~/.arc/vomses, ~/.voms/vomses, $ARC_LOCATION/etc/vomses, $ARC_LOCATION/etc/grid-security/vomses, $PWD/vomses, /etc/vomses, /etc/grid-security/vomses

    QStringList vomsPaths;

    vomsPaths << QDir::homePath() + "/.arc/vomses";
    vomsPaths << QDir::homePath() + "/.voms/vomses";
    vomsPaths << "/etc/vomses";
    vomsPaths << "/etc/grid-security/vomses";

    this->clear();

    int i, j;

    for (i=0; i<vomsPaths.count(); i++)
    {
        if (QFileInfo(vomsPaths[i]).exists())
        {
            QFile vomsFile(vomsPaths[i]);
            if(!vomsFile.open(QIODevice::ReadOnly)) {
                QMessageBox::information(0, "error", vomsFile.errorString());
            }

            QTextStream in(&vomsFile);

            while(!in.atEnd()) {
                QString line = in.readLine();
                QStringList fields = line.split(" ");

                QStringList strippedFields;

                for (j=0; j<fields.count(); j++)
                {
                    strippedFields.append(fields[j].replace("\"", ""));
                }

                qDebug() << strippedFields;

                VomsListEntry* entry = new VomsListEntry();
                entry->setAlias(strippedFields[0]);
                entry->setMachine(strippedFields[1]);
                entry->setPort(strippedFields[2]);
                entry->setHostDN(strippedFields[3]);

                m_vomsList.append(entry);
            }

            vomsFile.close();
        }
    }
}

VomsListEntry* VomsList::at(int idx)
{
    if ((idx>=0)&&(idx<m_vomsList.count()))
        return m_vomsList.at(idx);
    else
        return 0;
}

int VomsList::count()
{
    return m_vomsList.count();
}


using namespace ArcCredential;

static int create_proxy_file(const std::string& path) {
    int f = -1;

    if((::unlink(path.c_str()) != 0) && (errno != ENOENT)) {
        throw std::runtime_error("Failed to remove proxy file " + path);
    }
    f = ::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_TRUNC, S_IRUSR | S_IWUSR);
    if (f == -1) {
        throw std::runtime_error("Failed to create proxy file " + path);
    }
    if(::chmod(path.c_str(), S_IRUSR | S_IWUSR) != 0) {
        ::unlink(path.c_str());
        ::close(f);
        throw std::runtime_error("Failed to change permissions of proxy file " + path);
    }
    return f;
}

static void write_proxy_file(const std::string& path, const std::string& content) {
    std::string::size_type off = 0;
    int f = create_proxy_file(path);
    while(off < content.length()) {
        ssize_t l = ::write(f, content.c_str(), content.length()-off);
        if(l < 0) {
            ::unlink(path.c_str());
            ::close(f);
            throw std::runtime_error("Failed to write into proxy file " + path);
        }
        off += (std::string::size_type)l;
    }
    ::close(f);
}

static void remove_proxy_file(const std::string& path) {
    if((::unlink(path.c_str()) != 0) && (errno != ENOENT)) {
        throw std::runtime_error("Failed to remove proxy file " + path);
    }
}

static void tls_process_error(Arc::Logger& logger) {
    unsigned long err;
    err = ERR_get_error();
    if (err != 0) {
        logger.msg(Arc::ERROR, "OpenSSL error -- %s", ERR_error_string(err, NULL));
        logger.msg(Arc::ERROR, "Library  : %s", ERR_lib_error_string(err));
        logger.msg(Arc::ERROR, "Function : %s", ERR_func_error_string(err));
        logger.msg(Arc::ERROR, "Reason   : %s", ERR_reason_error_string(err));
    }
    return;
}

#define PASS_MIN_LENGTH (0)
static int input_password(char *password, int passwdsz, bool verify,
                          const std::string& prompt_m_info,
                          const std::string& prompt_verify_m_info,
                          Arc::Logger& logger) {
    UI *ui = NULL;
    int res = 0;
    ui = UI_new();
    if (ui) {
        int ok = 0;
        char* buf = new char[passwdsz];
        memset(buf, 0, passwdsz);
        int ui_flags = 0;
        char *prompt1 = NULL;
        char *prompt2 = NULL;
        prompt1 = UI_construct_prompt(ui, "passphrase", prompt_m_info.c_str());
        prompt2 = UI_construct_prompt(ui, "passphrase", prompt_verify_m_info.c_str());
        ui_flags |= UI_INPUT_FLAG_DEFAULT_PWD;
        UI_ctrl(ui, UI_CTRL_PRINT_ERRORS, 1, 0, 0);
        ok = UI_add_input_string(ui, prompt1, ui_flags, password,
                                 PASS_MIN_LENGTH, passwdsz - 1);
        if (ok >= 0 && verify) {
            ok = UI_add_verify_string(ui, prompt2, ui_flags, buf,
                                      PASS_MIN_LENGTH, passwdsz - 1, password);
        }
        if (ok >= 0) {
            do {
                ok = UI_process(ui);
            } while (ok < 0 && UI_ctrl(ui, UI_CTRL_IS_REDOABLE, 0, 0, 0));
        }

        if (ok >= 0) res = strlen(password);
        if (ok == -1) {
            logger.msg(Arc::ERROR, "User interface error");
            tls_process_error(logger);
            memset(password, 0, (unsigned int)passwdsz);
            res = 0;
        }
        if (ok == -2) {
            logger.msg(Arc::ERROR, "Aborted!");
            memset(password, 0, (unsigned int)passwdsz);
            res = 0;
        }
        UI_free(ui);
        delete[] buf;
        OPENSSL_free(prompt1);
        OPENSSL_free(prompt2);
    }
    return res;
}

static bool is_file(std::string path) {
    if (Glib::file_test(path, Glib::FILE_TEST_IS_REGULAR))
        return true;
    return false;
}

static bool is_dir(std::string path) {
    if (Glib::file_test(path, Glib::FILE_TEST_IS_DIR))
        return true;
    return false;
}

static std::vector<std::string> search_vomses(std::string path) {
    std::vector<std::string> vomses_files;
    if(is_file(path)) vomses_files.push_back(path);
    else if(is_dir(path)) {
        //if the path 'vomses' is a directory, search all of the files under this directory,
        //i.e.,  'vomses/voA'  'vomses/voB'
        std::string path_header = path;
        std::string fullpath;
        Glib::Dir dir(path);
        for(Glib::Dir::iterator i = dir.begin(); i != dir.end(); i++ ) {
            fullpath = path_header + G_DIR_SEPARATOR_S + *i;
            if(is_file(fullpath)) vomses_files.push_back(fullpath);
            else if(is_dir(fullpath)) {
                std::string sub_path = fullpath;
                //if the path is a directory, search the all of the files under this directory,
                //i.e., 'vomses/extra/myprivatevo'
                Glib::Dir subdir(sub_path);
                for(Glib::Dir::iterator j = subdir.begin(); j != subdir.end(); j++ ) {
                    fullpath = sub_path + G_DIR_SEPARATOR_S + *j;
                    if(is_file(fullpath)) vomses_files.push_back(fullpath);
                    //else if(is_dir(fullpath)) { //if it is again a directory, the files under it will be ignored }
                }
            }
        }
    }
    return vomses_files;
}

static std::string tokens_to_string(std::vector<std::string> tokens) {
    std::string s;
    for(int n = 0; n<tokens.size(); ++n) {
        s += "\""+tokens[n]+"\" ";
    };
    return s;
}

ArcProxyController::ArcProxyController()
    :logger(Arc::Logger::getRootLogger(), "arcproxy"), logCerr(std::cerr)
{
    setlocale(LC_ALL, "");

    m_use_gsi_comm = false;
    m_use_gsi_proxy = false;
    m_info = false;
    m_remove_proxy = false;
    m_use_empty_passphrase = false; //if use empty passphrase to myproxy server
    m_timeout = -1;
    m_version = false;
    m_use_http_comm = false;

    m_debug = "WARNING";

    // This ensure command line args overwrite all other options
    if(!m_cert_path.empty())Arc::SetEnv("X509_USER_CERT", m_cert_path);
    if(!m_key_path.empty())Arc::SetEnv("X509_USER_KEY", m_key_path);
    if(!m_proxy_path.empty())Arc::SetEnv("X509_USER_PROXY", m_proxy_path);
    if(!m_ca_dir.empty())Arc::SetEnv("X509_CERT_DIR", m_ca_dir);

    m_proxyWindow = 0;
    m_application = 0;

    m_uiReturnStatus = RS_OK;

}

void ArcProxyController::setUiReturnStatus(TReturnStatus status)
{
    m_uiReturnStatus = status;
}

ArcProxyController::TReturnStatus ArcProxyController::getUiReturnStatus()
{
    return m_uiReturnStatus;
}

ArcProxyController::~ArcProxyController()
{
    if (m_proxyWindow!=0)
        delete m_proxyWindow;

    if (m_application!=0)
        delete m_application;

    m_passphrase.fill(0);
    m_passphrase.clear();
}

VomsList& ArcProxyController::vomsList()
{
    return m_vomsList;
}

void ArcProxyController::setPassphrase(const QString& passphrase)
{
    m_passphrase = passphrase;
}

void ArcProxyController::setUseGSIProxy(bool flag)
{
    m_use_gsi_proxy = flag;
}

bool ArcProxyController::getUseGSIProxy()
{
    return m_use_gsi_proxy;
}

ArcProxyController::TCertStatus ArcProxyController::checkCert()
{
    Arc::ArcLocation::Init("");

    Arc::UserConfig usercfg(m_conffile,
                            Arc::initializeCredentialsType(Arc::initializeCredentialsType::TryCredentials));
    if (!usercfg) {
        return CS_INVALID_CONFIG;
    }
    // Check for needed credentials objects
    // Can proxy be used for? Could not find it in documentation.
    // Key and certificate not needed if only printing proxy information
    if((usercfg.CertificatePath().empty() || (usercfg.KeyPath().empty() && (usercfg.CertificatePath().find(".p12") == std::string::npos))) && !m_info) {
        return CS_NOT_FOUND;
    }
    if(!m_vomslist.empty() || !m_myproxy_command.empty()) {
        // For external communication CAs are needed
        if(usercfg.CACertificatesDirectory().empty()) {
            logger.msg(Arc::ERROR, "Failed to find CA certificates");
            return CS_CADIR_NOT_FOUND;
        }
    }
    return CS_VALID;
}

ArcProxyController::TReturnStatus ArcProxyController::showProxyUI()
{
    m_proxyWindow = new ProxyWindow(0, this);
    m_proxyWindow->setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, m_proxyWindow->size(), qApp->desktop()->availableGeometry()));
    m_proxyWindow->exec();
    return m_uiReturnStatus;
}

void ArcProxyController::showProxyUIAppLoop()
{
    m_application = new QApplication(0,0,0);
    this->showProxyUI();
    m_application->exec();
}


int ArcProxyController::initialize()
{
    /*
    Arc::Logger::getRootLogger().removeDestinations();
    Arc::Logger::getRootLogger().addDestination(logCerr);
    Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);
    */
    logCerr.setFormat(Arc::ShortFormat);

    Arc::ArcLocation::Init("");

#ifdef HAVE_NSS
    //Using nss db dominate other option
    if(m_use_nssdb) {
        std::string nssdb_path = get_nssdb_path();
        if(nssdb_path.empty()) {
            std::cout << Arc::IString("The nss db can not be detected under firefox profile") << std::endl;
            return EXIT_FAILURE;
        }
        bool res;
        std::string configdir = nssdb_path;
        res = AuthN::nssInit(configdir);
        std::cout<< Arc::IString("nss db to be accesses: %s\n", configdir.c_str());

        char* slotpw = NULL; //"secretpw";  //TODO: Input passphrase to nss db
        //The nss db under firefox profile seems to not be protected by any passphrase by default
        bool ascii = true;
        const char* trusts = "p,p,p";

        std::string proxy_csrfile = "proxy.csr";
        std::string proxy_keyname = "proxykey";
        std::string proxy_privk_str;
        res = AuthN::nssGenerateCSR(proxy_keyname, "CN=Test,OU=ARC,O=EMI", slotpw, proxy_csrfile, proxy_privk_str, ascii);
        if(!res) return EXIT_FAILURE;

        std::string proxy_certfile = "myproxy.pem";
        std::string issuername = "Imported Certificate";
        //The name of the certificate imported in firefox is
        //normally "Imported Certificate" by default, if name is not specified
        int duration = 12;
        res = AuthN::nssCreateCert(proxy_csrfile, issuername, "", duration, proxy_certfile, ascii);
        if(!res) return EXIT_FAILURE;

        const char* proxy_certname = "proxycert";
        res = AuthN::nssImportCert(slotpw, proxy_certfile, proxy_certname, trusts, ascii);
        if(!res) return EXIT_FAILURE;

        //Compose the proxy certificate
        if(!proxy_path.empty())Arc::SetEnv("X509_USER_PROXY", proxy_path);
        Arc::UserConfig usercfg(conffile,
                                Arc::initializeCredentialsType(Arc::initializeCredentialsType::NotTryCredentials));
        if (!usercfg) {
            logger.msg(Arc::ERROR, "Failed configuration initialization.");
            return EXIT_FAILURE;
        }
        if(proxy_path.empty()) proxy_path = usercfg.ProxyPath();
        usercfg.ProxyPath(proxy_path);
        std::string cert_file = "cert.pem";
        res = AuthN::nssExportCertificate(issuername, cert_file);
        if(!res) return EXIT_FAILURE;

        std::string proxy_cred_str;
        std::ifstream proxy_s(proxy_certfile.c_str());
        std::getline(proxy_s, proxy_cred_str,'\0');
        proxy_s.close();

        std::string eec_cert_str;
        std::ifstream eec_s(cert_file.c_str());
        std::getline(eec_s, eec_cert_str,'\0');
        eec_s.close();

        proxy_cred_str.append(proxy_privk_str).append(eec_cert_str);
        write_proxy_file(proxy_path, proxy_cred_str);

        Arc::Credential proxy_cred(proxy_path, proxy_path, "", "");
        Arc::Time left = proxy_cred.GetEndTime();
        std::cout << Arc::IString("Proxy generation succeeded") << std::endl;
        std::cout << Arc::IString("Your proxy is valid until: %s", left.str(Arc::UserTime)) << std::endl;

        return EXIT_SUCCESS;
    }
#endif

    // If debug is specified as argument, it should be set before loading the configuration.

    if (!m_debug.empty())
        Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(m_debug));

    // Set default, predefined or guessed credentials. Also check if they exist.

    Arc::UserConfig usercfg(m_conffile,
                            Arc::initializeCredentialsType(Arc::initializeCredentialsType::TryCredentials));
    if (!usercfg) {
        logger.msg(Arc::ERROR, "Failed configuration initialization.");
        return EXIT_FAILURE;
    }
    // Check for needed credentials objects
    // Can proxy be used for? Could not find it in documentation.
    // Key and certificate not needed if only printing proxy information
    if((usercfg.CertificatePath().empty() || (usercfg.KeyPath().empty() && (usercfg.CertificatePath().find(".p12") == std::string::npos))) && !m_info) {
        logger.msg(Arc::ERROR, "Failed to find certificate and/or private key or files have improper permissions or ownership.");
        logger.msg(Arc::ERROR, "You may try to increase verbosity to get more information.");
        return EXIT_FAILURE;
    }
    if(!m_vomslist.empty() || !m_myproxy_command.empty()) {
        // For external communication CAs are needed
        if(usercfg.CACertificatesDirectory().empty()) {
            logger.msg(Arc::ERROR, "Failed to find CA certificates");
            logger.msg(Arc::ERROR, "Cannot find the CA certificates directory path, "
                       "please set environment variable X509_CERT_DIR, "
                       "or cacertificatesdirectory in a configuration file.");
            logger.msg(Arc::ERROR, "You may try to increase verbosity to get more information.");
            logger.msg(Arc::ERROR, "The CA certificates directory is required for "
                       "contacting VOMS and MyProxy servers.");
            return EXIT_FAILURE;
        }
    }
    // Proxy is special case. We either need default or predefined path.
    // No guessing or testing is needed.
    // By running credentials initialization once more all set values
    // won't change. But proxy will get default value if not set.
    {
        Arc::UserConfig tmpcfg(m_conffile,
                               Arc::initializeCredentialsType(Arc::initializeCredentialsType::NotTryCredentials));
        if(m_proxy_path.empty()) m_proxy_path = tmpcfg.ProxyPath();
        usercfg.ProxyPath(m_proxy_path);
    }
    // Get back all paths
    if(m_key_path.empty()) m_key_path = usercfg.KeyPath();
    if(m_cert_path.empty()) m_cert_path = usercfg.CertificatePath();
    if(m_ca_dir.empty()) m_ca_dir = usercfg.CACertificatesDirectory();
    if(m_voms_dir.empty()) m_voms_dir = Arc::GetEnv("X509_VOMS_DIR");

    if (m_debug.empty() && !usercfg.Verbosity().empty())
        Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(usercfg.Verbosity()));

    if (m_timeout > 0) usercfg.Timeout(m_timeout);
}

QString ArcProxyController::getIdentity()
{
    QString identity;

    Arc::Credential cred(m_cert_path, "", "", "");

    identity = cred.GetDN().c_str();
    return identity;
}

void ArcProxyController::setValidityPeriod(int seconds)
{
    std::string constraintString;
    std::stringstream out;
    out << "validityPeriod=" << seconds;
    constraintString = out.str();

    m_constraintlist.clear();
    m_constraintlist.push_back(constraintString);
}

ArcProxyController::TProxyStatus ArcProxyController::checkProxy()
{
    const Arc::Time now;

    if (m_proxy_path.empty()) {
        return PS_PATH_EMPTY;
    }
    else if (!(Glib::file_test(m_proxy_path, Glib::FILE_TEST_EXISTS))) {
        return PS_NOT_FOUND;
    }

    Arc::Credential holder(m_proxy_path, "", "", "");

    if (holder.GetEndTime() < now)
    {
        return PS_EXPIRED;
    }
    else if (now < holder.GetStartTime())
    {
        return PS_NOT_VALID;
    }
    else
    {
        return PS_VALID;
    }
    return PS_VALID;
}

int ArcProxyController::printInformation()
{
    const Arc::Time now;
    std::vector<Arc::VOMSACInfo> voms_attributes;
    bool res = false;

    if (m_proxy_path.empty()) {
        logger.msg(Arc::ERROR, "Cannot find the path of the proxy file, "
                   "please setup environment X509_USER_PROXY, "
                   "or proxypath in a configuration file");
        return EXIT_FAILURE;
    }
    else if (!(Glib::file_test(m_proxy_path, Glib::FILE_TEST_EXISTS))) {
        logger.msg(Arc::ERROR, "Cannot find file at %s for getting the proxy. "
                   "Please make sure this file exists.", m_proxy_path);
        return EXIT_FAILURE;
    }

    Arc::Credential holder(m_proxy_path, "", "", "");
    std::cout << Arc::IString("Subject: %s", holder.GetDN()) << std::endl;
    std::cout << Arc::IString("Issuer: %s", holder.GetIssuerName()) << std::endl;
    std::cout << Arc::IString("Identity: %s", holder.GetIdentityName()) << std::endl;
    if (holder.GetEndTime() < now)
        std::cout << Arc::IString("Time left for proxy: Proxy expired") << std::endl;
    else if (now < holder.GetStartTime())
        std::cout << Arc::IString("Time left for proxy: Proxy not valid yet") << std::endl;
    else
        std::cout << Arc::IString("Time left for proxy: %s", (holder.GetEndTime() - now).istr()) << std::endl;
    std::cout << Arc::IString("Proxy path: %s", m_proxy_path) << std::endl;
    std::cout << Arc::IString("Proxy type: %s", certTypeToString(holder.GetType())) << std::endl;

    Arc::VOMSTrustList voms_trust_dn;
    voms_trust_dn.AddRegex(".*");
    res = parseVOMSAC(holder, m_ca_dir, "", m_voms_dir, voms_trust_dn, voms_attributes, true, true);
    // Not printing error message because parseVOMSAC will print everything itself
    //if (!res) logger.msg(Arc::ERROR, "VOMS attribute parsing failed");
    for(int n = 0; n<voms_attributes.size(); ++n) {
        if(voms_attributes[n].attributes.size() > 0) {
            std::cout<<"====== "<<Arc::IString("AC extension information for VO ")<<
                       voms_attributes[n].voname<<" ======"<<std::endl;
            if(voms_attributes[n].status & Arc::VOMSACInfo::ParsingError) {
                std::cout << Arc::IString("Error detected while parsing this AC")<<std::endl;
                if(voms_attributes[n].status & Arc::VOMSACInfo::X509ParsingFailed) {
                    std::cout << "Failed parsing X509 structures ";
                }
                if(voms_attributes[n].status & Arc::VOMSACInfo::ACParsingFailed) {
                    std::cout << "Failed parsing Attribute Certificate structures ";
                }
                if(voms_attributes[n].status & Arc::VOMSACInfo::InternalParsingFailed) {
                    std::cout << "Failed parsing VOMS structures ";
                }
                std::cout << std::endl;
            }
            if(voms_attributes[n].status & Arc::VOMSACInfo::ValidationError) {
                std::cout << Arc::IString("AC is invalid: ");
                if(voms_attributes[n].status & Arc::VOMSACInfo::CAUnknown) {
                    std::cout << "CA of VOMS service is not known ";
                }
                if(voms_attributes[n].status & Arc::VOMSACInfo::CertRevoked) {
                    std::cout << "VOMS service certificate is revoked ";
                }
                if(voms_attributes[n].status & Arc::VOMSACInfo::CertRevoked) {
                    std::cout << "LSC matching/processing failed ";
                }
                if(voms_attributes[n].status & Arc::VOMSACInfo::TrustFailed) {
                    std::cout << "Failed to match configured trust chain ";
                }
                if(voms_attributes[n].status & Arc::VOMSACInfo::TimeValidFailed) {
                    std::cout << "Out of time restrictions ";
                }
                std::cout << std::endl;
            }
            std::cout << "VO        : "<<voms_attributes[n].voname << std::endl;
            std::cout << "subject   : "<<voms_attributes[n].holder << std::endl;
            std::cout << "issuer    : "<<voms_attributes[n].issuer << std::endl;
            for(int i = 0; i < voms_attributes[n].attributes.size(); i++) {
                std::string attr = voms_attributes[n].attributes[i];
                std::string::size_type pos;
                if((pos = attr.find("role")) != std::string::npos) {
                    std::string str = attr.substr(pos+5);
                    std::cout << "attribute : role = " << str << " (" << voms_attributes[n].voname << ")"<<std::endl;
                }
                else if((pos = attr.find("hostname")) != std::string::npos) {
                    std::string str = attr.substr(pos+9);
                    std::cout << "uri       : " << str <<std::endl;
                }
                else
                    std::cout << "attribute : " << attr <<std::endl;

                //std::cout << "attribute : "<<voms_attributes[n].attributes[i]<<std::endl;
                //do not display those attributes that have already been displayed
                //(this can happen when there are multiple voms server )
            }
            Arc::Time ct;
            if(ct < voms_attributes[n].from) {
                std::cout << Arc::IString("Time left for AC: AC is not valid yet")<<std::endl;
            } else if(ct > voms_attributes[n].till) {
                std::cout << Arc::IString("Time left for AC: AC has expired")<<std::endl;
            } else {
                std::cout << Arc::IString("Time left for AC: %s", (voms_attributes[n].till-ct).istr())<<std::endl;
            }
        }
        /*
      Arc::Time now;
      Arc::Time till = voms_attributes[n].till;
      if(now < till)
        std::cout << Arc::IString("Timeleft for AC: %s", (till-now).istr())<<std::endl;
      else
        std::cout << Arc::IString("AC has been expired for: %s", (now-till).istr())<<std::endl;
*/
    }
    return EXIT_SUCCESS;
}

int ArcProxyController::removeProxy()
{
    if (m_proxy_path.empty()) {
        logger.msg(Arc::ERROR, "Cannot find the path of the proxy file, "
                   "please setup environment X509_USER_PROXY, "
                   "or proxypath in a configuration file");
        return EXIT_FAILURE;
    } else if (!(Glib::file_test(m_proxy_path, Glib::FILE_TEST_EXISTS))) {
        logger.msg(Arc::ERROR, "Cannot remove proxy file at %s, because it's not there", m_proxy_path);
        return EXIT_FAILURE;
    }
    if((unlink(m_proxy_path.c_str()) != 0) && (errno != ENOENT)) {
        logger.msg(Arc::ERROR, "Cannot remove proxy file at %s", m_proxy_path);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int ArcProxyController::generateProxy()
{
    Arc::UserConfig usercfg("", Arc::initializeCredentialsType( Arc::initializeCredentialsType::TryCredentials));

    if (!usercfg) {
        logger.msg(Arc::ERROR, "Failed configuration initialization");
        return EXIT_FAILURE;
    }

    const Arc::Time now;
    Arc::User user;

    if ((m_cert_path.empty() || m_key_path.empty()) &&
            ((m_myproxy_command == "PUT") || (m_myproxy_command == "put") || (m_myproxy_command == "Put"))) {
        if (m_cert_path.empty())
            logger.msg(Arc::ERROR, "Cannot find the user certificate path, "
                       "please setup environment X509_USER_CERT, "
                       "or certificatepath in a configuration file");
        if (m_key_path.empty())
            logger.msg(Arc::ERROR, "Cannot find the user private key path, "
                       "please setup environment X509_USER_KEY, "
                       "or keypath in a configuration file");
        return EXIT_FAILURE;
    }

    std::map<std::string, std::string> constraints;
    for (std::list<std::string>::iterator it = m_constraintlist.begin();
         it != m_constraintlist.end(); it++) {
        std::string::size_type pos = it->find('=');
        if (pos != std::string::npos)
            constraints[it->substr(0, pos)] = it->substr(pos + 1);
        else
            constraints[*it] = "";
    }

    //proxy validity period
    //Set the default proxy validity lifetime to 12 hours if there is
    //no validity lifetime provided by command caller
    // Set default values first
    // TODO: Is default validityPeriod since now or since validityStart?
    Arc::Time validityStart = now; // now by default
    Arc::Period validityPeriod(12*60*60);
    if (m_myproxy_command == "put" || m_myproxy_command == "PUT" || m_myproxy_command == "Put") {
        //For myproxy PUT operation, the proxy should be 7 days according to the default
        //definition in myproxy implementation.
        validityPeriod = 7*24*60*60;
    }
    // Acquire constraints. Check for valid values and conflicts.
    if((!constraints["validityStart"].empty()) &&
            (!constraints["validityEnd"].empty()) &&
            (!constraints["validityPeriod"].empty())) {
        std::cerr << Arc::IString("The start, end and period can't be set simultaneously") << std::endl;
        return EXIT_FAILURE;
    }
    if(!constraints["validityStart"].empty()) {
        validityStart = Arc::Time(constraints["validityStart"]);
        if (validityStart == Arc::Time(Arc::Time::UNDEFINED)) {
            std::cerr << Arc::IString("The start time that you set: %s can't be recognized.", (std::string)constraints["validityStart"]) << std::endl;
            return EXIT_FAILURE;
        }
    }
    if(!constraints["validityPeriod"].empty()) {
        validityPeriod = Arc::Period(constraints["validityPeriod"]);
        if (validityPeriod.GetPeriod() <= 0) {
            std::cerr << Arc::IString("The period that you set: %s can't be recognized.", (std::string)constraints["validityPeriod"]) << std::endl;
            return EXIT_FAILURE;
        }
    }
    if(!constraints["validityEnd"].empty()) {
        Arc::Time validityEnd = Arc::Time(constraints["validityEnd"]);
        if (validityEnd == Arc::Time(Arc::Time::UNDEFINED)) {
            std::cerr << Arc::IString("The end time that you set: %s can't be recognized.", (std::string)constraints["validityEnd"]) << std::endl;
            return EXIT_FAILURE;
        }
        if(!constraints["validityPeriod"].empty()) {
            // If period is explicitely set then start is derived from end and period
            validityStart = validityEnd - validityPeriod;
        } else {
            // otherwise start - optionally - and end are set, period is derived
            if(validityEnd < validityStart) {
                std::cerr << Arc::IString("The end time that you set: %s is before start time:%s.", (std::string)validityEnd,(std::string)validityStart) << std::endl;
                // error
                return EXIT_FAILURE;
            }
            validityPeriod = validityEnd - validityStart;
        }
    }
    // Here we have validityStart and validityPeriod defined
    Arc::Time validityEnd = validityStart + validityPeriod;
    // Warn user about strange times but do not prevent user from doing anything legal
    if(validityStart < now) {
        std::cout << Arc::IString("WARNING: The start time that you set: %s is before current time: %s", (std::string)validityStart, (std::string)now) << std::endl;
    }
    if(validityEnd < now) {
        std::cout << Arc::IString("WARNING: The end time that you set: %s is before current time: %s", (std::string)validityEnd, (std::string)now) << std::endl;
    }

    //voms AC valitity period
    //Set the default voms AC validity lifetime to 12 hours if there is
    //no validity lifetime provided by command caller
    Arc::Period vomsACvalidityPeriod(12*60*60);
    if(!constraints["vomsACvalidityPeriod"].empty()) {
        vomsACvalidityPeriod = Arc::Period(constraints["vomsACvalidityPeriod"]);
        if (vomsACvalidityPeriod.GetPeriod() == 0) {
            std::cerr << Arc::IString("The VOMS AC period that you set: %s can't be recognized.", (std::string)constraints["vomsACvalidityPeriod"]) << std::endl;
            return EXIT_FAILURE;
        }
    } else {
        if(validityPeriod < vomsACvalidityPeriod) vomsACvalidityPeriod = validityPeriod;
        // It is strange that VOMS AC may be valid less than proxy itself.
        // Maybe it would be more correct to have it valid by default from
        // now till validityEnd.
    }
    std::string voms_period = Arc::tostring(vomsACvalidityPeriod.GetPeriod());

    //myproxy validity period.
    //Set the default myproxy validity lifetime to 12 hours if there is
    //no validity lifetime provided by command caller
    Arc::Period myproxyvalidityPeriod(12*60*60);
    if(!constraints["myproxyvalidityPeriod"].empty()) {
        myproxyvalidityPeriod = Arc::Period(constraints["myproxyvalidityPeriod"]);
        if (myproxyvalidityPeriod.GetPeriod() == 0) {
            std::cerr << Arc::IString("The MyProxy period that you set: %s can't be recognized.", (std::string)constraints["myproxyvalidityPeriod"]) << std::endl;
            return EXIT_FAILURE;
        }
    } else {
        if(validityPeriod < myproxyvalidityPeriod) myproxyvalidityPeriod = validityPeriod;
        // see vomsACvalidityPeriod
    }
    std::string myproxy_period = Arc::tostring(myproxyvalidityPeriod.GetPeriod());


    Arc::OpenSSLInit();

    //If the "INFO" myproxy command is given, try to get the
    //information about the existence of stored credentials
    //on the myproxy server.
    try {
        if (m_myproxy_command == "info" || m_myproxy_command == "INFO" || m_myproxy_command == "Info") {
            if (m_myproxy_server.empty())
                throw std::invalid_argument("URL of MyProxy server is missing");

            if(m_user_name.empty()) {
                Arc::Credential proxy_cred(m_proxy_path, "", "", "");
                std::string cert_dn = proxy_cred.GetIdentityName();
                m_user_name = cert_dn;
            }
            if (m_user_name.empty())
                throw std::invalid_argument("Username to MyProxy server is missing");

            std::string respinfo;

            //if(usercfg.CertificatePath().empty()) usercfg.CertificatePath(cert_path);
            //if(usercfg.KeyPath().empty()) usercfg.KeyPath(key_path);
            if(usercfg.ProxyPath().empty() && !m_proxy_path.empty()) usercfg.ProxyPath(m_proxy_path);
            else {
                if(usercfg.CertificatePath().empty() && !m_cert_path.empty()) usercfg.CertificatePath(m_cert_path);
                if(usercfg.KeyPath().empty() && !m_key_path.empty()) usercfg.KeyPath(m_key_path);
            }
            if(usercfg.CACertificatesDirectory().empty()) usercfg.CACertificatesDirectory(m_ca_dir);

            Arc::CredentialStore cstore(usercfg,Arc::URL("myproxy://"+m_myproxy_server));
            std::map<std::string,std::string> myproxyopt;
            myproxyopt["username"] = m_user_name;
            if(!cstore.Info(myproxyopt,respinfo))
                throw std::invalid_argument("Failed to get info from MyProxy service");

            std::cout << Arc::IString("Succeeded to get info from MyProxy server") << std::endl;
            std::cout << respinfo << std::endl;
            return EXIT_SUCCESS;
        }

    } catch (std::exception& err) {
        logger.msg(Arc::ERROR, err.what());
        tls_process_error(logger);
        return EXIT_FAILURE;
    }

    //If the "NEWPASS" myproxy command is given, try to get the
    //information about the existence of stored credentials
    //on the myproxy server.
    try {
        if (m_myproxy_command == "newpass" || m_myproxy_command == "NEWPASS" || m_myproxy_command == "Newpass" || m_myproxy_command == "NewPass") {
            if (m_myproxy_server.empty())
                throw std::invalid_argument("URL of MyProxy server is missing");

            if(m_user_name.empty()) {
                Arc::Credential proxy_cred(m_proxy_path, "", "", "");
                std::string cert_dn = proxy_cred.GetIdentityName();
                m_user_name = cert_dn;
            }
            if (m_user_name.empty())
                throw std::invalid_argument("Username to MyProxy server is missing");

            std::string prompt1 = "MyProxy server";
            char password[256];
            std::string passphrase;
            int res = input_password(password, 256, false, prompt1, "", logger);
            if (!res)
                throw std::invalid_argument("Error entering passphrase");
            passphrase = password;

            std::string prompt2 = "MyProxy server";
            char newpassword[256];
            std::string newpassphrase;
            res = input_password(newpassword, 256, true, prompt1, prompt2, logger);
            if (!res)
                throw std::invalid_argument("Error entering passphrase");
            newpassphrase = newpassword;

            if(usercfg.ProxyPath().empty() && !m_proxy_path.empty()) usercfg.ProxyPath(m_proxy_path);
            else {
                if(usercfg.CertificatePath().empty() && !m_cert_path.empty()) usercfg.CertificatePath(m_cert_path);
                if(usercfg.KeyPath().empty() && !m_key_path.empty()) usercfg.KeyPath(m_key_path);
            }
            if(usercfg.CACertificatesDirectory().empty()) usercfg.CACertificatesDirectory(m_ca_dir);

            Arc::CredentialStore cstore(usercfg,Arc::URL("myproxy://"+m_myproxy_server));
            std::map<std::string,std::string> myproxyopt;
            myproxyopt["username"] = m_user_name;
            myproxyopt["password"] = passphrase;
            myproxyopt["newpassword"] = newpassphrase;
            if(!cstore.ChangePassword(myproxyopt))
                throw std::invalid_argument("Failed to change password MyProxy service");

            std::cout << Arc::IString("Succeeded to change password on MyProxy server") << std::endl;

            return EXIT_SUCCESS;
        }

    } catch (std::exception& err) {
        logger.msg(Arc::ERROR, err.what());
        tls_process_error(logger);
        return EXIT_FAILURE;
    }

    //If the "DESTROY" myproxy command is given, try to get the
    //information about the existence of stored credentials
    //on the myproxy server.
    try {
        if (m_myproxy_command == "destroy" || m_myproxy_command == "DESTROY" || m_myproxy_command == "Destroy") {
            if (m_myproxy_server.empty())
                throw std::invalid_argument("URL of MyProxy server is missing");

            if(m_user_name.empty()) {
                Arc::Credential proxy_cred(m_proxy_path, "", "", "");
                std::string cert_dn = proxy_cred.GetIdentityName();
                m_user_name = cert_dn;
            }
            if (m_user_name.empty())
                throw std::invalid_argument("Username to MyProxy server is missing");

            std::string prompt1 = "MyProxy server";
            char password[256];
            std::string passphrase;
            int res = input_password(password, 256, false, prompt1, "", logger);
            if (!res)
                throw std::invalid_argument("Error entering passphrase");
            passphrase = password;

            std::string respinfo;

            if(usercfg.ProxyPath().empty() && !m_proxy_path.empty()) usercfg.ProxyPath(m_proxy_path);
            else {
                if(usercfg.CertificatePath().empty() && !m_cert_path.empty()) usercfg.CertificatePath(m_cert_path);
                if(usercfg.KeyPath().empty() && !m_key_path.empty()) usercfg.KeyPath(m_key_path);
            }
            if(usercfg.CACertificatesDirectory().empty()) usercfg.CACertificatesDirectory(m_ca_dir);

            Arc::CredentialStore cstore(usercfg,Arc::URL("myproxy://"+m_myproxy_server));
            std::map<std::string,std::string> myproxyopt;
            myproxyopt["username"] = m_user_name;
            myproxyopt["password"] = passphrase;
            if(!cstore.Destroy(myproxyopt))
                throw std::invalid_argument("Failed to destroy credential on MyProxy service");

            std::cout << Arc::IString("Succeeded to destroy credential on MyProxy server") << std::endl;

            return EXIT_SUCCESS;
        }
    } catch (std::exception& err) {
        logger.msg(Arc::ERROR, err.what());
        tls_process_error(logger);
        return EXIT_FAILURE;
    }

    //If the "GET" myproxy command is given, try to get a delegated
    //certificate from the myproxy server.
    //For "GET" command, certificate and key are not needed, and
    //anonymous GSSAPI is used (GSS_C_ANON_FLAG)
    try {
        if (m_myproxy_command == "get" || m_myproxy_command == "GET" || m_myproxy_command == "Get") {
            if (m_myproxy_server.empty())
                throw std::invalid_argument("URL of MyProxy server is missing");

            if(m_user_name.empty()) {
                Arc::Credential proxy_cred(m_proxy_path, "", "", "");
                std::string cert_dn = proxy_cred.GetIdentityName();
                m_user_name = cert_dn;
            }
            if (m_user_name.empty())
                throw std::invalid_argument("Username to MyProxy server is missing");

            std::string prompt1 = "MyProxy server";
            char password[256];

            std::string passphrase = password;
            if(!m_use_empty_passphrase) {
                int res = input_password(password, 256, false, prompt1, "", logger);
                if (!res)
                    throw std::invalid_argument("Error entering passphrase");
                passphrase = password;
            }

            std::string proxy_cred_str_pem;

            Arc::initializeCredentialsType cred_type(Arc::initializeCredentialsType::SkipCredentials);
            Arc::UserConfig usercfg_tmp(cred_type);
            usercfg_tmp.CACertificatesDirectory(usercfg.CACertificatesDirectory());

            Arc::CredentialStore cstore(usercfg_tmp,Arc::URL("myproxy://"+m_myproxy_server));
            std::map<std::string,std::string> myproxyopt;
            myproxyopt["username"] = m_user_name;
            myproxyopt["password"] = passphrase;
            myproxyopt["lifetime"] = myproxy_period;
            if(!cstore.Retrieve(myproxyopt,proxy_cred_str_pem))
                throw std::invalid_argument("Failed to retrieve proxy from MyProxy service");
            write_proxy_file(m_proxy_path,proxy_cred_str_pem);

            //Assign proxy_path to cert_path and key_path,
            //so the later voms functionality can use the proxy_path
            //to create proxy with voms AC extension. In this
            //case, "--cert" and "--key" is not needed.
            m_cert_path = m_proxy_path;
            m_key_path = m_proxy_path;
            std::cout << Arc::IString("Succeeded to get a proxy in %s from MyProxy server %s", m_proxy_path, m_myproxy_server) << std::endl;

            return EXIT_SUCCESS;
        }

    } catch (std::exception& err) {
        logger.msg(Arc::ERROR, err.what());
        tls_process_error(logger);
        return EXIT_FAILURE;
    }

    Arc::Time proxy_start = validityStart;
    Arc::Period proxy_period = validityPeriod;
    if (constraints["validityStart"].empty() && constraints["validityEnd"].empty()) {
        // If start/end is not explicitely specified then add 5 min back gap.
        proxy_start = proxy_start - Arc::Period(300);
        proxy_period.SetPeriod(proxy_period.GetPeriod() + 300);
    }

    //Create proxy or voms proxy
    try {
        Arc::Credential signer(m_cert_path, m_key_path, "", "", this->m_passphrase.toStdString());
        if (signer.GetIdentityName().empty()) {
            std::cerr << Arc::IString("Proxy generation failed: No valid certificate found.") << std::endl;
            return EXIT_FAILURE;
        }
        EVP_PKEY* pkey = signer.GetPrivKey();
        if(!pkey) {
            std::cerr << Arc::IString("Proxy generation failed: No valid private key found.") << std::endl;
            return EXIT_FAILURE;
        }
        if(pkey) EVP_PKEY_free(pkey);
        std::cout << Arc::IString("Your identity: %s", signer.GetIdentityName()) << std::endl;
        if (now > signer.GetEndTime()) {
            std::cerr << Arc::IString("Proxy generation failed: Certificate has expired.") << std::endl;
            return EXIT_FAILURE;
        }
        else if (now < signer.GetStartTime()) {
            std::cerr << Arc::IString("Proxy generation failed: Certificate is not valid yet.") << std::endl;
            return EXIT_FAILURE;
        }

        std::string private_key, signing_cert, signing_cert_chain;

        int keybits = 1024;
        std::string req_str;
        std::string policy;
        policy = constraints["proxyPolicy"].empty() ? constraints["proxyPolicyFile"] : constraints["proxyPolicy"];
        Arc::Credential cred_request(proxy_start, proxy_period, keybits);
        cred_request.GenerateRequest(req_str);
        cred_request.OutputPrivatekey(private_key);
        signer.OutputCertificate(signing_cert);
        signer.OutputCertificateChain(signing_cert_chain);

        if (!m_vomslist.empty()) { //If we need to generate voms proxy

            //Generate a temporary self-signed proxy certificate
            //to contact the voms server
            std::string proxy_cert;
            if (!signer.SignRequest(&cred_request, proxy_cert))
                throw std::runtime_error("Failed to sign proxy");
            proxy_cert.append(private_key).append(signing_cert).append(signing_cert_chain);
            write_proxy_file(m_proxy_path,proxy_cert);

            //Parse the voms server and command from command line
            std::multimap<std::string, std::string> server_command_map;
            for (std::list<std::string>::iterator it = m_vomslist.begin();
                 it != m_vomslist.end(); it++) {
                size_t p;
                std::string voms_server;
                std::string command;
                p = (*it).find(":");
                //here user could give voms name or voms nick name
                voms_server = (p == std::string::npos) ? (*it) : (*it).substr(0, p);
                command = (p == std::string::npos) ? "" : (*it).substr(p + 1);
                server_command_map.insert(std::pair<std::string, std::string>(voms_server, command));
            }

            //Parse the 'vomses' file to find configure lines corresponding to
            //the information from the command line
            if (m_vomses_path.empty())
                m_vomses_path = usercfg.VOMSESPath();
            if (m_vomses_path.empty()) {
                logger.msg(Arc::ERROR, "$X509_VOMS_FILE, and $X509_VOMSES are not set;\nUser has not specify the location for vomses information;\nThere is also not vomses location information in user's configuration file;\nCannot find vomses in default locations: ~/.arc/vomses, ~/.voms/vomses, $ARC_LOCATION/etc/vomses, $ARC_LOCATION/etc/grid-security/vomses, $PWD/vomses, /etc/vomses, /etc/grid-security/vomses, and the location at the corresponding sub-directory");
                return EXIT_FAILURE;
            }

            //the 'vomses' location could be one single files;
            //or it could be a directory which includes multiple files, such as 'vomses/voA', 'vomses/voB', etc.
            //or it could be a directory which includes multiple directories that includes multiple files,
            //such as 'vomses/atlas/voA', 'vomses/atlas/voB', 'vomses/alice/voa', 'vomses/alice/vob',
            //'vomses/extra/myprivatevo', 'vomses/mypublicvo'
            std::vector<std::string> vomses_files;
            //If the location is a file
            if(is_file(m_vomses_path)) vomses_files.push_back(m_vomses_path);
            //If the locaton is a directory, all the files and directories will be scanned
            //to find the vomses information. The scanning will not stop until all of the
            //files and directories are all scanned.
            else {
                std::vector<std::string> files;
                files = search_vomses(m_vomses_path);
                if(!files.empty())vomses_files.insert(vomses_files.end(), files.begin(), files.end());
                files.clear();
            }

            std::map<std::string, std::vector<std::vector<std::string> > > matched_voms_line;
            for(std::vector<std::string>::iterator file_i = vomses_files.begin(); file_i != vomses_files.end(); file_i++) {
                std::string vomses_file = *file_i;
                std::ifstream in_f(vomses_file.c_str());
                std::string voms_line;
                while (true) {
                    voms_line.clear();
                    std::getline<char>(in_f, voms_line, '\n');
                    if (voms_line.empty())
                        break;
                    if((voms_line.size() >= 1) && (voms_line[0] == '#')) continue;

                    bool has_find = false;
                    //boolean value to record if the vomses server information has been found in this vomses line
                    std::vector<std::string> voms_tokens;
                    Arc::tokenize(voms_line,voms_tokens," \t","\"");
#define VOMS_LINE_NICKNAME (0)
#define VOMS_LINE_HOST (1)
#define VOMS_LINE_PORT (2)
#define VOMS_LINE_SN (3)
#define VOMS_LINE_NAME (4)
#define VOMS_LINE_NUM (5)
                    if(voms_tokens.size() != VOMS_LINE_NUM) {
                        // Warning: malformed voms line
                        logger.msg(Arc::WARNING, "VOMS line contains wrong number of tokens (%u expected): \"%s\"", (unsigned int)VOMS_LINE_NUM, voms_line);
                    }
                    if(voms_tokens.size() > VOMS_LINE_NAME) {
                        std::string str = voms_tokens[VOMS_LINE_NAME];

                        for (std::multimap<std::string, std::string>::iterator it = server_command_map.begin();
                             it != server_command_map.end(); it++) {
                            std::string voms_server = (*it).first;
                            if (str == voms_server) {
                                matched_voms_line[voms_server].push_back(voms_tokens);
                                has_find = true;
                                break;
                            };
                        };
                    };

                    if(!has_find) {
                        //you can also use the nick name of the voms server
                        if(voms_tokens.size() > VOMS_LINE_NAME) {
                            std::string str1 = voms_tokens[VOMS_LINE_NAME];
                            for (std::multimap<std::string, std::string>::iterator it = server_command_map.begin();
                                 it != server_command_map.end(); it++) {
                                std::string voms_server = (*it).first;
                                if (str1 == voms_server) {
                                    matched_voms_line[voms_server].push_back(voms_tokens);
                                    break;
                                };
                            };
                        };
                    };
                };
            };//end of scanning all of the vomses files

            //Judge if we can not find any of the voms server in the command line from 'vomses' file
            //if(matched_voms_line.empty()) {
            //  logger.msg(Arc::ERROR, "Cannot get voms server information from file: %s", vomses_path);
            // throw std::runtime_error("Cannot get voms server information from file: " + vomses_path);
            //}
            //if (matched_voms_line.size() != server_command_map.size())
            for (std::multimap<std::string, std::string>::iterator it = server_command_map.begin();
                 it != server_command_map.end(); it++)
                if (matched_voms_line.find((*it).first) == matched_voms_line.end())
                    logger.msg(Arc::ERROR, "Cannot get VOMS server %s information from the vomses files",
                               (*it).first);

            //Contact the voms server to retrieve attribute certificate
            ArcCredential::AC **aclist = NULL;
            std::string acorder;
            Arc::MCCConfig cfg;
            cfg.AddProxy(m_proxy_path);
            cfg.AddCADir(m_ca_dir);

            for (std::map<std::string, std::vector<std::vector<std::string> > >::iterator it = matched_voms_line.begin();
                 it != matched_voms_line.end(); it++) {
                std::string voms_server;
                std::list<std::string> command_list;
                voms_server = (*it).first;
                std::vector<std::vector<std::string> > voms_lines = (*it).second;

                bool succeeded = false; //a boolean value to indicate if there is valid message returned from voms server, by using the current voms_line
                for (std::vector<std::vector<std::string> >::iterator line_it = voms_lines.begin(); line_it != voms_lines.end(); line_it++) {
                    std::vector<std::string> voms_line = *line_it;
                    int count = server_command_map.count(voms_server);
                    logger.msg(Arc::DEBUG, "There are %d commands to the same VOMS server %s", count, voms_server);

                    std::multimap<std::string, std::string>::iterator command_it;
                    for(command_it = server_command_map.equal_range(voms_server).first; command_it!=server_command_map.equal_range(voms_server).second; ++command_it) {
                        command_list.push_back((*command_it).second);
                    }

                    std::string address;
                    if(voms_line.size() > VOMS_LINE_HOST) address = voms_line[VOMS_LINE_HOST];
                    if(address.empty()) {
                        logger.msg(Arc::ERROR, "Cannot get VOMS server address information from vomses line: \"%s\"", tokens_to_string(voms_line));
                        throw std::runtime_error("Cannot get VOMS server address information from vomses line: \"" + tokens_to_string(voms_line) + "\"");
                    }

                    std::string port;
                    if(voms_line.size() > VOMS_LINE_PORT) port = voms_line[VOMS_LINE_PORT];

                    std::string voms_name;
                    if(voms_line.size() > VOMS_LINE_NAME) voms_name = voms_line[VOMS_LINE_NAME];

                    logger.msg(Arc::INFO, "Contacting VOMS server (named %s): %s on port: %s",
                               voms_name, address, port);
                    std::cout << Arc::IString("Contacting VOMS server (named %s): %s on port: %s", voms_name, address, port) << std::endl;

                    std::string send_msg;
                    send_msg.append("<?xml version=\"1.0\" encoding = \"US-ASCII\"?><voms>");
                    std::string command;

                    for(std::list<std::string>::iterator c_it = command_list.begin(); c_it != command_list.end(); c_it++) {
                        std::string command_2server;
                        command = *c_it;
                        if (command.empty())
                            command_2server.append("G/").append(voms_name);
                        else if (command == "all" || command == "ALL")
                            command_2server.append("A");
                        else if (command == "list")
                            command_2server.append("N");
                        else {
                            std::string::size_type pos = command.find("/Role=");
                            if (pos == 0)
                                command_2server.append("R").append(command.substr(pos + 6));
                            else if (pos != std::string::npos && pos > 0)
                                command_2server.append("B").append(command.substr(0, pos)).append(":").append(command.substr(pos + 6));
                            else if(command[0] == '/')
                                command_2server.append("G").append(command);
                        }
                        send_msg.append("<command>").append(command_2server).append("</command>");
                    }

                    std::string ordering;
                    for(std::list<std::string>::iterator o_it = m_orderlist.begin(); o_it != m_orderlist.end(); o_it++) {
                        ordering.append(o_it == m_orderlist.begin() ? "" : ",").append(*o_it);
                    }
                    logger.msg(Arc::VERBOSE, "Try to get attribute from VOMS server with order: %s", ordering);
                    send_msg.append("<order>").append(ordering).append("</order>");
                    send_msg.append("<lifetime>").append(voms_period).append("</lifetime></voms>");
                    logger.msg(Arc::VERBOSE, "Message sent to VOMS server %s is: %s", voms_name, send_msg);

                    std::string ret_str;
                    if(m_use_http_comm) {
                        // Use http to contact voms server, for the RESRful interface provided by voms server
                        // The format of the URL: https://moldyngrid.org:15112/generate-ac?fqans=/testbed.univ.kiev.ua/blabla/Role=test-role&lifetime=86400
                        // fqans is composed of the voname, group name and role, i.e., the "command" for voms.
                        std::string url_str;
                        if(!command.empty()) url_str = "https://" + address + ":" + port + "/generate-ac?" + "fqans=" + command + "&lifetime=" + voms_period;
                        else url_str = "https://" + address + ":" + port + "/generate-ac?" + "lifetime=" + voms_period;
                        Arc::URL voms_url(url_str);
                        Arc::ClientHTTP client(cfg, voms_url, usercfg.Timeout());
                        client.RelativeURI(true);
                        Arc::PayloadRaw request;
                        Arc::PayloadRawInterface* response;
                        Arc::HTTPClientInfo info;
                        Arc::MCC_Status status = client.process("GET", &request, &info, &response);
                        if (!status) {
                            if (response) delete response;
                            std::cout << Arc::IString("The VOMS server with the information:\n\t%s\"\ncan not be reached, please make sure it is available", tokens_to_string(voms_line)) << std::endl;
                            continue; //There could be another voms replicated server with the same name exists
                        }
                        if (!response) {
                            logger.msg(Arc::ERROR, "No http response from VOMS server");
                            continue;
                        }
                        if(response->Content() != NULL) ret_str.append(response->Content());
                        if (response) delete response;
                        logger.msg(Arc::VERBOSE, "Returned message from VOMS server: %s", ret_str);
                    }
                    else {
                        // Use GSI or TLS to contact voms server
                        Arc::ClientTCP client(cfg, address, atoi(port.c_str()), m_use_gsi_comm ? Arc::GSISec : Arc::SSL3Sec, usercfg.Timeout());
                        Arc::PayloadRaw request;
                        request.Insert(send_msg.c_str(), 0, send_msg.length());
                        Arc::PayloadStreamInterface *response = NULL;
                        Arc::MCC_Status status = client.process(&request, &response, true);
                        if (!status) {
                            //logger.msg(Arc::ERROR, (std::string)status);
                            if (response) delete response;
                            std::cout << Arc::IString("The VOMS server with the information:\n\t%s\"\ncan not be reached, please make sure it is available", tokens_to_string(voms_line)) << std::endl;
                            continue; //There could be another voms replicated server with the same name exists
                        }
                        if (!response) {
                            logger.msg(Arc::ERROR, "No stream response from VOMS server");
                            continue;
                        }
                        char ret_buf[1024];
                        int len = sizeof(ret_buf);
                        while(response->Get(ret_buf, len)) {
                            ret_str.append(ret_buf, len);
                            len = sizeof(ret_buf);
                        };
                        if (response) delete response;
                        logger.msg(Arc::VERBOSE, "Returned message from VOMS server: %s", ret_str);
                    }

                    Arc::XMLNode node;
                    Arc::XMLNode(ret_str).Exchange(node);
                    if((!node) || ((bool)(node["error"]))) {
                        if((bool)(node["error"])) {
                            std::string str = node["error"]["item"]["message"];
                            std::string::size_type pos;
                            std::string tmp_str = "The validity of this VOMS AC in your proxy is shortened to";
                            if((pos = str.find(tmp_str))!= std::string::npos) {
                                std::string tmp = str.substr(pos + tmp_str.size() + 1);
                                std::cout << Arc::IString("The validity duration of VOMS AC is shortened from %s to %s, due to the validity constraint on voms server side.\n", voms_period, tmp);
                            }
                            else
                                std::cout << Arc::IString("Cannot get any AC or attributes info from VOMS server: %s;\n       Returned message from VOMS server: %s\n", voms_server, str);
                            break; //since the voms servers with the same name should be looked as the same for robust reason, the other voms server should that can be reached could returned the same message. So we exists the loop, even if there are other backup voms server exist.
                        }
                        else
                            std::cout << Arc::IString("Returned message from VOMS server %s is: %s\n", voms_server, ret_str);
                        break;
                    }

                    //Put the return attribute certificate into proxy certificate as the extension part
                    std::string codedac;
                    if (command == "list")
                        codedac = (std::string)(node["bitstr"]);
                    else
                        codedac = (std::string)(node["ac"]);
                    std::string decodedac;
                    int size;
                    char *dec = NULL;
                    dec = Arc::VOMSDecode((char*)(codedac.c_str()), codedac.length(), &size);
                    if (dec != NULL) {
                        decodedac.append(dec, size);
                        free(dec);
                        dec = NULL;
                    }

                    if (command == "list") {
                        //logger.msg(Arc::INFO, "The attribute information from voms server: %s is list as following:\n%s",
                        //           voms_server, decodedac);
                        std::cout << Arc::IString("The attribute information from VOMS server: %s is list as following:", voms_server) << std::endl << decodedac << std::endl;
                        return EXIT_SUCCESS;
                    }

                    Arc::addVOMSAC(aclist, acorder, decodedac);
                    succeeded = true; break;
                }//end of the scanning of multiple vomses lines with the same name
                if(succeeded==false) {
                    if(voms_lines.size() > 1)
                        std::cout << Arc::IString("There are %d servers with the same name: %s in your vomses file, but all of them can not been reached, or can return valid message. But proxy without voms AC extension will still be generated.", voms_lines.size(), voms_server) << std::endl;
                }
            }

            //Put the returned attribute certificate into proxy certificate
            if (aclist != NULL)
                cred_request.AddExtension("acseq", (char**)aclist);
            else std::cout << Arc::IString("Failed to add voms AC extension. Your proxy may be incomplete.") << std::endl;
            if (!acorder.empty())
                cred_request.AddExtension("order", acorder);
        }

        if (!m_use_gsi_proxy) {
            if(!policy.empty()) {
                cred_request.SetProxyPolicy("rfc", "anylanguage", policy, -1);
            } else if(CERT_IS_LIMITED_PROXY(signer.GetType())) {
                // Gross hack for globus. If Globus marks own proxy as limited
                // it expects every derived proxy to be limited or at least
                // independent. Independent proxies has little sense in Grid
                // world. So here we make our proxy globus-limited to allow
                // it to be used with globus code.
                cred_request.SetProxyPolicy("rfc", "limited", policy, -1);
            } else {
                cred_request.SetProxyPolicy("rfc", "inheritAll", policy, -1);
            }
        } else {
            cred_request.SetProxyPolicy("gsi2", "", "", -1);
        }

        std::string proxy_cert;
        if (!signer.SignRequest(&cred_request, proxy_cert))
            throw std::runtime_error("Failed to sign proxy");

        proxy_cert.append(private_key).append(signing_cert).append(signing_cert_chain);

        //If myproxy command is "Put", then the proxy path is set to /tmp/myproxy-proxy.uid.pid
        if (m_myproxy_command == "put" || m_myproxy_command == "PUT" || m_myproxy_command == "Put")
            m_proxy_path = Glib::build_filename(Glib::get_tmp_dir(), "myproxy-proxy."
                                              + Arc::tostring(user.get_uid()) + Arc::tostring((int)(getpid())));
        write_proxy_file(m_proxy_path,proxy_cert);

        Arc::Credential proxy_cred(m_proxy_path, m_proxy_path, "", "");
        Arc::Time left = proxy_cred.GetEndTime();
        std::cout << Arc::IString("Proxy generation succeeded") << std::endl;
        std::cout << Arc::IString("Your proxy is valid until: %s", left.str(Arc::UserTime)) << std::endl;

        //return EXIT_SUCCESS;
    } catch (std::exception& err) {
        logger.msg(Arc::ERROR, err.what());
        tls_process_error(logger);
        return EXIT_FAILURE;
    }

    //Delegate the former self-delegated credential to
    //myproxy server
    try {
        if (m_myproxy_command == "put" || m_myproxy_command == "PUT" || m_myproxy_command == "Put") {
            if (m_myproxy_server.empty())
                throw std::invalid_argument("URL of MyProxy server is missing");
            if(m_user_name.empty()) {
                Arc::Credential proxy_cred(m_proxy_path, "", "", "");
                std::string cert_dn = proxy_cred.GetIdentityName();
                m_user_name = cert_dn;
            }
            if (m_user_name.empty())
                throw std::invalid_argument("Username to MyProxy server is missing");

            std::string prompt1 = "MyProxy server";
            std::string prompt2 = "MyProxy server";
            char password[256];
            std::string passphrase;
            if(m_retrievable_by_cert.empty()) {
                int res = input_password(password, 256, true, prompt1, prompt2, logger);
                if (!res)
                    throw std::invalid_argument("Error entering passphrase");
                passphrase = password;
            }

            std::string proxy_cred_str_pem;
            std::ifstream proxy_cred_file(m_proxy_path.c_str());
            if(!proxy_cred_file)
                throw std::invalid_argument("Failed to read proxy file "+m_proxy_path);
            std::getline(proxy_cred_file,proxy_cred_str_pem,'\0');
            if(proxy_cred_str_pem.empty())
                throw std::invalid_argument("Failed to read proxy file "+m_proxy_path);
            proxy_cred_file.close();

            usercfg.ProxyPath(m_proxy_path);
            if(usercfg.CACertificatesDirectory().empty()) { usercfg.CACertificatesDirectory(m_ca_dir); }

            Arc::CredentialStore cstore(usercfg,Arc::URL("myproxy://"+m_myproxy_server));
            std::map<std::string,std::string> myproxyopt;
            myproxyopt["username"] = m_user_name;
            myproxyopt["password"] = passphrase;
            myproxyopt["lifetime"] = myproxy_period;
            if(!m_retrievable_by_cert.empty()) {
                myproxyopt["retriever_trusted"] = m_retrievable_by_cert;
            }
            if(!cstore.Store(myproxyopt,proxy_cred_str_pem,true,proxy_start,proxy_period))
                throw std::invalid_argument("Failed to delegate proxy to MyProxy service");

            remove_proxy_file(m_proxy_path);

            std::cout << Arc::IString("Succeeded to put a proxy onto MyProxy server") << std::endl;

            return EXIT_SUCCESS;
        }
    } catch (std::exception& err) {
        logger.msg(Arc::ERROR, err.what());
        tls_process_error(logger);
        remove_proxy_file(m_proxy_path);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;

}

