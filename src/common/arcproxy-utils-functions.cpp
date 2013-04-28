#include "arcproxy-utils-functions.h"

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

#include "arc-gui-config.h"

#include <arc/ArcLocation.h>
#include <arc/DateTime.h>
#include <arc/delegation/DelegationInterface.h>
#include <arc/OptionParser.h>
#include <arc/StringConv.h>
#include <arc/User.h>
#include <arc/Utils.h>
#include <arc/UserConfig.h>
#if ARC_VERSION_MAJOR >= 3
#include <arc/communication/ClientInterface.h>
#else
#include <arc/client/ClientInterface.h>
#endif
#include <arc/credential/VOMSAttribute.h>
#include <arc/credential/VOMSUtil.h>
#include <arc/credential/Credential.h>
#include <arc/credential/CertUtil.h>
#include <arc/credentialstore/CredentialStore.h>
#include <arc/crypto/OpenSSL.h>
#include <arc/FileUtils.h>

#include <openssl/ui.h>

#define HAVE_NSS

#ifdef HAVE_NSS
#include <arc/credential/NSSUtil.h>
#endif

#define VOMS_LINE_NICKNAME (0)
#define VOMS_LINE_HOST (1)
#define VOMS_LINE_PORT (2)
#define VOMS_LINE_SN (3)
#define VOMS_LINE_NAME (4)
#define VOMS_LINE_NUM (5)

