#include <scheduler/SchedulerWorkerMP.h>

using namespace balancedbanana::communication;

#if 0
SchedulerWorkerMP::SchedulerWorkerMP(balancedbanana::communication::Communicator *communicator) :
MessageProcessor(communicator){
}
#endif

void SchedulerWorkerMP::processHardwareDetailMessage(const HardwareDetailMessage &msg) {
    //TODO implement
}

void SchedulerWorkerMP::processPublicKeyAuthMessage(const PublicKeyAuthMessage &msg) {
    //TODO implement
}

void SchedulerWorkerMP::processSnapshotMessage(const SnapshotMessage &msg) {
    //TODO implement
}

void SchedulerWorkerMP::processTaskMessage(const TaskMessage &msg) {
    //TODO implement
}

void SchedulerWorkerMP::processWorkerAuthMessage(const WorkerAuthMessage &msg) {
    //TODO implement
}