#include "gtest/gtest.h"
#include "commandLineInterface/WorkerCommandLineProcessor.h"

using balancedbanana::commandLineInterface::WorkerCommandLineProcessor;
using balancedbanana::commandLineInterface::Task;
using balancedbanana::commandLineInterface::TaskType;

TEST(WorkerCommandLineProcessor, noArguments)
{
    WorkerCommandLineProcessor clp;

    char* argv[] = { "./bbs" };
    int argc = 1;

    std::shared_ptr<Task> task = std::make_shared<Task>();

    clp.process(argc, argv, task);

    ASSERT_EQ(task->getType(), (int)TaskType::WORKERSTART);
}


TEST(WorkerCommandLineProcessor, legalArguments)
{
    WorkerCommandLineProcessor clp;

    char* argv[] = { "./bbs", "-s", "192.168.0.1", "-S", "25567" };
    int argc = 5;

    std::shared_ptr<Task> task = std::make_shared<Task>();

    clp.process(argc, argv, task);

    ASSERT_EQ(task->getType(), (int)TaskType::WORKERSTART);
    ASSERT_STREQ(task->getServerIP().c_str(), argv[2]);
    ASSERT_EQ(task->getServerPort(), 25567);
}