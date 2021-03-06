#include <database/Factory.h>
#include <database/job_details.h>
#include <database/user_details.h>
#include <database/job_result.h>
#include <scheduler/User.h>
#include <scheduler/Worker.h>
#include <scheduler/Job.h>
#include <database/JobStatus.h>
#include <scheduler/IUser.h>
#include <QDebug>
#include <gtest/gtest.h>

using namespace balancedbanana::database;
using namespace balancedbanana::scheduler;

class CreateUserTest : public ::testing::Test {
protected:
    void SetUp() override {
        user_info.public_key = "34nrhk3hkr";
        user_info.email = "someemail@kit.edu";
        user_info.name = "CentOS";
        user_info.id = 1;
    }

    user_details user_info;
};

bool compareUsers(User &expected, User &actual){
    return expected.pubkey() == actual.pubkey()
           && expected.id() == actual.id()
           && expected.name() == actual.name()
           && expected.email() == actual.email();
}


TEST_F(CreateUserTest, CreateUserTest_Success_Test) {
    User user_expected = User(user_info.id, user_info.name, user_info.public_key);
    user_expected.setEmail(user_info.email);
    std::shared_ptr<User> user_actual = Factory::createUser(user_info);
    ASSERT_TRUE(compareUsers(user_expected, *user_actual));
}

class CreateJobTest : public ::testing::Test {
protected:
    void SetUp() override {
        job_info.id = 1;
        job_info.user_id = 1;
        job_info.command = "mkdir build";
        job_info.schedule_time = QDateTime::currentDateTime();
        job_info.empty = false;
        job_info.config.set_image("testimage");
        job_info.config.set_current_working_dir(".");
        job_info.start_time = std::make_optional<QDateTime>(QDateTime::currentDateTime().addDays(2));
        job_info.status = (int) JobStatus::processing;
        job_info.worker_id = 1;

        user_info.public_key = "34nrhk3hkr";
        user_info.email = "someemail@kit.edu";
        user_info.name = "CentOS";
        user_info.id = 1;
    }

    job_details job_info;
    user_details user_info;
};

bool compareJobs(const Job& expected, const Job& actual){
    return compareUsers(*expected.getUser(), *actual.getUser())
    && expected.getAllocated_cores() == actual.getAllocated_cores()
    && expected.getAllocated_osIdentifier() == actual.getAllocated_osIdentifier()
    && expected.getAllocated_ram() == actual.getAllocated_ram()
    && expected.getId() == actual.getId()
    && expected.getStarted_at() == actual.getStarted_at()
    && expected.getScheduled_at() == actual.getScheduled_at()
    && expected.getFinished_at() == actual.getFinished_at()
    && expected.getCommand() == actual.getCommand()
    && expected.getWorker_id() == actual.getWorker_id()
    && expected.getConfig()->image() == actual.getConfig()->image()
    && expected.getConfig()->current_working_dir() == actual.getConfig()->current_working_dir()
    && static_cast<int>(expected.getStatus()) == static_cast<int>(actual.getStatus())
    && ((!expected.getResult() && !actual.getResult()) || (expected.getResult()->stdout == actual.getResult()->stdout
    && expected.getResult()->exit_code == actual.getResult()->exit_code));
}

void fillJobWithDetails(const job_details& job_info, std::shared_ptr<User> user, Job& job_expected){
    job_expected.setUser(user);
    job_expected.setStatus(static_cast<JobStatus>(job_info.status));
    job_expected.setCommand(job_info.command);
    job_expected.setScheduled_at(job_info.schedule_time);

    if (job_info.worker_id.has_value()){
        job_expected.setWorker_id(job_info.worker_id.value());
    }

    if (job_info.start_time.has_value()){
        job_expected.setStarted_at(job_info.start_time.value());
    }

    if (job_info.finish_time.has_value()){
        job_expected.setFinished_at(job_info.finish_time.value());
    }

    if (job_info.allocated_specs.has_value()){
        job_expected.setAllocated_ram(job_info.allocated_specs->ram);
        job_expected.setAllocated_osIdentifier(job_info.allocated_specs->osIdentifier);
        job_expected.setAllocated_cores(job_info.allocated_specs->cores);
    }

    if (job_info.result.has_value()){
        std::shared_ptr<job_result> resultPtr = std::make_shared<job_result>(job_info.result.value());
        job_expected.setResult(resultPtr);
    }
}

TEST_F(CreateJobTest, CreateJobTest_Success_Test){
    std::shared_ptr<User> user = std::make_shared<User>(user_info.id, user_info.name, user_info.public_key);
    user->setEmail(user_info.email);
    Job job_expected(job_info.id, std::make_shared<JobConfig>(job_info.config));
    fillJobWithDetails(job_info, user, job_expected);
    std::shared_ptr<Job> job_actual = Factory::createJob(job_info, user);
    ASSERT_TRUE(compareJobs(job_expected, *job_actual));
}

TEST_F(CreateJobTest, CreateJobTest_FullCreate_Test){
    job_info.finish_time = QDateTime::currentDateTime();
    job_info.allocated_specs = {" ", .ram = 5, .cores = 4};
    job_info.result = {.stdout = " ", .exit_code = 5};
    std::shared_ptr<User> user = std::make_shared<User>(user_info.id, user_info.name, user_info.public_key);
    user->setEmail(user_info.email);
    Job job_expected(job_info.id, std::make_shared<JobConfig>(job_info.config));
    fillJobWithDetails(job_info, user, job_expected);
    std::shared_ptr<Job> job_actual = Factory::createJob(job_info, user);
    ASSERT_TRUE(compareJobs(job_expected, *job_actual));
}

class CreateWorkerTest : public ::testing::Test {
protected:
    void SetUp() override {
        worker_info.empty = false;
        worker_info.public_key = "casdasdc";
        worker_info.specs = {.osIdentifier = " ", .ram = 5, .cores = 6};
        worker_info.id = 1;
        worker_info.name = "Rakan";
    }

    worker_details worker_info;
};

bool compareWorkers(Worker& expected, Worker& actual){
    return expected.getId() == actual.getId()
    && ((!expected.getSpec().has_value() && !actual.getSpec().has_value())
    || (expected.getSpec().value() == actual.getSpec().value()))
    && expected.name() == actual.name()
    && expected.pubkey() == actual.pubkey();
}

TEST_F(CreateWorkerTest, CreateWorkerTest_Success_Test){
    Worker worker_expected = Worker(worker_info.id, worker_info.name, worker_info.public_key, worker_info.specs);
    std::shared_ptr<Worker> worker_actual = Factory::createWorker(worker_info);
    ASSERT_TRUE(compareWorkers(worker_expected, *worker_actual));
}
