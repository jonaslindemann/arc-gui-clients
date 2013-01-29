#include "arcsubmitcontroller.h"


#include "arc-gui-config.h"

#include <arc/UserConfig.h>

#ifdef ARC_VERSION_3
#include <arc/compute/Broker.h>
#include <arc/compute/ComputingServiceRetriever.h>
#include <arc/compute/ExecutionTarget.h>
#include <arc/compute/Job.h>
#include <arc/compute/JobDescription.h>
#include <arc/compute/SubmissionStatus.h>
#include <arc/compute/Submitter.h>
#else
#include <arc/client/ComputingServiceRetriever.h>
#include <arc/client/Broker.h>
#include <arc/client/Job.h>
#endif
#include <arc/UserConfig.h>
#include <arc/Logger.h>

#include <QtConcurrentRun>

#include "utils.h"

ArcSubmitController::ArcSubmitController()
{
    m_jobListFilename = "myjoblist.xml";
    connect(&m_submissionWatcher, SIGNAL(finished()), this, SLOT(submissionFinished()));
}

ArcSubmitController::~ArcSubmitController()
{

}

void ArcSubmitController::setJobListFilename(QString filename)
{
    m_jobListFilename = filename;
}

void ArcSubmitController::addJobDescription(Arc::JobDescription jobDescription)
{
    m_jobDescriptions.push_back(jobDescription);
}

void ArcSubmitController::clear()
{
    m_jobDescriptions.clear();
}

void ArcSubmitController::startSubmission()
{
    m_submissionWatcher.setFuture(QtConcurrent::run(this, &ArcSubmitController::submit));
}

void ArcSubmitController::submissionFinished()
{
    Q_EMIT onSubmissionFinished();
}

static Arc::Logger logger(Arc::Logger::getRootLogger(), "ArcSub-UI");

#ifdef ARC_VERSION_3

class HandleSubmittedJobs : public Arc::EntityConsumer<Arc::Job> {
public:
  HandleSubmittedJobs(const std::string& jobidfile, const std::string& joblist) : jobidfile(jobidfile), joblist(joblist), submittedJobs() {}

  void addEntity(const Arc::Job& j) {
    std::cout << Arc::IString("Job submitted with jobid: %s", j.JobID) << std::endl;
    submittedJobs.push_back(j);
  }

  void write() const {
    if (!jobidfile.empty() && !Arc::Job::WriteJobIDsToFile(submittedJobs, jobidfile)) {
      logger.msg(Arc::WARNING, "Cannot write jobids to file (%s)", jobidfile);
    }
    if (!Arc::Job::WriteJobsToFile(joblist, submittedJobs)) {
      std::cout << Arc::IString("Warning: Failed to lock job list file %s", joblist)
                << std::endl;
      std::cout << Arc::IString("To recover missing jobs, run arcsync") << std::endl;
    }
  }

  void printsummary(const std::list<Arc::JobDescription>& originalDescriptions, const std::list<const Arc::JobDescription*>& notsubmitted) const {
    if (originalDescriptions.size() > 1) {
      std::cout << std::endl << Arc::IString("Job submission summary:") << std::endl;
      std::cout << "-----------------------" << std::endl;
      std::cout << Arc::IString("%d of %d jobs were submitted", submittedJobs.size(), submittedJobs.size()+notsubmitted.size()) << std::endl;
      if (!notsubmitted.empty()) {
        std::cout << Arc::IString("The following %d were not submitted", notsubmitted.size()) << std::endl;
        for (std::list<const Arc::JobDescription*>::const_iterator it = notsubmitted.begin();
             it != notsubmitted.end(); ++it) {
          int jobnr = 1;
          for (std::list<Arc::JobDescription>::const_iterator itOrig = originalDescriptions.begin();
               itOrig != originalDescriptions.end(); ++itOrig, ++jobnr) {
            if (&(*itOrig) == *it) {
              std::cout << Arc::IString("Job nr.") << " " << jobnr;
              if (!(*it)->Identification.JobName.empty()) {
                std::cout << ": " << (*it)->Identification.JobName;
              }
              std::cout << std::endl;
              break;
            }
          }
        }
      }
    }
  }

  void clearsubmittedjobs() { submittedJobs.clear(); }

private:
  const std::string jobidfile, joblist;
  std::list<Arc::Job> submittedJobs;
};