int create_proxy_file(const std::string& path) {
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

void write_proxy_file(const std::string& path, const std::string& content) {
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

void remove_proxy_file(const std::string& path) {
    if((::unlink(path.c_str()) != 0) && (errno != ENOENT)) {
        throw std::runtime_error("Failed to remove proxy file " + path);
    }
}

void tls_process_error(Arc::Logger& logger) {
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

#define PASS_MIN_LENGTH (4)
int input_password(char *password, int passwdsz, bool verify,
                   const std::string& prompt_info,
                   const std::string& prompt_verify_info,
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
        prompt1 = UI_construct_prompt(ui, "passphrase", prompt_info.c_str());
        ui_flags |= UI_INPUT_FLAG_DEFAULT_PWD;
        UI_ctrl(ui, UI_CTRL_PRINT_ERRORS, 1, 0, 0);
        ok = UI_add_input_string(ui, prompt1, ui_flags, password,
                                 0, passwdsz - 1);
        if (ok >= 0) {
            do {
                ok = UI_process(ui);
            } while (ok < 0 && UI_ctrl(ui, UI_CTRL_IS_REDOABLE, 0, 0, 0));
        }

        if (ok >= 0) res = strlen(password);

        if (ok >= 0 && verify) {
            UI_free(ui);
            ui = UI_new();
            if(!ui) {
                ok = -1;
            } else {
                // TODO: use some generic password strength evaluation
                if(res < PASS_MIN_LENGTH) {
                    UI_add_info_string(ui, "WARNING: Your password is too weak (too short)!\n"
                                       "Make sure this is really what You wanted to enter.\n");
                }
                prompt2 = UI_construct_prompt(ui, "passphrase", prompt_verify_info.c_str());
                ok = UI_add_verify_string(ui, prompt2, ui_flags, buf,
                                          0, passwdsz - 1, password);
                if (ok >= 0) {
                    do {
                        ok = UI_process(ui);
                    } while (ok < 0 && UI_ctrl(ui, UI_CTRL_IS_REDOABLE, 0, 0, 0));
                }
            }
        }

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
        if(ui) UI_free(ui);
        delete[] buf;
        if(prompt1) OPENSSL_free(prompt1);
        if(prompt2) OPENSSL_free(prompt2);
    }
    return res;
}

bool is_file(std::string path) {
    if (Glib::file_test(path, Glib::FILE_TEST_IS_REGULAR))
        return true;
    return false;
}

bool is_dir(std::string path) {
    if (Glib::file_test(path, Glib::FILE_TEST_IS_DIR))
        return true;
    return false;
}

std::vector<std::string> search_vomses(std::string path) {
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

std::string tokens_to_string(std::vector<std::string> tokens) {
    std::string s;
    for(int n = 0; n<tokens.size(); ++n) {
        s += "\""+tokens[n]+"\" ";
    };
    return s;
}

#ifdef HAVE_NSS
void get_default_nssdb_path(std::vector<std::string>& nss_paths) {
    const Arc::User user;
    // The profiles.ini could exist under firefox, seamonkey and thunderbird
    std::vector<std::string> profiles_homes;

#ifndef WIN32
    std::string home_path = user.Home();
#else
    std::string home_path = Glib::get_home_dir();
#endif

    std::string profiles_home;

#if defined(__APPLE__)
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
    // the nss configuration information (nss db location).
    // All of the information about nss db location  will be
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

void remove_cert_file(const std::string& path) {
  if((::unlink(path.c_str()) != 0) && (errno != ENOENT)) {
    throw std::runtime_error("Failed to remove certificate file " + path);
  }
}

bool find_matched_vomses(std::map<std::string, std::vector<std::vector<std::string> > > &matched_voms_line /*output*/,
    std::multimap<std::string, std::string>& server_command_map /*output*/,
    std::list<std::string>& vomses /*output*/,
    std::list<std::string>& vomslist, std::string& vomses_path, Arc::UserConfig& usercfg, Arc::Logger& logger) {
  //Parse the voms server and command from command line
  for (std::list<std::string>::iterator it = vomslist.begin();
      it != vomslist.end(); it++) {
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
  if (vomses_path.empty())
    vomses_path = usercfg.VOMSESPath();
  if (vomses_path.empty()) {
    logger.msg(Arc::ERROR, "$X509_VOMS_FILE, and $X509_VOMSES are not set;\nUser has not specify the location for vomses information;\nThere is also not vomses location information in user's configuration file;\nCannot find vomses in default locations: ~/.arc/vomses, ~/.voms/vomses, $ARC_LOCATION/etc/vomses, $ARC_LOCATION/etc/grid-security/vomses, $PWD/vomses, /etc/vomses, /etc/grid-security/vomses, and the location at the corresponding sub-directory");
    return false;
  }

  //the 'vomses' location could be one single files;
  //or it could be a directory which includes multiple files, such as 'vomses/voA', 'vomses/voB', etc.
  //or it could be a directory which includes multiple directories that includes multiple files,
  //such as 'vomses/atlas/voA', 'vomses/atlas/voB', 'vomses/alice/voa', 'vomses/alice/vob',
  //'vomses/extra/myprivatevo', 'vomses/mypublicvo'
  std::vector<std::string> vomses_files;
  //If the location is a file
  if(is_file(vomses_path)) vomses_files.push_back(vomses_path);
  //If the locaton is a directory, all the files and directories will be scanned
  //to find the vomses information. The scanning will not stop until all of the
  //files and directories are all scanned.
  else {
    std::vector<std::string> files;
    files = search_vomses(vomses_path);
    if(!files.empty())vomses_files.insert(vomses_files.end(), files.begin(), files.end());
    files.clear();
  }


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
            vomses.push_back(voms_line);
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
              vomses.push_back(voms_line);
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
  return true;
}

bool contact_voms_servers(std::list<std::string>& vomslist, std::list<std::string>& orderlist,
      std::string& vomses_path, bool use_gsi_comm, bool use_http_comm, const std::string& voms_period,
      Arc::UserConfig& usercfg, Arc::Logger& logger, const std::string& tmp_proxy_path, std::string& vomsacseq) {

  std::string ca_dir;
  ca_dir = usercfg.CACertificatesDirectory();

  std::map<std::string, std::vector<std::vector<std::string> > > matched_voms_line;
  std::multimap<std::string, std::string> server_command_map;
  std::list<std::string> vomses;
  if(!find_matched_vomses(matched_voms_line, server_command_map, vomses, vomslist, vomses_path, usercfg, logger))
    return false;

  //Contact the voms server to retrieve attribute certificate
  Arc::MCCConfig cfg;
  cfg.AddProxy(tmp_proxy_path);
  cfg.AddCADir(ca_dir);

  for (std::map<std::string, std::vector<std::vector<std::string> > >::iterator it = matched_voms_line.begin();
       it != matched_voms_line.end(); it++) {
    std::string voms_server;
    std::list<std::string> command_list;
    voms_server = (*it).first;
    std::vector<std::vector<std::string> > voms_lines = (*it).second;

    bool succeeded = false;
    //a boolean value to indicate if there is valid message returned from voms server, by using the current voms_line
    for (std::vector<std::vector<std::string> >::iterator line_it = voms_lines.begin();
        line_it != voms_lines.end(); line_it++) {
      std::vector<std::string> voms_line = *line_it;
      int count = server_command_map.count(voms_server);
      logger.msg(Arc::DEBUG, "There are %d commands to the same VOMS server %s", count, voms_server);

      std::multimap<std::string, std::string>::iterator command_it;
      for(command_it = server_command_map.equal_range(voms_server).first;
          command_it!=server_command_map.equal_range(voms_server).second; ++command_it) {
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
      for(std::list<std::string>::iterator o_it = orderlist.begin(); o_it != orderlist.end(); o_it++) {
        ordering.append(o_it == orderlist.begin() ? "" : ",").append(*o_it);
      }
      logger.msg(Arc::VERBOSE, "Try to get attribute from VOMS server with order: %s", ordering);
      send_msg.append("<order>").append(ordering).append("</order>");
      send_msg.append("<lifetime>").append(voms_period).append("</lifetime></voms>");
      logger.msg(Arc::VERBOSE, "Message sent to VOMS server %s is: %s", voms_name, send_msg);

      std::string ret_str;
      if(use_http_comm) {
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
          std::cout << Arc::IString("The VOMS server with the information:\n\t%s\ncan not be reached, please make sure it is available", tokens_to_string(voms_line)) << std::endl;
          continue; //There could be another voms replicated server with the same name exists
        }
        if (!response) {
          logger.msg(Arc::ERROR, "No HTTP response from VOMS server");
          continue;
        }
        if(response->Content() != NULL) ret_str.append(response->Content());
        if (response) delete response;
        logger.msg(Arc::VERBOSE, "Returned message from VOMS server: %s", ret_str);
      }
      else {
        // Use GSI or TLS to contact voms server
        Arc::ClientTCP client(cfg, address, atoi(port.c_str()), use_gsi_comm ? Arc::GSISec : Arc::SSL3Sec, usercfg.Timeout());
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
          else {
            std::cout << Arc::IString("Cannot get any AC or attributes info from VOMS server: %s;\n       Returned message from VOMS server: %s\n", voms_server, str);
            break; //since the voms servers with the same name should be looked as the same for robust reason, the other voms server that can be reached could returned the same message. So we exists the loop, even if there are other backup voms server exist.
          }
        }
        else {
          std::cout << Arc::IString("Returned message from VOMS server %s is: %s\n", voms_server, ret_str);
          break;
        }
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
        std::cout << Arc::IString("The attribute information from VOMS server: %s is list as following:", voms_server) << std::endl << decodedac << std::endl;
        return true;
      }

      vomsacseq.append(VOMS_AC_HEADER).append("\n");
      vomsacseq.append(codedac).append("\n");
      vomsacseq.append(VOMS_AC_TRAILER).append("\n");

      succeeded = true; break;
    }//end of the scanning of multiple vomses lines with the same name
    if(succeeded == false) {
      if(voms_lines.size() > 1)
        std::cout << Arc::IString("There are %d servers with the same name: %s in your vomses file, but all of them can not be reached, or can not return valid message. But proxy without VOMS AC extension will still be generated.", voms_lines.size(), voms_server) << std::endl;
    }
  }

  return true;
}

void get_nss_certname(std::string& certname, Arc::Logger& logger) {
    std::list<AuthN::certInfo> certInfolist;
    AuthN::nssListUserCertificatesInfo(certInfolist);
    if(certInfolist.size()) {
        std::cout<<Arc::IString("There are %d user certificates existing in the NSS database",
                                certInfolist.size())<<std::endl;
    }
    int n = 1;
    std::list<AuthN::certInfo>::iterator it;
    for(it = certInfolist.begin(); it != certInfolist.end(); it++) {
        AuthN::certInfo cert_info = (*it);
        std::string sub_dn = cert_info.subject_dn;
        std::string cn_name;
        std::string::size_type pos1, pos2;
        pos1 = sub_dn.find("CN=");
        if(pos1 != std::string::npos) {
            pos2 = sub_dn.find(",", pos1);
            if(pos2 != std::string::npos)
                cn_name = " ("+sub_dn.substr(pos1+3, pos2-pos1-3) + ")";
        }
        std::cout<<Arc::IString("Number %d is with nickname: %s%s", n, cert_info.certname, cn_name)<<std::endl;
        Arc::Time now;
        std::string msg;
        if(now > cert_info.end) msg = "(expired)";
        else if((now + 300) > cert_info.end) msg = "(will be expired in 5 min)";
        else if((now + 3600*24) > cert_info.end) {
            Arc::Period left(cert_info.end - now);
            msg = std::string("(will be expired in ") + std::string(left) + ")";
        }
        std::cout<<Arc::IString("    expiration time: %s ", cert_info.end.str())<<msg<<std::endl;
        //std::cout<<Arc::IString("    certificate dn:  %s", cert_info.subject_dn)<<std::endl;
        //std::cout<<Arc::IString("    issuer dn:       %s", cert_info.issuer_dn)<<std::endl;
        //std::cout<<Arc::IString("    serial number:   %d", cert_info.serial)<<std::endl;
        logger.msg(Arc::INFO, "    certificate dn:  %s", cert_info.subject_dn);
        logger.msg(Arc::INFO, "    issuer dn:       %s", cert_info.issuer_dn);
        logger.msg(Arc::INFO, "    serial number:   %d", cert_info.serial);
        n++;
    }

    std::cout << Arc::IString("Please choose the one you would use (1-%d): ", certInfolist.size());
    if(certInfolist.size() == 1) { it = certInfolist.begin(); certname = (*it).certname; }
    char c;
    while(true && (certInfolist.size()>1)) {
        c = getchar();
        int num = c - '0';
        if((num<=certInfolist.size()) && (num>=1)) {
            it = certInfolist.begin();
            std::advance(it, num-1);
            certname = (*it).certname;
            break;
        }
    }
}
#endif
