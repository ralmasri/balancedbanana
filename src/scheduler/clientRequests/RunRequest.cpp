#include "scheduler/clientRequests/RunRequest.h"

#include "communication/Communicator.h"
#include <sstream>
#include <communication/message/TaskMessage.h>
#include <scheduler/Clients.h>

using balancedbanana::communication::Communicator;
using balancedbanana::communication::TaskMessage;

namespace balancedbanana
{
namespace scheduler
{

RunRequest::RunRequest(const std::shared_ptr<Task> &task,
                       const uint64_t userID,
                       Communicator &client,
                       const std::function<std::shared_ptr<Job>(uint64_t jobID)> &dbGetJob,
                       const std::function<std::shared_ptr<Worker>(uint64_t workerID)> &dbGetWorker,
                       const std::function<std::shared_ptr<Job>(const uint64_t userID, const std::shared_ptr<JobConfig> &config, QDateTime &scheduleTime, const std::string &jobCommand)> &dbAddJob,
                       const std::function<uint64_t(uint64_t jobID)> &queueGetPosition)
    : ClientRequest(task, userID, client, dbGetJob, dbGetWorker, dbAddJob, queueGetPosition)
{
}

std::shared_ptr<RespondToClientMessage> RunRequest::executeRequestAndFetchData()
{
    // prepare response
    std::stringstream response;

    // enter job into database
    auto config = task->getConfig();
    QDateTime scheduleTime = QDateTime::currentDateTime();
    std::shared_ptr<Job> job = dbAddJob(userID, config, scheduleTime, task->getTaskCommand());
    Clients::enterByUser(job->getUser()->id() | (uint64_t)0x8000000000000000, *client);

    // fail if job could not be created, otherwise return success
    if (job == nullptr)
    {
        response << OPERATION_FAILURE << std::endl;
    }
    else
    {
        response << PREFIX_JOB_ID << job->getId() << std::endl;
    }

    // respond
    return std::make_shared<RespondToClientMessage>(response.str(), !task->getConfig()->blocking_mode().value_or(false), 0);
}

} // namespace scheduler

} // namespace balancedbanana
