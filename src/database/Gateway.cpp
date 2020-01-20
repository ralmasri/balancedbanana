#include <database/Gateway.h>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>

using namespace balancedbanana::configfiles;
using namespace balancedbanana::database;

Gateway::Gateway() {
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("placeholder");
    db.setDatabaseName("placeholder");
    db.setUserName("placeholder");
    db.setPassword("placeholder");

    if(!db.isOpen()){
        qDebug() << "Error: connection with database failed.";
    } else {
        qDebug() << "Database: connection was successful.";
    }
}

uint64_t Gateway::addWorker(std::string public_key, int space, int ram, int cores, const std::string  address) {    
    // Check args
    
    //TODO Implement public_key check
    //TODO Implemment address check
    if (space <= 0 || ram <=0 || cores <=0){
        qDebug() << "addWorker error: negative args";
    }

    // Converting the various args into QVariant Objects
    QVariant q_public_key = QString::fromStdString(public_key);
    QVariant q_space = QVariant::fromValue(space);
    QVariant q_ram = QVariant::fromValue(ram);
    QVariant q_cores = QVariant::fromValue(cores);
    QVariant q_address = QString::fromStdString(address);

    // Creating the query
    QSqlQuery query("INSERT INTO workers (key, space, ram, cores, address) VALUES (?, ?, ?, ?, ?)");
    query.addBindValue(q_public_key);
    query.addBindValue(q_space);
    query.addBindValue(q_ram);
    query.addBindValue(q_cores);
    query.addBindValue(q_address);

    // Executing the query.
    if (!query.exec()){
        qDebug() << "addWorker error: " << query.lastError();
    }
    //TODO Create the ids and return them
    //TODO Return id for success or -1 for error.

}

//Removes a worker.
bool Gateway::removeWorker(const uint64_t id) {
}

worker_details Gateway::getWorker(const uint64_t worker_id) {
}

std::vector<std::shared_ptr<worker_details>> Gateway::getWorkers() {
}

//Adds a new Job to the database and returns its ID.
uint64_t Gateway::addJob(uint64_t user_id, const JobConfig& config, const std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> schedule_time, const std::string &command) {
}

bool Gateway::removeJob(const uint64_t job_id) {
}

job_details Gateway::getJob(const uint64_t job_id) {
}

std::vector<std::shared_ptr<job_details>> Gateway::getJobs() {
}

//Adds a user to the database and returns their ID.
uint64_t Gateway::addUser(const std::string name, const std::string email, int auth_key) {
}

bool Gateway::removeUser(const uint64_t user_id) {
}

user_details Gateway::getUser(const uint64_t user_id) {
}

std::vector<std::shared_ptr<user_details>> Gateway::getUsers() {
}

//Assigns a Worker (or a partition of a Worker) to a Job. The Job has now been started.
bool Gateway::startJob(const uint64_t job_id, const uint64_t worker_id, const Specs specs) {
}

bool Gateway::finishJob(const uint64_t job_id, const std::chrono::time_point<std::chrono::system_clock> finish_time, const std::string stdout, const int8_t exit_code) {
}

job_result Gateway::getJobResult(const uint64_t job_id) {
}