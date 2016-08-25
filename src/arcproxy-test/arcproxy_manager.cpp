#include "arcproxy_manager.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

std::map<pass_destination_type, Arc::PasswordSource*> g_passsources;

#ifdef HAVE_NSS
static void get_default_nssdb_path(std::vector<std::string>& nss_paths) {
    const Arc::User user;
    // The profiles.ini could exist under firefox, seamonkey and thunderbird
    std::vector<std::string> profiles_homes;

#ifndef WIN32
    std::string home_path = user.Home();
#else
    std::string home_path = Glib::get_home_dir();
#endif

    std::string profiles_home;

#if defined(_MACOSX)
    profiles_home = home_path + G_DIR_SEPARATOR_S "Library" G_DIR_SEPARATOR_S "Application Support" G_DIR_SEPARATOR_S "Firefox";
    profiles_homes.push_back(profiles_home);

    profiles_home = home_path + G_DIR_SEPARATOR_S "Library" G_DIR_SEPARATOR_S "Application Support" G_DIR_SEPARATOR_S "SeaMonkey";
    profiles_homes.push_back(profiles_home);

    profiles_home = home_path + G_DIR_SEPARATOR_S "Library" G_DIR_SEPARATOR_S "Thunderbird";
    profiles_homes.push_back(profiles_home);

#elif defined(WIN32)
    //Windows Vista and Win7
    profiles_home = home_path + G_DIR_SEPARATOR_S "AppData" G_DIR_SEPARATOR_S "Roaming" G_DIR_SEPARATOR_S "Mozilla" G_DIR_SEPARATOR_S "Firefox";
    profiles_homes.push_back(profiles_home);

    profiles_home = home_path + G_DIR_SEPARATOR_S "AppData" G_DIR_SEPARATOR_S "Roaming" G_DIR_SEPARATOR_S "Mozilla" G_DIR_SEPARATOR_S "SeaMonkey";
    profiles_homes.push_back(profiles_home);

    profiles_home = home_path + G_DIR_SEPARATOR_S "AppData" G_DIR_SEPARATOR_S "Roaming" G_DIR_SEPARATOR_S "Thunderbird";
    profiles_homes.push_back(profiles_home);

    //WinXP and Win2000
    profiles_home = home_path + G_DIR_SEPARATOR_S "Application Data" G_DIR_SEPARATOR_S "Mozilla" G_DIR_SEPARATOR_S "Firefox";
    profiles_homes.push_back(profiles_home);

    profiles_home = home_path + G_DIR_SEPARATOR_S "Application Data" G_DIR_SEPARATOR_S "Mozilla" G_DIR_SEPARATOR_S "SeaMonkey";
    profiles_homes.push_back(profiles_home);

    profiles_home = home_path + G_DIR_SEPARATOR_S "Application Data" G_DIR_SEPARATOR_S "Thunderbird";
    profiles_homes.push_back(profiles_home);
    Arc::UserConfig m_usercfg
        #else //Linux
    profiles_home = home_path + G_DIR_SEPARATOR_S ".mozilla" G_DIR_SEPARATOR_S "firefox";
    profiles_homes.push_back(profiles_home);

    profiles_home = home_path + G_DIR_SEPARATOR_S ".mozilla" G_DIR_SEPARATOR_S "seamonkey";
    profiles_homes.push_back(profiles_home);

    profiles_home = home_path + G_DIR_SEPARATOR_S ".thunderbird";
    profiles_homes.push_back(profiles_home);
#endif

    std::vector<std::string> pf_homes;
    // Remove the unreachable directories
    for(int i=0; i<profiles_homes.size(); i++) {
        std::string pf_home;
        pf_home = profiles_homes[i];
        struct stat st;
        if(::stat(pf_home.c_str(), &st) != 0) continue;
        if(!S_ISDIR(st.st_mode)) continue;
        if(user.get_uid() != st.st_uid) continue;
        pf_homes.push_back(pf_home);
    }

    // Record the profiles.ini path, and the directory it belongs to.
    std::map<std::string, std::string> ini_home;
    // Remove the unreachable "profiles.ini" files
    for(int i=0; i<pf_homes.size(); i++) {
        std::string pf_file = pf_homes[i] + G_DIR_SEPARATOR_S "profiles.ini";
        struct stat st;
        if(::stat(pf_file.c_str(),&st) != 0) continue;
        if(!S_ISREG(st.st_mode)) continue;
        if(user.get_uid() != st.st_uid) continue;
        ini_home[pf_file] = pf_homes[i];
    }

    // All of the reachable profiles.ini files will be parsed to get
    // the nss configuration m_information (nss db location).
    // All of the m_information about nss db location  will be
    // merged together for users to choose
    std::map<std::string, std::string>::iterator it;
    for(it = ini_home.begin(); it != ini_home.end(); it++) {
        std::string pf_ini = (*it).first;
        std::string pf_home = (*it).second;

        std::string profiles;
        std::ifstream in_f(pf_ini.c_str());
        std::getline<char>(in_f, profiles, '\0');

        std::list<std::string> lines;
        Arc::tokenize(profiles, lines, "\n");

        // Parse each [Profile]
        for (std::list<std::string>::iterator i = lines.begin(); i != lines.end(); ++i) {
            std::vector<std::string> inivalue;
            Arc::tokenize(*i, inivalue, "=");
            if((inivalue[0].find("Profile") != std::string::npos) &&
                    (inivalue[0].find("StartWithLast") == std::string::npos)) {
                bool is_relative = false;
                std::string path;
                std::advance(i, 1);
                for(; i != lines.end();) {
                    inivalue.clear();
                    Arc::tokenize(*i, inivalue, "=");
                    if (inivalue.size() == 2) {
                        if (inivalue[0] == "IsRelative") {
                            if(inivalue[1] == "1") is_relative = true;
                            else is_relative = false;
                        }
                        if (inivalue[0] == "Path") path = inivalue[1];
                    }
                    if(inivalue[0].find("Profile") != std::string::npos) { i--; break; }
                    std::advance(i, 1);
                }
                std::string nss_path;
                if(is_relative) nss_path = pf_home + G_DIR_SEPARATOR_S + path;
                else nss_path = path;

                struct stat st;
                if((::stat(nss_path.c_str(),&st) == 0) && (S_ISDIR(st.st_mode))
                        && (user.get_uid() == st.st_uid))
                    nss_paths.push_back(nss_path);
                if(i == lines.end()) break;
            }
        }
    }
    return;
}

