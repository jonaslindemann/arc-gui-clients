#ifndef __arcproxy_utils_functions_h__
#define __arcproxy_utils_functions_h__

#include <string>
#include <vector>

#include <arc/Logger.h>
#include <arc/UserConfig.h>

bool contact_voms_servers(std::list<std::string>& vomslist, std::list<std::string>& orderlist,
      std::string& vomses_path, bool use_gsi_comm, bool use_http_comm, const std::string& voms_period,
      Arc::UserConfig& usercfg, Arc::Logger& logger, const std::string& tmp_proxy_path, std::string& vomsacseq);

bool find_matched_vomses(std::map<std::string, std::vector<std::vector<std::string> > > &matched_voms_line /*output*/,
    std::multimap<std::string, std::string>& server_command_map /*output*/,
    std::list<std::string>& vomses /*output*/,
    std::list<std::string>& vomslist, std::string& vomses_path, Arc::UserConfig& usercfg, Arc::Logger& logger);


int create_proxy_file(const std::string& path);
void write_proxy_file(const std::string& path, const std::string& content);
void remove_proxy_file(const std::string& path);
void remove_cert_file(const std::string& path);
void tls_process_error(Arc::Logger& logger);
#define PASS_MIN_LENGTH (4)
int input_password(char *password, int passwdsz, bool verify,
                   const std::string& prompt_info,
                   const std::string& prompt_verify_info,
                   Arc::Logger& logger);

bool is_file(std::string path);
bool is_dir(std::string path);
std::vector<std::string> search_vomses(std::string path);
std::string tokens_to_string(std::vector<std::string> tokens);

#undef HAVE_NSS
#ifdef HAVE_NSS
void get_default_nssdb_path(std::vector<std::string>& nss_paths);
void get_nss_certname(std::string& certname, Arc::Logger& logger);
#endif

void tls_process_error(Arc::Logger& logger);

#endif
