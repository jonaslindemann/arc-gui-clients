#ifndef __utils_h__
#define __utils_h__

#include <arc/UserConfig.h>
#include <arc/client/Endpoint.h>
#include <arc/client/JobDescription.h>

#include <list>
#include <string>

std::list<std::string> getSelectedURLsFromUserConfigAndCommandLine(Arc::UserConfig usercfg, std::list<std::string> computingelements);
std::list<std::string> getRejectDiscoveryURLsFromUserConfigAndCommandLine(Arc::UserConfig usercfg, std::list<std::string> rejectdiscovery);
std::list<std::string> getRejectManagementURLsFromUserConfigAndCommandLine(Arc::UserConfig usercfg, std::list<std::string> rejectmanagement);
std::list<Arc::Endpoint> getServicesFromUserConfigAndCommandLine(Arc::UserConfig usercfg, std::list<std::string> registries, std::list<std::string> computingelements, std::string requestedSubmissionInterfaceName = "");
void printjobid(const std::string& jobid, const std::string& jobidfile);

#endif