static void get_nss_certname(std::string& certname, Arc::Logger& logger) {
    std::list<ArcAuthNSS::certInfo> certinfolist;
    ArcAuthNSS::nssListUserCertificatesInfo(certinfolist);
    if(certinfolist.size()) {
        std::cout<<Arc::IString("There are %d user certificates existing in the NSS database",
                                certinfolist.size())<<std::endl;
    }
    int n = 1;
    std::list<ArcAuthNSS::certInfo>::iterator it;
    for(it = certinfolist.begin(); it != certinfolist.end(); it++) {
        ArcAuthNSS::certInfo certinfo = (*it);
        std::string sub_dn = certinfo.subject_dn;
        std::string cn_name;
        std::string::size_type pos1, pos2;
        pos1 = sub_dn.find("CN=");
        if(pos1 != std::string::npos) {
            pos2 = sub_dn.find(",", pos1);
            if(pos2 != std::string::npos)
                cn_name = " ("+sub_dn.substr(pos1+3, pos2-pos1-3) + ")";
        }
        std::cout<<Arc::IString("Number %d is with nickname: %s%s", n, certinfo.certname, cn_name)<<std::endl;
        Arc::Time now;
        std::string msg;
        if(now > certinfo.end) msg = "(expired)";
        else if((now + 300) > certinfo.end) msg = "(will be expired in 5 min)";
        else if((now + 3600*24) > certinfo.end) {
            Arc::Period left(certinfo.end - now);
            msg = std::string("(will be expired in ") + std::string(left) + ")";
        }
        std::cout<<Arc::IString("    expiration time: %s ", certinfo.end.str())<<msg<<std::endl;
        //std::cout<<Arc::IString("    certificate dn:  %s", cert_m_info.subject_dn)<<std::endl;
        //std::cout<<Arc::IString("    issuer dn:       %s", cert_m_info.issuer_dn)<<std::endl;
        //std::cout<<Arc::IString("    serial number:   %d", cert_m_info.serial)<<std::endl;
        logger.msg(Arc::INFO, "    certificate dn:  %s", certinfo.subject_dn);
        logger.msg(Arc::INFO, "    issuer dn:       %s", certinfo.issuer_dn);
        logger.msg(Arc::INFO, "    serial number:   %d", certinfo.serial);
        n++;
    }


    std::cout << Arc::IString("Please choose the one you would use (1-%d): ", certinfolist.size());
    if(certinfolist.size() == 1) { it = certinfolist.begin(); certname = (*it).certname; }

    // -------------------------------------------------------------------------------------------------
    // ---- INTERACTIVE --------------------------------------------------------------------------------
    // -------------------------------------------------------------------------------------------------

    char c;
    while(true && (certinfolist.size()>1)) {
        c = getchar();
        int num = c - '0';
        if((num<=certinfolist.size()) && (num>=1)) {
            it = certinfolist.begin();
            std::advance(it, num-1);
            certname = (*it).certname;
            break;
        }
    }

    // -------------------------------------------------------------------------------------------------
    // ---- INTERACTIVE --------------------------------------------------------------------------------
    // -------------------------------------------------------------------------------------------------
}

#endif

static std::string signTypeToString(Arc::Signalgorithm alg) {
    switch(alg) {
    case Arc::SIGN_SHA1: return "sha1";
#if (OPENSSL_m_version_NUMBER >= 0x0090800fL)
    case Arc::SIGN_SHA224: return "sha224";
    case Arc::SIGN_SHA256: return "sha256";
    case Arc::SIGN_SHA384: return "sha384";
    case Arc::SIGN_SHA512: return "sha512";
#endif
    default:
        break;
    }
    return "unknown";
}

