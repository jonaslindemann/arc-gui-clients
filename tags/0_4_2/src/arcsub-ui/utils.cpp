#include "utils.h"

#include "arc-gui-config.h"

#include <arc/UserConfig.h>
#if ARC_VERSION_MAJOR >= 3
#include <arc/compute/ComputingServiceRetriever.h>
#include <arc/compute/Broker.h>
#include <arc/compute/Job.h>
#else
#include <arc/client/ComputingServiceRetriever.h>
#include <arc/UserConfig.h>
#include <arc/client/Broker.h>
#include <arc/client/Job.h>
#endif

#if ARC_VERSION_MAJOR >= 3
std::list<Arc::Endpoint> getServicesFromUserConfigAndCommandLine(Arc::UserConfig usercfg, std::list<std::string> registries, std::list<std::string> computingelements, std::string requestedSubmissionInterfaceName, std::string infointerface) {
    std::list<Arc::Endpoint> services;
    if (computingelements.empty() && registries.empty()) {
        std::list<Arc::ConfigEndpoint> endpoints = usercfg.GetDefaultServices();
        for (std::list<Arc::ConfigEndpoint>::const_iterator its = endpoints.begin(); its != endpoints.end(); its++) {
            services.push_back(*its);
        }
    } else {
        for (std::list<std::string>::const_iterator it = computingelements.begin(); it != computingelements.end(); it++) {
            // check if the string is a group or alias
            std::list<Arc::ConfigEndpoint> newServices = usercfg.GetServices(*it, Arc::ConfigEndpoint::COMPUTINGINFO);
            if (newServices.empty()) {
                // if it was not an alias or a group, then it should be the URL
                Arc::Endpoint service(*it);
                service.Capability.insert(Arc::Endpoint::GetStringForCapability(Arc::Endpoint::COMPUTINGINFO));
                if (!infointerface.empty()) {
                    service.InterfaceName = infointerface;
                }
                service.RequestedSubmissionInterfaceName = requestedSubmissionInterfaceName;
                services.push_back(service);
            } else {
                // if it was a group (or an alias), add all the services
                for (std::list<Arc::ConfigEndpoint>::iterator its = newServices.begin(); its != newServices.end(); its++) {
                    if (!requestedSubmissionInterfaceName.empty()) {
                        // if there was a submission interface requested, this overrides the one from the config
                        its->RequestedSubmissionInterfaceName = requestedSubmissionInterfaceName;
                    }
                    services.push_back(*its);
                }
            }
        }
        for (std::list<std::string>::const_iterator it = registries.begin(); it != registries.end(); it++) {
            // check if the string is a name of a group
            std::list<Arc::ConfigEndpoint> newServices = usercfg.GetServices(*it, Arc::ConfigEndpoint::REGISTRY);
            if (newServices.empty()) {
                // if it was not an alias or a group, then it should be the URL
                Arc::Endpoint service(*it);
                service.Capability.insert(Arc::Endpoint::GetStringForCapability(Arc::Endpoint::REGISTRY));
                services.push_back(service);
            } else {
                // if it was a group (or an alias), add all the services
                services.insert(services.end(), newServices.begin(), newServices.end());
            }
        }
    }
    return services;
}
#else
std::list<Arc::Endpoint> getServicesFromUserConfigAndCommandLine(Arc::UserConfig usercfg, std::list<std::string> registries, std::list<std::string> computingelements, std::string requestedSubmissionInterfaceName) {
    std::list<Arc::Endpoint> services;
    if (computingelements.empty() && registries.empty()) {
        std::list<Arc::ConfigEndpoint> endpoints = usercfg.GetDefaultServices();
        for (std::list<Arc::ConfigEndpoint>::const_iterator its = endpoints.begin(); its != endpoints.end(); its++) {
            services.push_back(Arc::Endpoint(*its));
        }
    } else {
        for (std::list<std::string>::const_iterator it = computingelements.begin(); it != computingelements.end(); it++) {
            // check if the string is a group or alias
            std::list<Arc::ConfigEndpoint> newServices = usercfg.GetServices(*it, Arc::ConfigEndpoint::COMPUTINGINFO);
            if (newServices.empty()) {
                // if it was not an alias or a group, then it should be the URL
                Arc::Endpoint service(*it);
                service.Capability.push_back(Arc::Endpoint::GetStringForCapability(Arc::Endpoint::COMPUTINGINFO));
                service.RequestedSubmissionInterfaceName = requestedSubmissionInterfaceName;
                services.push_back(service);
            } else {
                // if it was a group (or an alias), add all the services
                services.insert(services.end(), newServices.begin(), newServices.end());
            }
        }
        for (std::list<std::string>::const_iterator it = registries.begin(); it != registries.end(); it++) {
            // check if the string is a name of a group
            std::list<Arc::ConfigEndpoint> newServices = usercfg.GetServices(*it, Arc::ConfigEndpoint::REGISTRY);
            if (newServices.empty()) {
                // if it was not an alias or a group, then it should be the URL
                Arc::Endpoint service(*it);
                service.Capability.push_back(Arc::Endpoint::GetStringForCapability(Arc::Endpoint::REGISTRY));
                services.push_back(service);
            } else {
                // if it was a group (or an alias), add all the services
                services.insert(services.end(), newServices.begin(), newServices.end());
            }
        }
    }
    return services;
}
#endif

std::list<std::string> getSelectedURLsFromUserConfigAndCommandLine(Arc::UserConfig usercfg, std::list<std::string> computingelements) {
    std::list<Arc::Endpoint> endpoints = getServicesFromUserConfigAndCommandLine(usercfg, std::list<std::string>(), computingelements);
    std::list<std::string> serviceURLs;
    for (std::list<Arc::Endpoint>::const_iterator it = endpoints.begin(); it != endpoints.end(); it++) {
        serviceURLs.push_back(it->URLString);
    }
    return serviceURLs;
}

std::list<std::string> getRejectDiscoveryURLsFromUserConfigAndCommandLine(Arc::UserConfig usercfg, std::list<std::string> rejectdiscovery) {
    std::list<std::string> rejectDiscoveryURLs = usercfg.RejectDiscoveryURLs();
    rejectDiscoveryURLs.insert(rejectDiscoveryURLs.end(), rejectdiscovery.begin(), rejectdiscovery.end());
    return rejectDiscoveryURLs;
}

std::list<std::string> getRejectManagementURLsFromUserConfigAndCommandLine(Arc::UserConfig usercfg, std::list<std::string> rejectmanagement) {
    std::list<std::string> rejectManagementURLs = usercfg.RejectManagementURLs();
    rejectManagementURLs.insert(rejectManagementURLs.end(), rejectmanagement.begin(), rejectmanagement.end());
    return rejectManagementURLs;
}

static Arc::Logger logger(Arc::Logger::getRootLogger(), "ArcSub-UI");

void printjobid(const std::string& jobid, const std::string& jobidfile) {
    if (!jobidfile.empty())
        if (!Arc::Job::WriteJobIDToFile(jobid, jobidfile))
            logger.msg(Arc::WARNING, "Cannot write jobid (%s) to file (%s)", jobid, jobidfile);
    std::cout << Arc::IString("Job submitted with jobid: %s", jobid) << std::endl;
}