int ArcSubmitController::submit()
{
    int retval = 0;
    bool direct_submission = false;

    Arc::UserConfig usercfg("", "", Arc::initializeCredentialsType(Arc::initializeCredentialsType::TryCredentials));
    if (!usercfg) {
        logger.msg(Arc::ERROR, "Failed configuration initialization");
        return 1;
    }

    std::list<std::string> registries;
    std::list<std::string> computingelements;
    std::list<Arc::Endpoint> services = getServicesFromUserConfigAndCommandLine(usercfg, registries, computingelements);

    HandleSubmittedJobs hsj("", m_jobListFilename.toStdString());
    Arc::Submitter s(usercfg);
    s.addConsumer(hsj);

    Arc::SubmissionStatus status;
    if (!direct_submission) {
      status = s.BrokeredSubmit(services, m_jobDescriptions);
    }
    else {
      status = s.Submit(services, m_jobDescriptions);
    }
    hsj.write();

    if (status.isSet(Arc::SubmissionStatus::BROKER_PLUGIN_NOT_LOADED)) {
      std::cerr << Arc::IString("ERROR: Unable to load broker %s", usercfg.Broker().first) << std::endl;
      return 1;
    }
    if (status.isSet(Arc::SubmissionStatus::NO_SERVICES)) {
     std::cerr << Arc::IString("ERROR: Job submission aborted because no resource returned any information") << std::endl;
     return 1;
    }
    if (status.isSet(Arc::SubmissionStatus::SUBMITTER_PLUGIN_NOT_LOADED)) {
      bool gridFTPJobPluginFailed = false;
      for (std::map<Arc::Endpoint, Arc::EndpointSubmissionStatus>::const_iterator it = s.GetEndpointSubmissionStatuses().begin();
           it != s.GetEndpointSubmissionStatuses().end(); ++it) {
        if (it->first.InterfaceName == "org.nordugrid.gridftpjob" && it->second == Arc::EndpointSubmissionStatus::NOPLUGIN) {
          gridFTPJobPluginFailed = true;
        }
      }
      if (gridFTPJobPluginFailed) {
        std::cerr << Arc::IString("ERROR: A computing resource using the gridftp interface was requested, but") << std::endl;
        std::cerr << Arc::IString("       the corresponding plugin could not be loaded. Is the plugin installed?") << std::endl;
        std::cerr << Arc::IString("       If not, please install the package 'nordugrid-arc-plugins-globus'.") << std::endl;
        std::cerr << Arc::IString("       Depending on your type of installation the package name might differ. ") << std::endl;
      }
      // TODO: What to do when failing to load other plugins.
      retval = 1;
    }
    if (status.isSet(Arc::SubmissionStatus::DESCRIPTION_NOT_SUBMITTED)) {
      std::cerr << Arc::IString("ERROR: One or multiple job descriptions was not submitted.") << std::endl;
      retval = 1;
    }

    hsj.printsummary(m_jobDescriptions, s.GetDescriptionsNotSubmitted());

    return retval;
}
#else
int ArcSubmitController::submit()
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
         m_jobDescriptions.begin(); itJ != m_jobDescriptions.end();
         ++itJ, ++jobnr) {
        std::cout << "Submitting " << jobnr << " of " << m_jobDescriptions.size() << std::endl;
        Q_EMIT onSubmissionStatus(jobnr, m_jobDescriptions.size(), "Submitting job.");
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

    if (!Arc::Job::WriteJobsToFile(m_jobListFilename.toStdString(), submittedJobs)) {
        std::cout << Arc::IString("Warning: Failed to lock job list file %s", usercfg.JobListFile())
                  << std::endl;
        std::cout << Arc::IString("To recover missing jobs, run arcsync") << std::endl;
    }

    if (m_jobDescriptions.size() > 1) {
        std::cout << std::endl << Arc::IString("Job submission summary:")
                  << std::endl;
        std::cout << "-----------------------" << std::endl;
        std::cout << Arc::IString("%d of %d jobs were submitted",
                                  submittedJobs.size(),
                                  m_jobDescriptions.size()) << std::endl;
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

    m_jobDescriptions.clear();

    return retval;
}
#endif