ArcProxyManager::ArcProxyManager(int &argc, char **argv)
    :logger(Arc::Logger::getRootLogger(), "arcproxy"), logcerr(std::cerr), m_qtApp(argc, argv)
{

    logcerr.setFormat(Arc::ShortFormat);

    Arc::Logger::getRootLogger().addDestination(logcerr);
    Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

    Arc::ArcLocation::Init(argv[0]);

    Arc::OptionParser options("",
                              istring("The arcproxy command creates a proxy from a key/certificate pair which can\n"
                                      "then be used to access grid resources."),
                              istring("Supported constraints are:\n"
                                      "  validityStart=time (e.g. 2008-05-29T10:20:30Z; if not specified, start from now)\n"
                                      "  validityEnd=time\n"
                                      "  validityPeriod=time (e.g. 43200 or 12h or 12H; if both validityPeriod and validityEnd\n"
                                      "    not specified, the default is 12 hours for local proxy, and 168 hours for delegated\n"
                                      "    proxy on myproxy server)\n"
                                      "  vomsACvalidityPeriod=time (e.g. 43200 or 12h or 12H; if not specified, the default\n"
                                      "    is the minimum value of 12 hours and validityPeriod)\n"
                                      "  myproxyvalidityPeriod=time (lifetime of proxies delegated by myproxy server,\n"
                                      "    e.g. 43200 or 12h or 12H; if not specified, the default is the minimum value of\n"
                                      "    12 hours and validityPeriod (which is lifetime of the delegated proxy on myproxy server))\n"
                                      "  proxyPolicy=policy content\n"
                                      "  proxyPolicyFile=policy file\n"
                                      "  keybits=number - length of the key to generate. Default is 1024 bits.\n"
                                      "    Special value 'inherit' is to use key length of signing certificate.\n"
                                      "  signingAlgorithm=name - signing algorithm to use for signing public key of proxy.\n"
                                      "    Possible values are sha1, sha2 (alias for sha256), sha224, sha256, sha384, sha512\n"
                                      "    and inherit (use algorithm of signing certificate). Default is inherit.\n"
                                      "    With old systems, only sha1 is acceptable.\n"
                                      "\n"
                                      "Supported m_information item names are:\n"
                                      "  subject - subject name of proxy certificate.\n"
                                      "  identity - identity subject name of proxy certificate.\n"
                                      "  issuer - issuer subject name of proxy certificate.\n"
                                      "  ca - subject name of CA ehich issued initial certificate\n"
                                      "  path - file system path to file containing proxy.\n"
                                      "  type - type of proxy certificate.\n"
                                      "  validityStart - timestamp when proxy validity starts.\n"
                                      "  validityEnd - timestamp when proxy validity ends.\n"
                                      "  validityPeriod - duration of proxy validity in seconds.\n"
                                      "  validityLeft - duration of proxy validity left in seconds.\n"
                                      "  vomsVO - VO name  represented by VOMS attribute\n"
                                      "  vomsSubject - subject of certificate for which VOMS attribute is issued\n"
                                      "  vomsIssuer - subject of service which issued VOMS certificate\n"
                                      "  vomsACvalidityStart - timestamp when VOMS attribute validity starts.\n"
                                      "  vomsACvalidityEnd - timestamp when VOMS attribute validity ends.\n"
                                      "  vomsACvalidityPeriod - duration of VOMS attribute validity in seconds.\n"
                                      "  vomsACvalidityLeft - duration of VOMS attribute validity left in seconds.\n"
                                      "  proxyPolicy\n"
                                      "  keybits - size of proxy certificate key in bits.\n"
                                      "  signingAlgorithm - algorithm used to sign proxy certificate.\n"
                                      "Items are printed in requested order and are separated by newline.\n"
                                      "If item has multiple values they are printed in same line separated by |.\n"
                                      "\n"
                                      "Supported password destinations are:\n"
                                      "  key - for reading private key\n"
                                      "  myproxy - for accessing credentials at MyProxy service\n"
                                      "  myproxynew - for creating credentials at MyProxy service\n"
                                      "  all - for any purspose.\n"
                                      "\n"
                                      "Supported password sources are:\n"
                                      "  quoted string (\"password\") - explicitly specified password\n"
                                      "  int - interactively request password from console\n"
                                      "  stdin - read password from standard input delimited by newline\n"
                                      "  file:filename - read password from file named filename\n"
                                      "  stream:# - read password from input stream number #.\n"
                                      "             Currently only 0 (standard input) is supported.\n"
                                      ));

    options.AddOption('P', "proxy", istring("path to the proxy file"),
                      istring("path"), m_proxy_path);

    options.AddOption('C', "cert", istring("path to the certificate file, it can be either PEM, DER, or PKCS12 formated"),
                      istring("path"), m_cert_path);

    options.AddOption('K', "key", istring("path to the private key file, if the certificate is in PKCS12 format, then no need to give private key"),
                      istring("path"), m_key_path);

    options.AddOption('T', "cadir", istring("path to the trusted certificate directory, only needed for the VOMS client functionality"),
                      istring("path"), m_ca_dir);

    options.AddOption('s', "vomsdir", istring("path to the top directory of VOMS *.lsc files, only needed for the VOMS client functionality"),
                      istring("path"), m_voms_dir);

    options.AddOption('V', "vomses", istring("path to the VOMS server configuration file"),
                      istring("path"), m_vomses_path);

    options.AddOption('S', "voms", istring("voms<:command>. Specify VOMS server (More than one VOMS server \n"
                                           "              can be specified like this: --voms VOa:command1 --voms VOb:command2). \n"
                                           "              :command is optional, and is used to ask for specific attributes(e.g: roles)\n"
                                           "              command options are:\n"
                                           "              all --- put all of this DN's attributes into AC; \n"
                                           "              list ---list all of the DN's attribute, will not create AC extension; \n"
                                           "              /Role=yourRole --- specify the role, if this DN \n"
                                           "                               has such a role, the role will be put into AC \n"
                                           "              /voname/groupname/Role=yourRole --- specify the VO, group and role; if this DN \n"
                                           "                               has such a role, the role will be put into AC \n"
                                           ),
                      istring("string"), m_vomslist);

    std::list<std::string> m_orderlist;
    options.AddOption('o', "order", istring("group<:role>. Specify ordering of attributes \n"
                                            "              Example: --order /knowarc.eu/coredev:Developer,/knowarc.eu/testers:Tester \n"
                                            "              or: --order /knowarc.eu/coredev:Developer --order /knowarc.eu/testers:Tester \n"
                                            " Note that it does not make sense to specify the order if you have two or more different VOMS servers specified"),
                      istring("string"), m_orderlist);

    options.AddOption('G', "gsicom", istring("use GSI communication protocol for contacting VOMS services"), m_use_gsi_comm);

    options.AddOption('H', "httpcom", istring("use HTTP communication protocol for contacting VOMS services that provide RESTful access \n"
                                              "               Note for RESTful access, \'list\' command and multiple VOMS server are not supported\n"
                                              ),
                      m_use_http_comm);

    options.AddOption('O', "old", istring("use GSI proxy (RFC 3820 compliant proxy is default)"), m_use_gsi_proxy);

    m_info = false;
    options.AddOption('I', "info", istring("print all m_information about this proxy."), m_info);

    options.AddOption('i', "infoitem", istring("print selected m_information about this proxy."), istring("string"), m_infoitemlist);

    m_remove_proxy = false;
    options.AddOption('r', "remove", istring("remove proxy"), m_remove_proxy);

    options.AddOption('U', "user", istring("username to MyProxy server (if missing subject of user certificate is used)"),
                      istring("string"), m_user_name);

    options.AddOption('N', "nopassphrase", istring(
                          "don't prompt for a credential passphrase, when retrieve a \n"
                          "              credential from on MyProxy server. \n"
                          "              The precondition of this choice is the credential is PUT onto\n"
                          "              the MyProxy server without a passphrase by using -R (--m_retrievable_by_cert) \n"
                          "              option when being PUTing onto Myproxy server. \n"
                          "              This option is specific for the GET command when contacting Myproxy server."
                          ),
                      m_use_empty_passphrase);

    options.AddOption('R', "m_retrievable_by_cert", istring(
                          "Allow specified entity to retrieve credential without passphrase.\n"
                          "              This option is specific for the PUT command when contacting Myproxy server."
                          ),
                      istring("string"), m_retrievable_by_cert);

    options.AddOption('L', "myproxysrv", istring("hostname[:port] of MyProxy server"),
                      istring("string"), m_myproxy_server);

    options.AddOption('M', "myproxycmd", istring(
                          "command to MyProxy server. The command can be PUT, GET, m_info, NEWPASS or DESTROY.\n"
                          "              PUT -- put a delegated credentials to the MyProxy server; \n"
                          "              GET -- get a delegated credentials from the MyProxy server; \n"
                          "              m_info -- get and present m_information about credentials stored at the MyProxy server; \n"
                          "              NEWPASS -- change password protecting credentials stored at the MyProxy server; \n"
                          "              DESTROY -- wipe off credentials stored at the MyProxy server; \n"
                          "              Local credentials (certificate and key) are not necessary except in case of PUT. \n"
                          "              MyProxy functionality can be used together with VOMS functionality.\n"
                          "              --voms and --vomses can be used for Get command if VOMS attributes\n"
                          "              is required to be included in the proxy.\n"
                          ),
                      istring("string"), m_myproxy_command);

    m_use_nssdb = false;
#ifdef HAVE_NSS
    options.AddOption('F', "nssdb", istring("use NSS credential database in default Mozilla profiles, \n"
                                            "              including Firefox, Seamonkey and Thunderbird.\n"), m_use_nssdb);
#endif

    options.AddOption('c', "constraint", istring("proxy constraints"),
                      istring("string"), m_constraintlist);

    options.AddOption('p', "passwordsource", istring("password destination=password source"),
                      istring("string"), m_passsourcelist);

    int m_timeout = -1;
    options.AddOption('t', "m_timeout", istring("m_timeout in seconds (default 20)"),
                      istring("seconds"), m_timeout);

    options.AddOption('z', "m_conffile",
                      istring("configuration file (default ~/.arc/client.conf)"),
                      istring("filename"), m_conffile);

    options.AddOption('d', "debug",
                      istring("FATAL, ERROR, WARNING, m_info, VERBOSE or DEBUG"),
                      istring("debuglevel"), m_debug);

    options.AddOption('v', "m_version", istring("print m_version m_information"),
                      m_version);

    std::list<std::string> params = options.Parse(argc, argv);

    if (m_version) {
        std::cout << Arc::IString("%s m_version %s", "arcproxy", VERSION) << std::endl;
        //return EXIT_SUCCESS;
    }

    // If debug is specified as argument, it should be set before loading the configuration.
    if (!m_debug.empty())
        Arc::Logger::getRootLogger().setThreshold(Arc::istring_to_level(m_debug));

    logger.msg(Arc::VERBOSE, "Running command: %s", options.GetCommandWithArguments());

    // This ensure command line args overwrite all other options
    if(!m_cert_path.empty())Arc::SetEnv("X509_USER_CERT", m_cert_path);
    if(!m_key_path.empty())Arc::SetEnv("X509_USER_KEY", m_key_path);
    if(!m_proxy_path.empty())Arc::SetEnv("X509_USER_PROXY", m_proxy_path);
    if(!m_ca_dir.empty())Arc::SetEnv("X509_CERT_DIR", m_ca_dir);
}

