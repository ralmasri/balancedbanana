#include <worker/Worker.h>
#include <worker/docker/Docker.h>
#include <worker/docker/Container.h>
#include <worker/docker/Checkpoint.h>
#include <commandLineInterface/WorkerCommandLineProcessor.h>
#include <communication/message/TaskMessage.h>
#include <communication/message/AuthResultMessage.h>
#include <communication/message/WorkerLoadResponseMessage.h>
#include <communication/message/TaskResponseMessage.h>
#include <communication/message/RespondToClientMessage.h>
#include <communication/message/HardwareDetailMessage.h>
#include <communication/authenticator/Authenticator.h>
#include <sys/sysinfo.h>

using namespace balancedbanana::worker;
using namespace balancedbanana::communication;
using balancedbanana::commandLineInterface::WorkerCommandLineProcessor;
using balancedbanana::communication::Communicator;
using balancedbanana::communication::Task;
using balancedbanana::communication::TaskMessage;
using balancedbanana::communication::RespondToClientMessage;
using balancedbanana::communication::TaskType;
using balancedbanana::configfiles::ApplicationConfig;

#ifdef _WIN32
#define HOME_ENV "USERPROFILE"
#else
#define HOME_ENV "HOME"
#endif

Worker::Worker()
{
    task = std::make_shared<Task>();
    auto configdir = std::filesystem::canonical(getenv(HOME_ENV)) / ".bbd";
    std::filesystem::create_directories(configdir);
    configpath = configdir / "appconfig.ini";
    config = ApplicationConfig(configpath);
    publicauthfailed = false;
}

void Worker::connectWithServer(const std::string &serverIpAdress, short serverPort)
{
    communicator = std::make_shared<Communicator>(serverIpAdress, serverPort, std::shared_ptr<balancedbanana::communication::MessageProcessor>(shared_from_this(), this));
}

void Worker::authenticateWithServer()
{
    balancedbanana::communication::authenticator::Authenticator auth(communicator);
    auto keypath = configpath.parent_path() / "privatekey.pem";
    if (!publicauthfailed && std::filesystem::exists(keypath) && config.Contains("name"))
    {
        std::ifstream file(keypath);
        std::stringstream content;
        content << file.rdbuf();
        auth.publickeyauthenticate(config["name"], content.str());
    }
    else
    {
        publicauthfailed = true;
        auto result = auth.authenticate();
        config["name"] = result.first;
        config.Save(configpath);
        std::ofstream file(keypath);
        file << result.second;
    }
}

std::future<int> Worker::processCommandLineArguments(int argc, const char *const *argv)
{
    WorkerCommandLineProcessor clp;
    auto code = clp.process(argc, argv, task);
    if ((uint32_t)task->getType())
    {
        std::string server = "localhost";
        short port = 8444;
        if (!task->getServerIP().empty())
        {
            server = task->getServerIP();
        }
        else if (config.Contains("server"))
        {
            server = config["server"];
        }
        if (task->getServerPort())
        {
            port = task->getServerPort();
        }
        else if (config.Contains("port"))
        {
            port = std::stoi(config["port"]);
        }
        try
        {
            connectWithServer(server, port);
        }
        catch (...)
        {
            std::cerr << "Error: Can not find Server\n";
            prom.set_value(code);
            return prom.get_future();
        }
        authenticateWithServer();
    }
    else
    {
        prom.set_value(code);
    }
    return prom.get_future();
}

void Worker::processAuthResultMessage(const AuthResultMessage &msg)
{
    if (!msg.getStatus())
    {
        switch ((TaskType)task->getType())
        {
        case TaskType::WORKERSTART: {
            // TODO Might check return value of sysinfo
            struct sysinfo info;
            sysinfo(&info);
            HardwareDetailMessage detail = { std::thread::hardware_concurrency(), info.totalram / (1024 * 1024), "GNU/Linux" };
            communicator->send(detail);
            std::thread([]() {
                std::string cmd;
                while (1)
                {
                    std::cin >> cmd;
                    if (cmd == "stop")
                    {
                        exit(0);
                    }
                }
            }).detach();
            break;
        }
        default:
            throw std::runtime_error("Sadly not implemented yet :(");
            break;
        }
    }
    else
    {
        if (!publicauthfailed && msg.getStatus() == 1)
        {
            publicauthfailed = true;
            authenticateWithServer();
        }
        else
        {
            std::cerr << "Error: Could not authenticate to the Server\n";
            prom.set_value(-1);
        }
    }
}

