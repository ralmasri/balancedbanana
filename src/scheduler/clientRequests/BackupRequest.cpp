#include "scheduler/clientRequests/BackupRequest.h"

#include "scheduler/Job.h"
#include "scheduler/Worker.h"
#include <sstream>
#include <communication/message/TaskMessage.h>
#include <database/Repository.h>

using balancedbanana::communication::TaskMessage;
using balancedbanana::database::JobStatus;
using balancedbanana::scheduler::Job;
using balancedbanana::database::Repository;

namespace balancedbanana
{
namespace scheduler
{

constexpr bool STOP_ON_BACKUP = false;

std::shared_ptr<std::string> BackupRequest::executeRequestAndFetchData(const std::shared_ptr<Task> &task,
                                                                       const std::function<std::shared_ptr<balancedbanana::scheduler::Job>(uint64_t)> &dbGetJob,
                                                                       const std::function<void(uint64_t, balancedbanana::database::JobStatus)> &dbUpdateJobStatus,
                                                                       const std::function<uint64_t(uint64_t, const std::shared_ptr<JobConfig>&, const std::string& command)> &dbAddJob,
                                                                       const std::function<std::shared_ptr<Worker>(uint64_t id)> &dbGetWorker,
                                                                       uint64_t userID)
{
    // Step 1: Go to DB and get job status
    std::stringstream response;

    if (task->getJobId().has_value() == false)
    {
        // Note that job id is required for the backup command
        // exit with the reponse set to the error message of not having a jobid
        response << NO_JOB_ID << std::endl;
        return std::make_shared<std::string>(response.str());
    }
    std::shared_ptr<Job> job = dbGetJob(task->getJobId().value());

    if (job == nullptr)
    {
        // Job not found
        response << NO_JOB_WITH_ID << std::endl;
        return std::make_shared<std::string>(response.str());
    }

    std::shared_ptr<Worker> worker = dbGetWorker(job->getWorker_id());
    switch ((job->getStatus()))
    {
    case JobStatus::scheduled:
        // nothing to backup
        response << OPERATION_UNAVAILABLE_JOB_NOT_RUN << std::endl;
        break;
    case JobStatus::processing:
        // backup and respond success / failure
        {
            // send backup request to worker
            // Set userId for Worker
            task->setUserId(userID);
            // Just Send to Worker
            worker->send(TaskMessage(*task));
        }
        response << OPERATION_PROGRESSING_BACKUP << std::endl;
        break;
    case JobStatus::paused:
        // backup and respond success / failure
        {
            // send backup request to worker
            // Set userId for Worker
            task->setUserId(userID);
            // Just Send to Worker
            worker->send(TaskMessage(*task));
        }
        response << OPERATION_PROGRESSING_BACKUP << std::endl;
        break;
    case JobStatus::interrupted:
        // backup and respond success / failure
        {
            // send backup request to worker
            // Set userId for Worker
            task->setUserId(userID);
            // Just Send to Worker
            worker->send(TaskMessage(*task));
        }
        response << OPERATION_PROGRESSING_BACKUP << std::endl;
        break;
    case JobStatus::canceled:
        // cannot make backup. job is killed
        response << OPERATION_UNAVAILABLE_JOB_ABORTED << std::endl;
        break;
    case JobStatus::finished:
        // Job is done
        response << OPERATION_UNAVAILABLE_JOB_FINISHED << std::endl;
        break;
    default:
        // add info job has corrupted status to response
        response << JOB_STATUS_UNKNOWN << std::endl;
        break;
    }

    // Step 2: Create and send ResponseMessage with status as string
    return std::make_shared<std::string>(response.str());
}

} // namespace scheduler

} // namespace balancedbanana
