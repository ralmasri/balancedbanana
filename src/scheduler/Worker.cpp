#include <scheduler/Worker.h>
#include <scheduler/SchedulerWorkerMP.h>
#include <communication/Communicator.h>
#include <communication/message/WorkerLoadRequestMessage.h>
#include <communication/message/WorkerLoadResponseMessage.h>
#include <mutex>

using namespace balancedbanana::communication;
using namespace balancedbanana::database;
using namespace balancedbanana::scheduler;

Worker::Worker(uint64_t id, const std::string &name, const std::string &publickey, const std::optional<database::Specs>
        &specs) :
IUser(name, publickey),
               id(id), specs(specs), connected(false), address(""), comm(nullptr), resp(0, 0, 0, 0, 0, 0, 0), mtx(), cnd(){
}

void Worker::send(const Message & msg) {
    std::lock_guard guard(mtx);
    comm->send(msg);
}

bool Worker::isConnected() {
    std::lock_guard guard(mtx);
    return connected;
}

std::optional<Specs> Worker::getSpec() {
    std::lock_guard guard(mtx);
    return specs;
}
void Worker::setSpec(const std::optional<Specs>& specs) {
    {
        std::lock_guard guard(mtx);
        this->specs = specs;
    }
    Update(balancedbanana::scheduler::WorkerObservableEvent::HARDWARE_DETAIL_UPDATE);
}

uint64_t Worker::getId() {
    std::lock_guard guard(mtx);
    return id;
}

const std::string &Worker::getAddress() {
    std::lock_guard lock(mtx);
    return address;
}
void Worker::setAddress(const std::string &adr) {
    {
        std::lock_guard lock(mtx);
        address = adr;
    }
    Update(balancedbanana::scheduler::WorkerObservableEvent::DATA_CHANGE);
}

void Worker::setCommunicator(const std::shared_ptr<communication::Communicator>& com) {
    comm = com;
    connected = com != nullptr;
    if(connected) {
        auto mp = std::static_pointer_cast<SchedulerWorkerMP>(com->GetMP());
        mp->OnWorkerLoadResponse([this](const WorkerLoadResponseMessage& res) {
            resp = res;
            cnd.notify_all();
        });
    }
}

const WorkerLoadResponseMessage& Worker::GetWorkerLoad() {
    std::unique_lock<std::mutex> lock(mtx);
    WorkerLoadRequestMessage request;
    comm->send(request);
    if(cnd.wait_for(lock, std::chrono::seconds(5)) == std::cv_status::timeout) {
        throw std::runtime_error("Timeout");
    }
    return resp;
}
