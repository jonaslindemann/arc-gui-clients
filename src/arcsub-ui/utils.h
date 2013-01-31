#ifndef __utils_h__
#define __utils_h__

#include "arc-gui-config.h"

#include <arc/UserConfig.h>

#if ARC_VERSION_MAJOR >= 3
#include <arc/compute/Endpoint.h>
#include <arc/compute/JobDescription.h>
#else
#include <arc/client/Endpoint.h>
#include <arc/client/JobDescription.h>
#endif

#include <list>
#include <string>

std::list<std::string> getSelectedURLsFromUserConfigAndCommandLine(Arc::UserConfig usercfg, std::list<std::string> computingelements);
std::list<std::string> getRejectDiscoveryURLsFromUserConfigAndCommandLine(Arc::UserConfig usercfg, std::list<std::string> rejectdiscovery);
std::list<std::string> getRejectManagementURLsFromUserConfigAndCommandLine(Arc::UserConfig usercfg, std::list<std::string> rejectmanagement);
#if ARC_VERSION_MAJOR >= 3
std::list<Arc::Endpoint> getServicesFromUserConfigAndCommandLine(Arc::UserConfig usercfg, std::list<std::string> registries, std::list<std::string> computingelements, std::string requestedSubmissionInterfaceName = "", std::string infointerface = "");
#else
std::list<Arc::Endpoint> getServicesFromUserConfigAndCommandLine(Arc::UserConfig usercfg, std::list<std::string> registries, std::list<std::string> computingelements, std::string requestedSubmissionInterfaceName = "");
#endif

void printjobid(const std::string& jobid, const std::string& jobidfile);

#endif
