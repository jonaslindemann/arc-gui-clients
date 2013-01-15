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
#ifdef ARC_VERSION_3
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

#undef HAVE_NSS

#ifdef HAVE_NSS
#include <arc/credential/NSSUtil.h>
#endif

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
std::string get_nssdb_path() {
    std::string nss_path;
    const Arc::User user;
#ifndef WIN32
    std::string home_path = user.Home();
#else
    std::string home_path = Glib::get_home_dir();
#endif
    std::string ff_home = home_path + G_DIR_SEPARATOR_S ".mozilla" G_DIR_SEPARATOR_S "firefox";

    struct stat st;
    if(::stat(ff_home.c_str(),&st) != 0) return std::string();
    if(!S_ISDIR(st.st_mode)) return std::string();
    if(user.get_uid() != st.st_uid) return std::string();

    std::string ff_profile = ff_home + G_DIR_SEPARATOR_S "profiles.ini";

    if(::stat(ff_profile.c_str(),&st) != 0) return std::string();
    if(!S_ISREG(st.st_mode)) return std::string();
    if(user.get_uid() != st.st_uid) return std::string();

    std::ifstream in_f(ff_profile.c_str());
    std::string profile_ini;
    bool is_relative = true;
    std::string path;
    std::getline<char>(in_f, profile_ini, '\0');

    std::list<std::string> lines;
    Arc::tokenize(profile_ini, lines, "\n");
    for (std::list<std::string>::iterator i = lines.begin(); i != lines.end(); ++i) {
        std::vector<std::string> inivalue;
        Arc::tokenize(*i, inivalue, "=");
        if (inivalue.size() == 2) {
            if (inivalue[0] == "IsRelative") {
                if(inivalue[1] == "1") is_relative = true;
                else is_relative = false;
            }
            if (inivalue[0] == "Path") path = inivalue[1];
        }
    }
    if(is_relative) nss_path = ff_home + G_DIR_SEPARATOR_S + path;
    else nss_path = path;

    if(::stat(nss_path.c_str(),&st) != 0) return std::string();
    if(!S_ISDIR(st.st_mode)) return std::string();
    if(user.get_uid() != st.st_uid) return std::string();

    return nss_path;
}
#endif