std::string ArcProxyManager::proxyPath()
{
    return m_proxy_path;
}

std::string ArcProxyManager::certPath()
{
    return m_cert_path;
}

bool ArcProxyManager::useHttpComm()
{
    return m_use_http_comm;
}

bool ArcProxyManager::useGsiProxy()
{
    return m_use_gsi_proxy;
}

bool ArcProxyManager::showInfoOpt()
{
    return m_info;
}

bool ArcProxyManager::removeProxyOpt()
{
    return m_remove_proxy;
}

bool ArcProxyManager::showVersionOpt()
{
    return m_version;
}

std::string ArcProxyManager::userName()
{
    return m_user_name;
}

bool ArcProxyManager::useEmptyPassphrase()
{
    return m_use_empty_passphrase;
}

std::string ArcProxyManager::retrievableByCert()
{
    return m_retrievable_by_cert;
}

std::string ArcProxyManager::myproxyServer()
{
    return m_myproxy_server;
}

std::string ArcProxyManager::myproxyCommand()
{
    return m_myproxy_command;
}

bool ArcProxyManager::useNssDB()
{
    return m_use_nssdb;
}

int ArcProxyManager::timeout()
{
    return m_timeout;
}

std::string ArcProxyManager::conffile()
{
    return m_conffile;
}

std::string ArcProxyManager::debugLevel()
{
    return m_debug;
}

