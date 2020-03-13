#pragma once

#include "job_details.h"
#include "job_result.h"
#include <configfiles/JobConfig.h>
#include <scheduler/Job.h>
#include <scheduler/Worker.h>
#include <scheduler/User.h>

#include <cinttypes>
#include <QDateTime>
#include <string>
#include <map>
#include <timedevents/Timer.h>
#include <chrono>

using namespace balancedbanana::configfiles;

namespace balancedbanana::database {

    using namespace balancedbanana::scheduler;

        //This is the interface that the rest of the program uses to query the database.
        class Repository : protected Observer<JobObservableEvent>, protected Observer<WorkerObservableEvent>, protected Observer<UserObservableEvent> {
        private:

            //structure: <id, <ptr, dirty>>
            std::map<uint64_t, std::pair<std::shared_ptr<Job>, bool>> jobCache;
            std::map<uint64_t, std::pair<std::shared_ptr<Worker>, bool>> workerCache;
            std::map<uint64_t, std::pair<std::shared_ptr<User>, bool>> userCache;
            //structure: <id, valid>
            std::pair<uint64_t, bool> lastJobId;
            std::pair<uint64_t, bool> lastWorkerId;
            std::pair<uint64_t, bool> lastUserId;
            timedevents::Timer timer;
            std::recursive_mutex mtx;

        public:
            Repository(const std::string& host_name, const std::string& databasename, const std::string& username,
                    const std::string& password,  uint64_t port, std::chrono::seconds updateInterval = std::chrono::minutes(1));

            ~Repository();

            std::shared_ptr<Worker> GetWorker(uint64_t id);
            std::shared_ptr<Worker> AddWorker(const std::string &name, const std::string &publickey, const Specs &specs, const std::string &address);

            std::shared_ptr<Job> GetJob(uint64_t id);
            std::shared_ptr<Job> AddJob(uint64_t user_id, const JobConfig& config, const QDateTime& schedule_time, const std::string& command);

            std::shared_ptr<User> GetUser(uint64_t id);
            std::shared_ptr<User> AddUser(const std::string& name, const std::string& email, const std::string& public_key);

            void WriteBack();
            void FlushCache();

            std::vector<std::shared_ptr<Worker>> GetActiveWorkers();
            std::vector<std::shared_ptr<Job>> GetUnfinishedJobs();
            std::vector<std::shared_ptr<Worker>> GetWorkers();
            std::shared_ptr<Worker> FindWorker(const std::string &name);
            std::shared_ptr<User> FindUser(const std::string &name);

        protected:
            void OnUpdate(Observable<WorkerObservableEvent> *observable, WorkerObservableEvent e) override;
            void OnUpdate(Observable<UserObservableEvent> *observable, UserObservableEvent e) override;
            void OnUpdate(Observable<JobObservableEvent> *observable, JobObservableEvent e) override;
        };
    }