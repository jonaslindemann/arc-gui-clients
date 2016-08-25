#ifndef __arcproxytest_h__
#define __arcproxytest_h__

#include <map>

#include <arc/ArcLocation.h>
#include <arc/OptionParser.h>
#include <arc/StringConv.h>
#include <arc/Utils.h>
#include <arc/UserConfig.h>
#include <arc/FileUtils.h>
#include <arc/communication/ClientInterface.h>
#include <arc/credentialstore/ClientVOMS.h>
#include <arc/credentialstore/ClientVOMSRESTful.h>
#include <arc/credential/VOMSConfig.h>
#include <arc/credential/VOMSUtil.h>
#include <arc/credential/Credential.h>
#include <arc/credentialstore/CredentialStore.h>
#include <arc/crypto/OpenSSL.h>

#include <arc/credential/NSSUtil.h>

typedef enum {
    pass_all,
    pass_private_key,
    pass_myproxy,
    pass_myproxy_new,
    pass_nss
} pass_destination_type;

extern std::map<pass_destination_type, Arc::PasswordSource*> g_passsources;

// Functions in arcproxy_proxy.cpp

// Create simple temporary proxy
void create_tmp_proxy(std::string& proxy, Arc::Credential& signer);

// Create proxy with all bells and whistles as specified in arguments
void create_proxy(std::string& proxy,
    Arc::Credential& signer,
    const std::string& proxy_policy,
    const Arc::Time& proxy_start, const Arc::Period& proxy_period,
    const std::string& vomsacseq,
    bool use_gsi_proxy,
    int keybits,
    const std::string& signing_algorithm);

// Store content of proxy
void write_proxy_file(const std::string& path, const std::string& content);

// Delete proxy file
void remove_proxy_file(const std::string& path);

// Delete certificate file
void remove_cert_file(const std::string& path);



// Functions in arcproxy_voms.cpp

// Create simple temporary proxy
// Collect VOMS AC from configured Voms servers
bool contact_voms_servers(std::map<std::string,std::list<std::string> >& vomscmdlist,
    std::list<std::string>& orderlist, std::string& vomses_path,
    bool use_gsi_comm, bool use_http_comm, const std::string& voms_period,
    Arc::UserConfig& usercfg, Arc::Logger& logger, const std::string& tmp_proxy_path, std::string& vomsacseq);



// Functions in arcproxy_myproxy.cpp

// Communicate with MyProxy server
bool contact_myproxy_server(const std::string& myproxy_server, const std::string& myproxy_command,
    const std::string& myproxy_user_name, bool use_empty_passphrase, const std::string& myproxy_period,
    const std::string& retrievable_by_cert, Arc::Time& proxy_start, Arc::Period& proxy_period,
    std::list<std::string>& vomslist, std::string& vomses_path, const std::string& proxy_path,
    Arc::UserConfig& usercfg, Arc::Logger& logger);

#endif