bool ArcProxyManager::init()
{
    Arc::UserConfig usercfg(m_conffile, Arc::initializeCredentialsType(Arc::initializeCredentialsType::TryCredentials));

    if (!usercfg) {
        logger.msg(Arc::ERROR, "Failed configuration initialization.");
        return EXIT_FAILURE;
    }

    if(m_use_nssdb) {
        usercfg.CertificatePath("");;
        usercfg.KeyPath("");;
    }

    // Check for needed credentials objects
    // Can proxy be used for? Could not find it in documentation.
    // Key and certificate not needed if only printing proxy m_information
    if ( (!(Arc::lower(m_myproxy_command) == "get")) && (!m_use_nssdb) ) {
        if((usercfg.CertificatePath().empty() ||
            (
                usercfg.KeyPath().empty() &&
                (usercfg.CertificatePath().find(".p12") == std::string::npos)
                )
            ) && !(m_info || (m_infoitemlist.size() > 0) || m_remove_proxy)) {
            logger.msg(Arc::ERROR, "Failed to find certificate and/or private key or files have improper permissions or ownership.");
            logger.msg(Arc::ERROR, "You may try to increase verbosity to get more m_information.");
            return EXIT_FAILURE;
        }
    }

    if(!m_vomslist.empty() || !m_myproxy_command.empty()) {
        // For external communication CAs are needed
        if(usercfg.CACertificatesDirectory().empty()) {
            logger.msg(Arc::ERROR, "Failed to find CA certificates");
            logger.msg(Arc::ERROR, "Cannot find the CA certificates directory path, "
                                   "please set environment variable X509_CERT_DIR, "
                                   "or cacertificatesdirectory in a configuration file.");
            logger.msg(Arc::ERROR, "You may try to increase verbosity to get more m_information.");
            logger.msg(Arc::ERROR, "The CA certificates directory is required for "
                                   "contacting VOMS and MyProxy servers.");
            return EXIT_FAILURE;
        }
    }

    // Convert list of voms+command into more convenient structure

    m_vomscmdlist.clear();

    if (!m_vomslist.empty()) {
        if (m_vomses_path.empty()) m_vomses_path = usercfg.VOMSESPath();
        if (m_vomses_path.empty()) {
            logger.msg(Arc::ERROR,
                       "$X509_VOMS_FILE, and $X509_VOMSES are not set;\n"
                       "User has not specified the location for vomses m_information;\n"
                       "There is also not vomses location m_information in user's configuration file;\n"
                       "Can not find vomses in default locations: ~/.arc/vomses, ~/.voms/vomses,\n"
                       "$ARC_LOCATION/etc/vomses, $ARC_LOCATION/etc/grid-security/vomses, $PWD/vomses,\n"
                       "/etc/vomses, /etc/grid-security/vomses, and the location at the corresponding sub-directory");
            return false;
        }
        for(std::list<std::string>::iterator v = m_vomslist.begin(); v != m_vomslist.end(); ++v) {
            std::string::size_type p = v->find(':');
            if(p == std::string::npos) {
                m_vomscmdlist[*v].push_back("");
            } else {
                m_vomscmdlist[v->substr(0,p)].push_back(v->substr(p+1));
                *v = v->substr(0,p);
            }
        }
        // Remove duplicates
        m_vomslist.sort(); m_vomslist.unique();
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
    if(m_voms_dir.empty()) m_voms_dir = Arc::GetEnv("X509_m_voms_dir");

    if (m_debug.empty() && !usercfg.Verbosity().empty())
        Arc::Logger::getRootLogger().setThreshold(Arc::istring_to_level(usercfg.Verbosity()));


    //if (!params.empty()) {
    //    logger.msg(Arc::ERROR, "Wrong number of arguments!");
    //    return EXIT_FAILURE;
    //}
}

int ArcProxyManager::removeProxy()
{
    if (m_proxy_path.empty()) {
        logger.msg(Arc::ERROR, "Cannot find the path of the proxy file, "
                               "please setup environment X509_USER_PROXY, "
                               "or proxypath in a configuration file");
        return EXIT_FAILURE;
    }
    if(!Arc::FileDelete(m_proxy_path)) {
        if(errno != ENOENT) {
            logger.msg(Arc::ERROR, "Cannot remove proxy file at %s", m_proxy_path);
        } else {
            logger.msg(Arc::ERROR, "Cannot remove proxy file at %s, because it's not there", m_proxy_path);
        }
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int ArcProxyManager::showInfo()
{
    Arc::Time now;

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
    std::cout << Arc::IString("Proxy key length: %i", holder.GetKeybits()) << std::endl;
    std::cout << Arc::IString("Proxy signature: %s", signTypeToString(holder.GetSigningAlgorithm())) << std::endl;

    Arc::VOMSTrustList voms_trust_dn;
    voms_trust_dn.AddRegex(".*");
    res = parseVOMSAC(holder, m_ca_dir, "", m_voms_dir, voms_trust_dn, voms_attributes, true, true);
    // Not printing error message because parseVOMSAC will print everything itself
    //if (!res) logger.msg(Arc::ERROR, "VOMS attribute parsing failed");
    for(int n = 0; n<voms_attributes.size(); ++n) {
        if(voms_attributes[n].attributes.size() > 0) {
            std::cout<<"====== "<<Arc::IString("AC extension m_information for VO ")<<
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
                    //passsources[pass_nss] = pass_source;
                }
                std::cout << std::endl;
            }
            std::cout << "VO        : "<<voms_attributes[n].voname << std::endl;
            std::cout << "subject   : "<<voms_attributes[n].holder << std::endl;
            std::cout << "issuer    : "<<voms_attributes[n].issuer << std::endl;
            bool found_uri = false;
            for(int i = 0; i < voms_attributes[n].attributes.size(); i++) {
                std::string attr = voms_attributes[n].attributes[i];
                std::string::size_type pos;
                if((pos = attr.find("hostname=")) != std::string::npos) {
                    if(found_uri) continue;
                    std::string str = attr.substr(pos+9);
                    std::string::size_type pos1 = str.find("/");
                    if(pos1 != std::string::npos) str = str.substr(0, pos1);
                    std::cout << "uri       : " << str <<std::endl;
                    found_uri = true;
                }
                else {
                    if(attr.find("Role=") == std::string::npos)
                        std::cout << "attribute : " << attr <<"/Role=NULL/Capability=NULL" <<std::endl;
                    else if(attr.find("Capability=") == std::string::npos)
                        std::cout << "attribute : " << attr <<"/Capability=NULL" <<std::endl;
                    else
                        std::cout << "attribute : " << attr <<std::endl;
                }

                if((pos = attr.find("Role=")) != std::string::npos) {
                    std::string str = attr.substr(pos+5);
                    std::cout << "attribute : role = " << str << " (" << voms_attributes[n].voname << ")"<<std::endl;
                }

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

int ArcProxyManager::showInfoList()
{
    if(m_infoitemlist.size() > 0)
    {
        Arc::Time now;
        std::vector<Arc::VOMSACInfo> voms_attributes;
        Arc::Credential holder(m_proxy_path, "", "", "");
        Arc::VOMSTrustList voms_trust_dn;
        voms_trust_dn.AddRegex(".*");
        parseVOMSAC(holder, m_ca_dir, "", m_voms_dir, voms_trust_dn, voms_attributes, true, true);
        for(std::list<std::string>::iterator ii = m_infoitemlist.begin();
            ii != m_infoitemlist.end(); ++ii) {
            if(*ii == "subject") {
                std::cout << holder.GetDN() << std::endl;
            } else if(*ii == "identity") {
                std::cout << holder.GetIdentityName() << std::endl;
            } else if(*ii == "issuer") {
                std::cout << holder.GetIssuerName() << std::endl;
            } else if(*ii == "ca") {
                std::cout << holder.GetCAName() << std::endl;
            } else if(*ii == "path") {
                std::cout << m_proxy_path << std::endl;
            } else if(*ii == "type") {
                std::cout << certTypeToString(holder.GetType()) << std::endl; // todo:less human readable
            } else if(*ii == "validityStart") {
                std::cout << holder.GetStartTime().GetTime() << std::endl;
            } else if(*ii == "validityEnd") {
                std::cout << holder.GetEndTime().GetTime() << std::endl;
            } else if(*ii == "validityPeriod") {
                std::cout << (holder.GetEndTime() - holder.GetStartTime()).GetPeriod() << std::endl;
            } else if(*ii == "validityLeft") {
                std::cout << ((now<holder.GetEndTime())?(holder.GetEndTime()-now):Arc::Period(0)).GetPeriod() << std::endl;
            } else if(*ii == "vomsVO") {
                for(int n = 0; n<voms_attributes.size(); ++n) {
                    if(n) std::cout << "|";
                    std::cout << voms_attributes[n].voname;
                }
                std::cout << std::endl;
            } else if(*ii == "vomsSubject") {
                for(int n = 0; n<voms_attributes.size(); ++n) {
                    if(n) std::cout << "|";
                    std::cout << voms_attributes[n].holder;
                }
                std::cout << std::endl;
            } else if(*ii == "vomsIssuer") {
                for(int n = 0; n<voms_attributes.size(); ++n) {
                    if(n) std::cout << "|";
                    std::cout << voms_attributes[n].issuer;
                }
                std::cout << std::endl;
            } else if(*ii == "vomsACvalidityStart") {
                for(int n = 0; n<voms_attributes.size(); ++n) {
                    if(n) std::cout << "|";
                    std::cout << voms_attributes[n].from.GetTime();
                }
                std::cout << std::endl;
            } else if(*ii == "vomsACvalidityEnd") {
                for(int n = 0; n<voms_attributes.size(); ++n) {
                    if(n) std::cout << "|";
                    std::cout << voms_attributes[n].till.GetTime();
                }
                std::cout << std::endl;
            } else if(*ii == "vomsACvalidityPeriod") {
                for(int n = 0; n<voms_attributes.size(); ++n) {
                    if(n) std::cout << "|";
                    std::cout << (voms_attributes[n].till-voms_attributes[n].from).GetPeriod();
                }
                std::cout << std::endl;
            } else if(*ii == "vomsACvalidityLeft") {
                for(int n = 0; n<voms_attributes.size(); ++n) {
                    if(n) std::cout << "|";
                    std::cout << ((voms_attributes[n].till>now)?(voms_attributes[n].till-now):Arc::Period(0)).GetPeriod();
                }
                std::cout << std::endl;
            } else if(*ii == "proxyPolicy") {
                std::cout << holder.GetProxyPolicy() << std::endl;
            } else if(*ii == "keybits") {
                std::cout << holder.GetKeybits() << std::endl;
            } else if(*ii == "signingAlgorithm") {
                std::cout << signTypeToString(holder.GetSigningAlgorithm()) << std::endl;
            } else {
                logger.msg(Arc::ERROR, "m_information item '%s' is not known",*ii);
            }
        }
        return EXIT_SUCCESS;
    }
}

int ArcProxyManager::createProxy()
{
    Arc::UserConfig usercfg(m_conffile, Arc::initializeCredentialsType(Arc::initializeCredentialsType::TryCredentials));

    if ((m_cert_path.empty() || m_key_path.empty()) &&
            (Arc::lower(m_myproxy_command) == "put")) {
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

    std::map<pass_destination_type, std::pair<std::string,bool> > passprompts;
    passprompts[pass_private_key] = std::pair<std::string,bool>("Enter password for private key",false);
    passprompts[pass_nss] = std::pair<std::string,bool>("Enter password for NSS security device",false);
    passprompts[pass_myproxy] = std::pair<std::string,bool>("MyProxy server",false);
    passprompts[pass_myproxy_new] = std::pair<std::string,bool>("MyProxy server (new)",true);
    for (std::list<std::string>::iterator it = m_passsourcelist.begin();
         it != m_passsourcelist.end(); it++) {
        std::string::size_type pos = it->find('=');
        if (pos == std::string::npos) {
            logger.msg(Arc::ERROR, "Cannot parse password source expression %s "
                                   "it must be of type=source format", *it);
            return EXIT_FAILURE;
        }
        std::string dest = it->substr(0, pos);
        pass_destination_type pass_dest;
        if(dest == "key") {
            pass_dest = pass_private_key;
        } else if(dest == "myproxy") {
            pass_dest = pass_myproxy;
        } else if(dest == "myproxynew") {
            pass_dest = pass_myproxy_new;
        } else if(dest == "nss") {
            pass_dest = pass_nss;
        } else if(dest == "all") {
            pass_dest = pass_all;
        } else {
            logger.msg(Arc::ERROR, "Cannot parse password type %s. "
                                   "Currently supported values are 'key','myproxy','myproxynew' and 'all'.", dest);
            return EXIT_FAILURE;
        }
        Arc::PasswordSource* pass_source;
        std::string pass = it->substr(pos + 1);
        if((pass[0] == '"') && (pass[pass.length()-1] == '"')) {
            pass_source = new Arc::PasswordSourceString(pass.substr(1,pass.length()-2));
        } else if(pass == "int") {
            if (pass_dest == pass_nss)
                pass_source = new Arc::PasswordSourceInteractive(passprompts[pass_nss].first,passprompts[pass_nss].second);
            else
                pass_source = new Arc::PasswordSourceInteractive(passprompts[pass_private_key].first,passprompts[pass_private_key].second);
#ifdef USE_GUI
        } else if(pass == "gui") {
            if (pass_dest == pass_nss)
                pass_source = new PasswordSourceDialog(passprompts[pass_nss].first,passprompts[pass_nss].second);
            else
                pass_source = new PasswordSourceDialog(passprompts[pass_private_key].first,passprompts[pass_private_key].second);
#endif
        } else if(pass == "stdin") {
            pass_source = new Arc::PasswordSourceStream(&std::cin);
        } else {
            pos = pass.find(':');
            if(pos == std::string::npos) {
                logger.msg(Arc::ERROR, "Cannot parse password source %s "
                                       "it must be of source_type or source_type:data format. "
                                       "Supported source types are int,stdin,stream,file.", pass);
                return EXIT_FAILURE;
            }
            std::string data = pass.substr(pos + 1);
            pass.resize(pos);
            if(pass == "file") {
                pass_source = new PasswordSourceFile(data);
                // TODO: combine same files
            } else if(pass == "stream") {
                if(data == "0") {
                    pass_source = new Arc::PasswordSourceStream(&std::cin);
                } else {
                    logger.msg(Arc::ERROR, "Only standard input is currently supported "
                                           "for password source.");
                    return EXIT_FAILURE;
                }
            } else {
                logger.msg(Arc::ERROR, "Cannot parse password source type %s. "
                                       "Supported source types are int,stdin,stream,file.", pass);
                return EXIT_FAILURE;
            }
        }
        if(pass_source) {
            if(pass_dest != pass_all) {
                g_passsources[pass_dest] = pass_source;
            } else {
                g_passsources[pass_private_key] = pass_source;
                g_passsources[pass_myproxy] = pass_source;
                g_passsources[pass_myproxy_new] = pass_source;
                g_passsources[pass_nss] = pass_source;
            }
        }
    }
    for(std::map<pass_destination_type, std::pair<std::string,bool> >::iterator p = passprompts.begin();
        p != passprompts.end();++p) {
        if(g_passsources.find(p->first) == g_passsources.end()) {
            g_passsources[p->first] = new Arc::PasswordSourceInteractive(p->second.first,p->second.second);
        }
    }

    Arc::Time now;

    //proxy validity period
    //Set the default proxy validity lifetime to 12 hours if there is
    //no validity lifetime provided by command caller
    // Set default values first
    // TODO: Is default validityPeriod since now or since validityStart?
    Arc::Time validityStart = now; // now by default
    Arc::Period validityPeriod(12*60*60);
    if (Arc::lower(m_myproxy_command) == "put") {
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
            // If period is explicitly set then start is derived from end and period
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

    std::string signing_algorithm = constraints["signingAlgorithm"];
    int keybits = 0;
    if(!constraints["keybits"].empty()) {
        if(constraints["keybits"] == "inherit") {
            keybits = -1;
        } else if((!Arc::stringto(constraints["keybits"],keybits)) || (keybits <= 0)) {
            std::cerr << Arc::IString("The keybits constraint is wrong: %s.", (std::string)constraints["keybits"]) << std::endl;
            return EXIT_FAILURE;
        }
    }


#ifdef HAVE_NSS
    // TODO: move to spearate file
    //Using nss db dominate other option
    if(m_use_nssdb) {
        // Get the nss db paths from firefox's profile.ini file
        std::vector<std::string> nssdb_paths;
        get_default_nssdb_path(nssdb_paths);
        if(nssdb_paths.empty()) {
            std::cout << Arc::IString("The NSS database can not be detected in the Firefox profile") << std::endl;
            return EXIT_FAILURE;
        }

        // Let user to choose which profile to use
        // if multiple profiles exist
        bool res;
        std::string configdir;
        if(nssdb_paths.size() > 1) {
            std::cout<<Arc::IString("There are %d NSS base directories where the certificate, key, and module databases live", nssdb_paths.size())<<std::endl;
            for(int i=0; i < nssdb_paths.size(); i++) {
                std::cout<<Arc::IString("Number %d is: %s", i+1, nssdb_paths[i])<<std::endl;
            }
            std::cout << Arc::IString("Please choose the NSS database you would like to use (1-%d): ", nssdb_paths.size());
            // todo: more than 9 paths
            while(true) {
                char c;
                c = getchar();
                int num = c - '0';
                if((num<=nssdb_paths.size()) && (num>=1)) {
                    configdir = nssdb_paths[num-1];
                    break;
                }
            }
        } else {
            configdir = nssdb_paths[0];
        }

        res = ArcAuthNSS::nssInit(configdir);
        std::cout<< Arc::IString("NSS database to be accessed: %s\n", configdir.c_str());

        //The nss db under firefox profile seems to not be protected by any passphrase by default
        bool ascii = true;
        const char* trusts = "u,u,u";

        // Generate CSR
        std::string proxy_csrfile = "proxy.csr";
        std::string proxy_keyname = "proxykey";
        std::string proxy_privk_str;
        res = ArcAuthNSS::nssGenerateCSR(proxy_keyname, "CN=Test,OU=ARC,O=EMI", *g_passsources[pass_nss], proxy_csrfile, proxy_privk_str, ascii);
        if(!res) return EXIT_FAILURE;

        // Create a temporary proxy and contact voms server
        std::string issuername;
        std::string vomsacseq;
        if (!m_vomslist.empty()) {

            std::string tmp_m_proxy_path;
            if(!Arc::TmpFileCreate(tmp_m_proxy_path,"")) return EXIT_FAILURE;

            get_nss_certname(issuername, logger);

            // Create tmp proxy cert
            int duration = 12;
            res = ArcAuthNSS::nssCreateCert(proxy_csrfile, issuername, NULL, duration, "", tmp_m_proxy_path, ascii);
            if(!res) {
                remove_proxy_file(tmp_m_proxy_path);
                return EXIT_FAILURE;
            }
            // TODO: Use FileUtils
            std::string tmp_proxy_cred_str;
            std::ifstream tmp_proxy_cert_s(tmp_m_proxy_path.c_str());
            std::getline(tmp_proxy_cert_s, tmp_proxy_cred_str,'\0');
            tmp_proxy_cert_s.close();

            // Export EEC
            std::string cert_file;
            if(!Arc::TmpFileCreate(cert_file,"")) {
                remove_proxy_file(tmp_m_proxy_path);
                return EXIT_FAILURE;
            }
            res = ArcAuthNSS::nssExportCertificate(issuername, cert_file);
            if(!res) {
                remove_cert_file(cert_file);
                remove_proxy_file(tmp_m_proxy_path);
                return EXIT_FAILURE;
            }
            std::string eec_cert_str;
            std::ifstream eec_s(cert_file.c_str());
            std::getline(eec_s, eec_cert_str,'\0');
            eec_s.close();
            remove_cert_file(cert_file);

            // Compose tmp proxy file
            tmp_proxy_cred_str.append(proxy_privk_str).append(eec_cert_str);
            write_proxy_file(tmp_m_proxy_path, tmp_proxy_cred_str);

            if(!contact_voms_servers(m_vomscmdlist, m_orderlist, m_vomses_path, m_use_gsi_comm,
                                     m_use_http_comm, voms_period, usercfg, logger, tmp_m_proxy_path, vomsacseq)) {
                remove_proxy_file(tmp_m_proxy_path);
                return EXIT_FAILURE;
            }
            remove_proxy_file(tmp_m_proxy_path);
        }

        // Create proxy with VOMS AC
        std::string proxy_certfile = "myproxy.pem";

        // Let user to choose which credential to use
        if(issuername.empty()) get_nss_certname(issuername, logger);
        std::cout<<Arc::IString("Certificate to use is: %s", issuername)<<std::endl;

        int duration;
        duration = validityPeriod.GetPeriod() / 3600;

        std::string vomsacseq_asn1;
        if(!vomsacseq.empty()) Arc::VOMSACSeqEncode(vomsacseq, vomsacseq_asn1);
        res = ArcAuthNSS::nssCreateCert(proxy_csrfile, issuername, "", duration, vomsacseq_asn1, proxy_certfile, ascii);
        if(!res) return EXIT_FAILURE;

        const char* proxy_certname = "proxycert";
        res = ArcAuthNSS::nssImportCert(*g_passsources[pass_nss], proxy_certfile, proxy_certname, trusts, ascii);
        if(!res) return EXIT_FAILURE;

        //Compose the proxy certificate
        if(!m_proxy_path.empty())Arc::SetEnv("X509_USER_PROXY", m_proxy_path);
        Arc::UserConfig m_usercfg(m_conffile,
                                  Arc::initializeCredentialsType(Arc::initializeCredentialsType::NotTryCredentials));
        if (!m_usercfg) {
            logger.msg(Arc::ERROR, "Failed configuration initialization.");
            return EXIT_FAILURE;
        }
        if(m_proxy_path.empty()) m_proxy_path = m_usercfg.ProxyPath();
        m_usercfg.ProxyPath(m_proxy_path);
        std::string cert_file = "cert.pem";
        res = ArcAuthNSS::nssExportCertificate(issuername, cert_file);
        if(!res) return EXIT_FAILURE;

        std::string proxy_cred_str;
        std::ifstream proxy_s(proxy_certfile.c_str());
        std::getline(proxy_s, proxy_cred_str,'\0');
        proxy_s.close();
        // Remove the proxy file, because the content
        // is recorded and then writen into proxy path,
        // together with private key, and the issuer of proxy.
        remove_proxy_file(proxy_certfile);

        std::string eec_cert_str;
        std::ifstream eec_s(cert_file.c_str());
        std::getline(eec_s, eec_cert_str,'\0');
        eec_s.close();
        remove_cert_file(cert_file);

        proxy_cred_str.append(proxy_privk_str).append(eec_cert_str);
        write_proxy_file(m_proxy_path, proxy_cred_str);

        Arc::Credential proxy_cred(m_proxy_path, m_proxy_path, "", "");
        Arc::Time left = proxy_cred.GetEndTime();
        std::cout << Arc::IString("Proxy generation succeeded") << std::endl;
        std::cout << Arc::IString("Your proxy is valid until: %s", left.str(Arc::UserTime)) << std::endl;

        return EXIT_SUCCESS;
    }
#endif


    Arc::OpenSSLInit();

    Arc::Time proxy_start = validityStart;
    Arc::Period proxy_period = validityPeriod;
    if (constraints["validityStart"].empty() && constraints["validityEnd"].empty()) {
        // If start/end is not explicitly specified then add 5 min back gap.
        proxy_start = proxy_start - Arc::Period(300);
        proxy_period.SetPeriod(proxy_period.GetPeriod() + 300);
    }
    std::string policy = constraints["proxyPolicy"].empty() ? constraints["proxyPolicyFile"] : constraints["proxyPolicy"];

    if (!m_myproxy_command.empty() && (Arc::lower(m_myproxy_command) != "put")) {
        bool res = contact_myproxy_server(m_myproxy_server, m_myproxy_command,
                                            m_user_name, m_use_empty_passphrase, myproxy_period, m_retrievable_by_cert,
                                            proxy_start, proxy_period, m_vomslist, m_vomses_path, m_proxy_path, usercfg, logger);
        if (res && (Arc::lower(m_myproxy_command) == "get")) {
            // IF the myproxy command is "Get", and voms command is given,
            // then we need to check if the proxy returned from myproxy server
            // includes VOMS AC, if not, we will use the returned proxy to
            // directly contact VOMS server to generate a proxy-on-proxy with
            // VOMS AC included.
            Arc::Credential holder(m_proxy_path, "", "", "");
            Arc::VOMSTrustList voms_trust_dn;
            voms_trust_dn.AddRegex(".*");
            std::vector<Arc::VOMSACInfo> voms_attributes;
            bool r = parseVOMSAC(holder, m_ca_dir, "", m_voms_dir, voms_trust_dn, voms_attributes, true, true);
            if (!r) logger.msg(Arc::ERROR, "VOMS attribute parsing failed");
            if(voms_attributes.size() == 0) {
                logger.msg(Arc::INFO, "Myproxy server did not return proxy with VOMS AC included");
                std::string vomsacseq;
                contact_voms_servers(m_vomscmdlist, m_orderlist, m_vomses_path, m_use_gsi_comm,
                                     m_use_http_comm, voms_period, usercfg, logger, m_proxy_path, vomsacseq);
                if(!vomsacseq.empty()) {
                    Arc::Credential signer(m_proxy_path, m_proxy_path, "", "");
                    std::string proxy_cert;
                    create_proxy(proxy_cert, signer, policy, proxy_start, proxy_period,
                                 vomsacseq, m_use_gsi_proxy, keybits, signing_algorithm);
                    write_proxy_file(m_proxy_path, proxy_cert);
                }
            }
            return EXIT_SUCCESS;
        }
        else return EXIT_FAILURE;
    }

    //Create proxy or voms proxy
    try {
        Arc::Credential signer(m_cert_path, m_key_path, "", "", *g_passsources[pass_private_key]);
        if (signer.GetIdentityName().empty()) {
            std::cerr << Arc::IString("Proxy generation failed: No valid certificate found.") << std::endl;
            return EXIT_FAILURE;
        }
#ifndef WIN32
        EVP_PKEY* pkey = signer.GetPrivKey();
        if(!pkey) {
            std::cerr << Arc::IString("Proxy generation failed: No valid private key found.") << std::endl;
            return EXIT_FAILURE;
        }
        if(pkey) EVP_PKEY_free(pkey);
#endif
        std::cout << Arc::IString("Your identity: %s", signer.GetIdentityName()) << std::endl;
        if (now > signer.GetEndTime()) {
            std::cerr << Arc::IString("Proxy generation failed: Certificate has expired.") << std::endl;
            return EXIT_FAILURE;
        }
        else if (now < signer.GetStartTime()) {
            std::cerr << Arc::IString("Proxy generation failed: Certificate is not valid yet.") << std::endl;
            return EXIT_FAILURE;
        }

        std::string vomsacseq;
        if (!m_vomslist.empty()) {
            //Generate a temporary self-signed proxy certificate
            //to contact the voms server
            std::string tmp_m_proxy_path;
            std::string tmp_proxy;
            if(!Arc::TmpFileCreate(tmp_m_proxy_path,"")) {
                std::cerr << Arc::IString("Proxy generation failed: Failed to create temporary file.") << std::endl;
                return EXIT_FAILURE;
            }
            create_tmp_proxy(tmp_proxy, signer);
            write_proxy_file(tmp_m_proxy_path, tmp_proxy);
            if(!contact_voms_servers(m_vomscmdlist, m_orderlist, m_vomses_path, m_use_gsi_comm,
                                     m_use_http_comm, voms_period, usercfg, logger, tmp_m_proxy_path, vomsacseq)) {
                remove_proxy_file(tmp_m_proxy_path);
                std::cerr << Arc::IString("Proxy generation failed: Failed to retrieve VOMS m_information.") << std::endl;
                return EXIT_FAILURE;
            }
            remove_proxy_file(tmp_m_proxy_path);
        }

        std::string proxy_cert;
        create_proxy(proxy_cert, signer, policy, proxy_start, proxy_period,
                     vomsacseq, m_use_gsi_proxy, keybits, signing_algorithm);

        //If myproxy command is "Put", then the proxy path is set to /tmp/myproxy-proxy.uid.pid
        if (Arc::lower(m_myproxy_command) == "put")
            m_proxy_path = Glib::build_filename(Glib::get_tmp_dir(), "myproxy-proxy."
                                                + Arc::tostring(m_user.get_uid()) + Arc::tostring((int)(getpid())));
        write_proxy_file(m_proxy_path,proxy_cert);

        Arc::Credential proxy_cred(m_proxy_path, m_proxy_path, "", "");
        Arc::Time left = proxy_cred.GetEndTime();
        std::cout << Arc::IString("Proxy generation succeeded") << std::endl;
        std::cout << Arc::IString("Your proxy is valid until: %s", left.str(Arc::UserTime)) << std::endl;

        //return EXIT_SUCCESS;
    } catch (std::exception& err) {
        logger.msg(Arc::ERROR, err.what());
        return EXIT_FAILURE;
    }

    //Delegate the former self-delegated credential to
    //myproxy server
    if (Arc::lower(m_myproxy_command) == "put") {
        bool res = contact_myproxy_server( m_myproxy_server, m_myproxy_command,
                                             m_user_name, m_use_empty_passphrase, myproxy_period, m_retrievable_by_cert,
                                             proxy_start, proxy_period, m_vomslist, m_vomses_path, m_proxy_path, usercfg, logger);
        if(res) return EXIT_SUCCESS;
        else return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

bool ArcProxyManager::hasValidProxy()
{
    Arc::Time now;

    std::vector<Arc::VOMSACInfo> voms_attributes;
    bool res = false;

    if (m_proxy_path.empty()) {
        logger.msg(Arc::ERROR, "Cannot find the path of the proxy file, "
                               "please setup environment X509_USER_PROXY, "
                               "or proxypath in a configuration file");
        return false;
    }
    else if (!(Glib::file_test(m_proxy_path, Glib::FILE_TEST_EXISTS))) {
        logger.msg(Arc::ERROR, "Cannot find file at %s for getting the proxy. "
                               "Please make sure this file exists.", m_proxy_path);
        return false;
    }

    Arc::Credential holder(m_proxy_path, "", "", "");

    if (holder.GetEndTime() < now)
    {
        std::cout << Arc::IString("Time left for proxy: Proxy expired") << std::endl;
        return false;
    }
    else if (now < holder.GetStartTime())
    {
        std::cout << Arc::IString("Time left for proxy: Proxy not valid yet") << std::endl;
        return false;
    }
    else
    {
        std::cout << Arc::IString("Time left for proxy: %s", (holder.GetEndTime() - now).istr()) << std::endl;
        return true;
    }

}
