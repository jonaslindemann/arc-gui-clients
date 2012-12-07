#ifndef __arcproxy_utils_functions_h__
#define __arcproxy_utils_functions_h__

#include <string>
#include <vector>

#include <arc/Logger.h>

int create_proxy_file(const std::string& path);
void write_proxy_file(const std::string& path, const std::string& content);
void remove_proxy_file(const std::string& path);
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

#ifdef HAVE_NSS
std::string get_nssdb_path();
#endif

void tls_process_error(Arc::Logger& logger);

#endif
