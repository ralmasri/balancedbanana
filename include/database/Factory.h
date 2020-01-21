#pragma once
#include "job_details.h"
#include "worker_details.h"
#include "user_details.h"
#include <scheduler/Job.h>
#include <scheduler/Worker.h>
#include <scheduler/User.h>

namespace balancedbanana {
    namespace database {

        //Factory method pattern. This class creates objects using given data.
        class Factory {
        public:
            //Creates a Job object.
            scheduler::Job createJob(const job_details);

            //Creates a Worker object.
            scheduler::Worker createWorker(const worker_details);

            //Creates a User object.
            scheduler::User createUser(const user_details);

        };
    }
}