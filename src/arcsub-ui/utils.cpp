#include "utils.h"

#include <arc/client/ComputingServiceRetriever.h>
#include <arc/UserConfig.h>
#include <arc/client/Broker.h>
#include <arc/client/Job.h>

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

static Arc::Logger logger(Arc::Logger::getRootLogger(), "ArcStorage-UI");

void printjobid(const std::string& jobid, const std::string& jobidfile) {
    if (!jobidfile.empty())
        if (!Arc::Job::WriteJobIDToFile(jobid, jobidfile))
            logger.msg(Arc::WARNING, "Cannot write jobid (%s) to file (%s)", jobid, jobidfile);
    std::cout << Arc::IString("Job submitted with jobid: %s", jobid) << std::endl;
}

JobSubmitter::JobSubmitter()
{
    m_jobListFilename = "myjoblist.xml";
}

JobSubmitter::~JobSubmitter()
{

}

int JobSubmitter::submit(std::list<Arc::JobDescription>& jobDescriptionList)
{
    int retval;

    Arc::UserConfig usercfg("", "", Arc::initializeCredentialsType(Arc::initializeCredentialsType::TryCredentials));
    if (!usercfg) {
        logger.msg(Arc::ERROR, "Failed configuration initialization");
        return 1;
    }

    std::list<std::string> preferredInterfaceNames;

    if (usercfg.InfoInterface().empty()) {
        preferredInterfaceNames.push_back("org.nordugrid.ldapglue2");
        preferredInterfaceNames.push_back("org.ogf.emies");
    } else {
        preferredInterfaceNames.push_back(usercfg.InfoInterface());
    }

    std::list<std::string> registries;
    std::list<std::string> computingelements;
    std::list<std::string> rejectmanagement;
    std::list<Arc::Endpoint> services = getServicesFromUserConfigAndCommandLine(usercfg, registries, computingelements);
    std::list<std::string> rejectDiscoveryURLs = getRejectDiscoveryURLsFromUserConfigAndCommandLine(usercfg, rejectmanagement);

    Arc::ComputingServiceRetriever csr(usercfg, services, rejectDiscoveryURLs, preferredInterfaceNames);
    csr.wait();

    if (csr.empty()) {
        std::cout << Arc::IString("Job submission aborted because no resource returned any information") << std::endl;
        return 1;
    }

    Arc::Broker broker(usercfg, usercfg.Broker().first);
    if (!broker.isValid()) {
        logger.msg(Arc::ERROR, "Unable to load broker %s", usercfg.Broker().first);
        return 1;
    }
    logger.msg(Arc::INFO, "Broker %s loaded", usercfg.Broker().first);

    int jobnr = 1;
    std::list<std::string> jobids;
    std::list<Arc::Job> submittedJobs;
    std::map<int, std::string> notsubmitted;

    for (std::list<Arc::JobDescription>::const_iterator itJ =
         jobDescriptionList.begin(); itJ != jobDescriptionList.end();
         ++itJ, ++jobnr) {
        bool descriptionSubmitted = false;
        submittedJobs.push_back(Arc::Job());
        broker.set(*itJ);
        Arc::ExecutionTargetSet etSet(broker, csr);
        for (Arc::ExecutionTargetSet::iterator itET = etSet.begin(); itET != etSet.end(); ++itET) {
            if (itET->Submit(usercfg, *itJ, submittedJobs.back())) {
                printjobid(submittedJobs.back().JobID.fullstr(), "");
                descriptionSubmitted = true;
                itET->RegisterJobSubmission(*itJ);
                break;
            }
        }
        if (!descriptionSubmitted && itJ->HasAlternatives()) {
            // TODO: Deal with alternative job descriptions more effective.
            for (std::list<Arc::JobDescription>::const_iterator itJAlt = itJ->GetAlternatives().begin();
                 itJAlt != itJ->GetAlternatives().end(); ++itJAlt) {
                broker.set(*itJAlt);
                Arc::ExecutionTargetSet etSetAlt(broker, csr);
                for (Arc::ExecutionTargetSet::iterator itET = etSetAlt.begin(); itET != etSetAlt.end(); ++itET) {
                    if (itET->Submit(usercfg, *itJAlt, submittedJobs.back())) {
                        printjobid(submittedJobs.back().JobID.fullstr(), "");
                        descriptionSubmitted = true;
                        itET->RegisterJobSubmission(*itJAlt);
                        break;
                    }
                }
                if (descriptionSubmitted) {
                    break;
                }
            }
        }

        if (!descriptionSubmitted) {
            std::cout << Arc::IString("Job submission failed, no more possible targets") << std::endl;
            submittedJobs.pop_back();
            notsubmitted[jobnr] = itJ->Identification.JobName;
            retval = 1;
        }
    } //end loop over all job descriptions

    if (!Arc::Job::WriteJobsToFile(m_jobListFilename, submittedJobs)) {
        std::cout << Arc::IString("Warning: Failed to lock job list file %s", usercfg.JobListFile())
                  << std::endl;
        std::cout << Arc::IString("To recover missing jobs, run arcsync") << std::endl;
    }

    if (jobDescriptionList.size() > 1) {
        std::cout << std::endl << Arc::IString("Job submission summary:")
                  << std::endl;
        std::cout << "-----------------------" << std::endl;
        std::cout << Arc::IString("%d of %d jobs were submitted",
                                  submittedJobs.size(),
                                  jobDescriptionList.size()) << std::endl;
        if (!notsubmitted.empty()) {
            std::cout << Arc::IString("The following %d were not submitted",
                                      notsubmitted.size()) << std::endl;
            std::map<int, std::string>::const_iterator it = notsubmitted.begin();
            for (; it != notsubmitted.end(); ++it) {
                std::cout << Arc::IString("Job nr.") << " " << it->first;
                if (!it->second.empty()) {
                    std::cout << ": " << it->second;
                }
                std::cout << std::endl;
            }
        }
    }
    return retval;
}