void Worker::processWorkerLoadRequestMessage(const WorkerLoadRequestMessage &msg) {
    // This might need more adjustments
    // TODO Are threads the virtual number of threads reserved by Jobs and number of threads never used
    // Might check return value of sysinfo
    struct sysinfo info;
    sysinfo(&info);
    WorkerLoadResponseMessage resp(info.loads[0], (info.loads[0] * std::thread::hardware_concurrency()) / 100, std::thread::hardware_concurrency(), info.freeram / (1024 * 1024), info.totalram / (1024 * 1024), info.freeswap / (1024 * 1024), info.totalswap / (1024 * 1024));
    communicator->send(resp);
}

void Worker::processTaskMessage(const TaskMessage &msg)
{
    auto task = msg.GetTask();
    std::thread([task, com = this->communicator, this]() {
        Docker docker;
        try
        {
            switch (task.getType())
            {
#if 0 /* UNUSED FEATURE */
                case TaskType::ADD_IMAGE: {
                auto& content = task.getAddImageFileContent();
                if(content.empty()) {
                    throw std::runtime_error("Task lacks ImageFileContent");
                }
                docker.BuildImage(task.getAddImageName(), content);
                Task response;
                response.setType(TaskType::ADD_IMAGE);
                response.setJobId(task.getJobId());
                TaskMessage taskmess(response);
                com->send(taskmess);
                break;
            }
            case TaskType::REMOVE_IMAGE: {
                docker.RemoveImage(task.getRemoveImageName());
                Task response;
                response.setType(TaskType::REMOVE_IMAGE);
                response.setJobId(task.getJobId());
                TaskMessage taskmess(response);
                com->send(taskmess);
                break;
            }
#endif /* UNUSED FEATURE */
            case TaskType::RUN: {
                // Backup id contains timestamp
                // auto&& dockerfile = task.getAddImageFileContent();
                // if(!dockerfile.empty() && task.getBackupId().has_value())
                //     docker.UpdateImage(task.getConfig()->image(), dockerfile, *task.getBackupId());
                auto container = docker.Run(task);
                #if 0 /* Now use jobid */
                // ToDo save the taskid / containerid mapping
                {
                    std::lock_guard<std::mutex> guard(midtodocker);
                    idtodocker[std::to_string(task.getJobId().value_or(0))] = container.GetId();
                }
                #endif
                TaskResponseMessage resp(task.getJobId().value_or(0), balancedbanana::database::JobStatus::processing);
                com->send(resp);
                RespondToClientMessage respondClient(std::to_string(task.getJobId().value_or(0)), task.getConfig()->blocking_mode().value_or(true), task.getClientId().value_or(0));
                com->send(respondClient);
                auto exitcode = container.Wait();
                Task response;
                response.setType(TaskType::RUN);
                response.setUserId(exitcode);
                // Spec said 200 lines
                response.setAddImageFileContent(container.Tail(200));
                response.setJobId(task.getJobId());
                TaskMessage taskmess(response);
                com->send(taskmess);
                break;
            }

            case TaskType::TAIL: {
                Container container("bbdjob" + std::to_string(*task.getJobId()));
                #if 0 /* Now use jobid */
                {
                    std::lock_guard<std::mutex> guard(midtodocker);
                    container = idtodocker[std::to_string(task.getJobId().value_or(0))];
                }
                #endif
                // Spec said 200 lines
                auto lines = container.Tail(200);
                
                RespondToClientMessage response(lines, true, task.getClientId().value_or(0));

                com->send(response);
                break;
            }

            case TaskType::PAUSE: {
                Container container("bbdjob" + std::to_string(*task.getJobId()));
                #if 0 /* Now use jobid */
                {
                    std::lock_guard<std::mutex> guard(midtodocker);
                    container = idtodocker[std::to_string(task.getJobId().value_or(0))];
                }
                #endif
                container.Pause();
                TaskResponseMessage resp(task.getJobId().value_or(0), balancedbanana::database::JobStatus::paused);
                com->send(resp);
                RespondToClientMessage respondClient("Success", true, task.getClientId().value_or(0));
                com->send(respondClient);
                break;
            }

            case TaskType::CONTINUE: {
                Container container("bbdjob" + std::to_string(*task.getJobId()));
                #if 0 /* Now use jobid */
                {
                    std::lock_guard<std::mutex> guard(midtodocker);
                    container = idtodocker[std::to_string(task.getJobId().value_or(0))];
                }
                #endif
                container.Continue();
                TaskResponseMessage resp(task.getJobId().value_or(0), balancedbanana::database::JobStatus::processing);
                com->send(resp);
                RespondToClientMessage respondClient("Success", true, task.getClientId().value_or(0));
                com->send(respondClient);
                break;
            }

            case TaskType::BACKUP:
            {
                // find job container
                Container container("");
                {
                    std::lock_guard<std::mutex> guard(midtodocker);
                    container = idtodocker[std::to_string(task.getJobId().value_or(0))];
                }

                // make checkpoint of container
                std::string checkpointID;
                auto checkpoint = container.CreateCheckpoint(checkpointID);

                RespondToClientMessage response(checkpoint.GetId(), true, task.getJobId().value_or(0));
                com->send(response);

                break;
            }

            case TaskType::RESTORE:
            {
                // find job container
                Container container("");
                {
                    std::lock_guard<std::mutex> guard(midtodocker);
                    container = idtodocker[std::to_string(task.getJobId().value_or(0))];
                }

                // restore checkpoint of container

                // find checkpoint
                auto checkpoints = container.GetCheckpoints();

                for (auto checkpoint : checkpoints) {
                    if (checkpoint.GetId().compare(std::to_string(task.getBackupId().value_or(-1))) == 0) {
                        // stop container
                        try {
                            container.Stop();
                        } catch (std::runtime_error& e) {
                            RespondToClientMessage response(e.what(), true, task.getJobId().value_or(0));
                            com->send(response);
                            break;
                        }

                        // and restart checkpoint
                        checkpoint.Start();

                        TaskResponseMessage resp(task.getJobId().value_or(0), balancedbanana::database::JobStatus::processing);
                        com->send(resp);

                        RespondToClientMessage response("Success.", true, task.getJobId().value_or(0));
                        com->send(response);
                    }
                }

                RespondToClientMessage response("Failure: Backup not found.", true, task.getJobId().value_or(0));
                com->send(response);

                break;
            }

            case TaskType::STATUS:
            {
                // find job container
                Container container("");
                {
                    std::lock_guard<std::mutex> guard(midtodocker);
                    container = idtodocker[std::to_string(task.getJobId().value_or(0))];
                }

                // determine job status
                // on second thought: this request can be (and is) handled entirely scheduler sided

                RespondToClientMessage response("", true, task.getJobId().value_or(0));
                com->send(response);

                break;
            }

            case TaskType::STOP:
            {
                // find job container
                Container container("");
                {
                    std::lock_guard<std::mutex> guard(midtodocker);
                    container = idtodocker[std::to_string(task.getJobId().value_or(0))];
                }

                // abort job (force cancel)
                try {
                    container.Stop();
                    TaskResponseMessage resp(task.getJobId().value_or(0), balancedbanana::database::JobStatus::canceled);
                    com->send(resp);
                    RespondToClientMessage response("Success.", true, task.getJobId().value_or(0));
                    com->send(response);
                } catch (std::runtime_error& e) {
                    RespondToClientMessage response(e.what(), true, task.getJobId().value_or(0));
                    com->send(response);
                    break;
                }

                break;
            }

            default:
                throw std::runtime_error("Not Implented yet :(");
            }
        }
        catch (const std::exception &ex)
        {
            std::cout << "Internal Error: " << ex.what() << "\n";
            Task response;
            response.setType(TaskType::HELP);
            response.setAddImageFileContent(ex.what());
            response.setJobId(task.getJobId());
            TaskMessage taskmess(response);
            com->send(taskmess);
        }
    }).detach();
}
