#include "arcsubmitcontroller.h"

#include <arc/UserConfig.h>
#include <arc/client/ComputingServiceRetriever.h>
#include <arc/UserConfig.h>
#include <arc/client/Broker.h>
#include <arc/client/Job.h>
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

int ArcSubmitController::submit()
{
    int retval;

    Arc::UserConfig usercfg("", "", Arc::initializeCredentialsType(Arc::initializeCredentialsType::TryCredentials));
    if (!usercfg) {
        logger.msg(Arc::ERROR, "Failed configuration initialization");
        return 1;
    }

    m_jobDescriptions.clear();

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
    return retval;
}
