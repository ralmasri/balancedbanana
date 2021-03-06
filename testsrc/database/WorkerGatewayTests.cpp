#include <gtest/gtest.h>
#include <database/WorkerGateway.h>
#include <database/worker_details.h>
#include <database/Repository.h>
#include <database/Utilities.h>

#include <QSqlQuery>
#include <QVariant>
#include <QDebug>
#include <QSqlError>
#include <QSqlRecord>
#include <random>
#include <QTextCodec>
#include <iostream>
#include <string>
#include <locale>

#include "DatabaseTest.h"

using namespace balancedbanana::database;

using WorkerGatewayTest = DatabaseTest;

/**
 * Deletes the all records in the workers table and resets the auto increment for the id.
 */
void resetWorkerTable(const QSqlDatabase &db){
    QSqlQuery query("ALTER TABLE workers CHANGE COLUMN `id` `id` BIGINT(10) UNSIGNED NOT NULL", db);
    query.exec();
    query.prepare("DELETE FROM workers");
    query.exec();
    query.prepare("ALTER TABLE workers CHANGE COLUMN `id` `id` BIGINT(10) UNSIGNED NOT NULL AUTO_INCREMENT");
    query.exec();
}

/**
 * Fixture class that initializes a sample worker's details.
 */
class AddWorkerTest : public WorkerGatewayTest {
protected:
    void SetUp() override {
        WorkerGatewayTest::SetUp();
        details.public_key = "34nrhk3hkr";
        Specs specs{};
        specs.osIdentifier = "Windows 10.4.1.4";
        specs.cores = 4;
        specs.ram = 16234;
        details.specs = specs;
        details.name = "CentOS";
        details.empty = false;
        details.id = 1;
    }

    void TearDown() override {
        resetWorkerTable(db);
    }

    worker_details details;
};

/**
 * Checks if the add query was successful.
 * @param details The record details that were added with the query.
 * @param id The id of the added record.
 * @return true if the add was successful, otherwise false.
 */
bool wasWorkerAddSuccessful(const worker_details& details, uint64_t id, const QSqlDatabase &db){
    QSqlQuery query("SELECT * FROM workers WHERE id = ?", db);
    query.addBindValue(QVariant::fromValue(id));
    if (query.exec()){
        if (query.next()){
            int nameIndex = query.record().indexOf("name");
            int ramIndex = query.record().indexOf("ram");
            int coresIndex = query.record().indexOf("cores");
            int osIndex = query.record().indexOf("osIdentifier");
            int keyIndex = query.record().indexOf("public_key");

            worker_details queryDetails{};
            queryDetails.name = query.value(nameIndex).toString().toStdString();
            if (query.value(coresIndex).isNull()){
                queryDetails.specs = std::nullopt;
            } else {
                Specs specs{};
                specs.cores = query.value(coresIndex).toUInt();
                specs.ram = query.value(ramIndex).toUInt();
                specs.osIdentifier = query.value(osIndex).toString().toStdString();
                queryDetails.specs = specs;
            }
            queryDetails.public_key = query.value(keyIndex).toString().toStdString();
            queryDetails.id = id;
            queryDetails.empty = false;
            EXPECT_TRUE(queryDetails == details);
            return true;
        } else {
            qDebug() << "record not found";
            return false;
        }
    } else {
        qDebug() << "query failed" << query.lastError();
        return false;
    }

}


// Test checks if the addWorker method in Gateway works properly given correct parameters
TEST_F(AddWorkerTest, AddWorkerTest_AddFirstWorkerSuccess_Test){

    // The first entry's id should be 1
    EXPECT_TRUE(workerGateway->addWorker(details) == 1);

    // The add must be successful
    EXPECT_TRUE(wasWorkerAddSuccessful(details, 1, db));
}

// Test to see if the auto increment feature works as expected.
// Adds the workers from the AddWorkerTest fixture
TEST_F(AddWorkerTest, AddWorkerTest_AddSecondWorkerSuccess_Test){

    // Add the worker from the first test. Since it's the first worker, its id should be 1.
    EXPECT_TRUE(workerGateway->addWorker(details) == 1);
    EXPECT_TRUE(wasWorkerAddSuccessful(details, 1, db));

    // Initialize a new worker
    worker_details seconddetails{};
    seconddetails.public_key = "sadfjsaljdf";
    seconddetails.specs = details.specs;
    seconddetails.name = "Ubuntu";
    seconddetails.id = 2;
    seconddetails.empty = false;

    EXPECT_TRUE(workerGateway->addWorker(seconddetails) == 2);
    EXPECT_TRUE(wasWorkerAddSuccessful(seconddetails, 2, db));
}

/**
 * Fixture class that deletes the workers table on setup and restores it on teardown.
 */
class NoWorkersTableTest : public WorkerGatewayTest {
protected:
    void SetUp() override {
        WorkerGatewayTest::SetUp();
        // Deletes the workers table
        QSqlQuery query("DROP TABLE workers", db);
        query.exec();

        // Setup the varaibles needed
        details.public_key = "34nrhk3hkr";
        Specs specs{};
        specs.osIdentifier = "Ubuntu 18.04.62";
        specs.ram = 16384;
        specs.cores = 4;
        details.specs = specs;
        details.name = "CentOS";
        id = 1;
        details.empty = false;
    }

    void TearDown() override {
        QSqlQuery query("CREATE TABLE IF NOT EXISTS `balancedbanana`.`workers`\n"
                        "(\n"
                        "    `id`         BIGINT(10) UNSIGNED NOT NULL AUTO_INCREMENT,\n"
                        "    `ram`        BIGINT(10) UNSIGNED NULL DEFAULT NULL,\n"
                        "    `cores`      INT(10) UNSIGNED NULL DEFAULT NULL,\n"
                        "    `osIdentifier`   TEXT NULL DEFAULT NULL,\n"
                        "    `public_key` LONGTEXT NOT NULL,\n"
                        "    `name`       VARCHAR(255) NOT NULL,\n"
                        "    PRIMARY KEY (`id`),\n"
                        "    UNIQUE INDEX `id_UNIQUE` (`id` ASC),\n"
                        "    UNIQUE INDEX `name_UNIQUE` (`name` ASC)\n"
                        ")\n"
                        "ENGINE = InnoDB\n"
                        "DEFAULT CHARACTER SET = utf8", db);
        query.exec();
    }

    worker_details details;
    uint64_t id{};
};

// Test to see if an exception is thrown when a worker is being added, but no workers' table exists.
TEST_F(NoWorkersTableTest, NoWorkersTableTest_AddWorker_Test){
    EXPECT_THROW(workerGateway->addWorker(details), std::logic_error);
}

// Test to see if an exception is thrown when a worker is being removed, but no workers' table exists.
TEST_F(NoWorkersTableTest, NoWorkersTableTest_RemoveWorker_Test){
    EXPECT_THROW(workerGateway->removeWorker(id), std::logic_error);
}

// Test to see if an exception is thrown when the worker getter is called, but no workers' table exists.
TEST_F(NoWorkersTableTest, NoWorkersTableTest_GetWorker_Test){
    EXPECT_THROW(workerGateway->getWorker(id), std::logic_error);
}

// Test to see if an exception is thrown when the workers getter is called, but no workers' table exists.
TEST_F(NoWorkersTableTest, NoWorkersTableTest_GetWorkers_Test){
    EXPECT_THROW(workerGateway->getWorkers(), std::logic_error);
}

/**
 * Check if the remove query on id was successful
 * @param id The id of the removed record.
 * @return  true if remove was successful, otherwise false.
 */
bool wasWorkerRemoveSuccessful(uint64_t id, const QSqlDatabase &db){
    QSqlQuery query("SELECT * FROM workers WHERE id = ?", db);
    query.addBindValue(QVariant::fromValue(id));
    if (query.exec()){
        return !query.next();
    } else {
        qDebug() << "wasWorkerRemoveSuccessful error: " << query.lastError();
        return false;
    }
}

/**
 * Fixture class that resets the auto increment on teardown.
 */
class RemoveWorkerTest : public WorkerGatewayTest {
protected:
    void TearDown() override{
        resetWorkerTable(db);
    }
};

// Test to see if a worker is removed after successfully.
TEST_F(RemoveWorkerTest, RemoveWorkerTest_SuccessfulRemove_Test){
    // Add a worker
    worker_details details{};
    details.public_key = "34nrhk3hkr";
    Specs specs{};
    specs.osIdentifier = "Kubuntu 18.0123";
    specs.ram = 16384;
    specs.cores = 6;
    details.specs = specs;
    details.name = "CentOS";
    details.id = 1;
    details.empty = false;
    // Since this is the first worker, this has to be true.
    EXPECT_TRUE(workerGateway->addWorker(details) == 1);
    EXPECT_TRUE(wasWorkerAddSuccessful(details, 1, db));

    // This must work
    EXPECT_NO_THROW(workerGateway->removeWorker(1));
    EXPECT_TRUE(wasWorkerRemoveSuccessful(1, db));
}

// Test to see if the remove method fails when it's called with an invalid id.
TEST_F(RemoveWorkerTest, RemoveWorkerTest_FailureRemove_Test){
    EXPECT_THROW(workerGateway->removeWorker(1), std::runtime_error);
}


/**
 * Fixture class that initializes a sample worker on setUp and resets the table on teardown.
 */
class GetWorkerTest : public WorkerGatewayTest {
protected:
    void SetUp() override {
        WorkerGatewayTest::SetUp();
        details.public_key = "34nrhk3hkr";
        Specs specs{};
        specs.osIdentifier = "UNIX";
        specs.ram = 16384;
        specs.cores = 4;
        details.specs = specs;
        details.name = "CentOS";
        details.id = 1;
        details.empty = false;
    }

    void TearDown() override {
        resetWorkerTable(db);
    }

    worker_details details;
};

// Test to see if the first worker can be retrieved correctly.
TEST_F(GetWorkerTest, GetWorkerTest_SuccessfulGet_Test){
    // Add the worker. Its id should be 1, since it's the first worker to be added.
    EXPECT_EQ(workerGateway->addWorker(details), details.id);

    // Get the worker and compare it to the added worker. They should be equal.
    worker_details expected_details = workerGateway->getWorker(details.id);
    EXPECT_TRUE(details == expected_details);
}

// Test to see if the getter method returns an empty worker_details when its called with an invalid id
TEST_F(GetWorkerTest, GetWorkerTest_NonExistentWorker_Test){
    EXPECT_THROW(workerGateway->getWorker(1), std::runtime_error);
}

/**
 * Fixture class that initializes three workers on setup and resets the table on teardown.
 */
class GetWorkersTest : public WorkerGatewayTest {
protected:
    void SetUp() override {
        WorkerGatewayTest::SetUp();
        // Set up the first worker
        first.public_key = "34nrhk3hkr";
        Specs firstSpecs{};
        firstSpecs.osIdentifier = "some os";
        firstSpecs.ram = 16384;
        firstSpecs.cores = 4;
        first.specs = firstSpecs;
        first.name = "CentOS";
        first.id = 1;
        first.empty = false;

        // Set up the second worker
        second.public_key = "fsd8iasdf8sadf";
        second.specs = firstSpecs;
        second.specs->ram = 17385;
        second.name = "Ubuntu";
        second.id = 2;
        second.empty = false;

        // Set up the third worker
        third.public_key = "asdfascascsd";
        third.specs = firstSpecs;
        third.specs->cores = 10;
        third.name = "Windows";
        third.id = 3;
        third.empty = false;
    }

    void TearDown() override {
        resetWorkerTable(db);
    }

    worker_details first;
    worker_details second;
    worker_details third;
};

// Test to see if getWorkers retrieves a vector of previously added workers from the database
TEST_F(GetWorkersTest, GetWorkersTest_SuccessfulGet_Test){
    // Add the workers. Their ids should match the order of their addition.
    EXPECT_EQ(workerGateway->addWorker(first), first.id);
    EXPECT_EQ(workerGateway->addWorker(second), second.id);
    EXPECT_EQ(workerGateway->addWorker(third), third.id);

    std::vector<worker_details> expectedVector;
    expectedVector.push_back(first);
    expectedVector.push_back(second);
    expectedVector.push_back(third);

    std::vector<worker_details> actualVector = workerGateway->getWorkers();
    EXPECT_TRUE(Utilities::areDetailVectorsEqual(expectedVector, actualVector));
}

// Test to see if the getter method returns an empty vector if the workers table is empty
TEST_F(GetWorkersTest, GetWorkersTest_NonExistentWorkers_Test){
    EXPECT_TRUE(workerGateway->getWorkers().empty());
}

class GetWorkerByNameTest : public WorkerGatewayTest {
protected:
    void SetUp() override {
        WorkerGatewayTest::SetUp();
        worker.public_key = "34nrhk3hkr";
        Specs specs{};
        specs.osIdentifier = "10240";
        specs.ram = 16384;
        specs.cores = 4;
        worker.specs = specs;
        worker.name = "CentOS";
        worker.id = 1;
        worker.empty = false;
    }

    void TearDown() override {
        resetWorkerTable(db);
    }

    worker_details worker;
};

TEST_F(GetWorkerByNameTest, GetWorkerByNameTest_NoWorkersTable_Test){
    QSqlQuery query("DROP TABLE workers", db);
    query.exec();
    EXPECT_THROW(workerGateway->getWorkerByName(worker.name), std::logic_error);
    query.prepare("CREATE TABLE IF NOT EXISTS `balancedbanana`.`workers`\n"
                  "(\n"
                  "    `id`         BIGINT(10) UNSIGNED NOT NULL AUTO_INCREMENT,\n"
                  "    `ram`        BIGINT(10) UNSIGNED NULL DEFAULT NULL,\n"
                  "    `cores`      INT(10) UNSIGNED NULL DEFAULT NULL,\n"
                  "    `osIdentifier`   TEXT NULL DEFAULT NULL,\n"
                  "    `public_key` LONGTEXT NOT NULL,\n"
                  "    `name`       VARCHAR(255) NOT NULL,\n"
                  "    PRIMARY KEY (`id`),\n"
                  "    UNIQUE INDEX `id_UNIQUE` (`id` ASC),\n"
                  "    UNIQUE INDEX `name_UNIQUE` (`name` ASC)\n"
                  ")\n"
                  "ENGINE = InnoDB\n"
                  "DEFAULT CHARACTER SET = utf8");
    query.exec();
}

TEST_F(GetWorkerByNameTest, GetWorkerByNameTest_WorkerFound_Test){
    EXPECT_EQ(workerGateway->addWorker(worker), worker.id);
    EXPECT_TRUE(wasWorkerAddSuccessful(worker, worker.id, db));
    EXPECT_TRUE(workerGateway->getWorkerByName(worker.name) == worker);
}

TEST_F(GetWorkerByNameTest, GetWorkerByNameTest_GetWorkerByNameTest_WorkerNotFound_Test_TestFound_Test){
    EXPECT_EQ(workerGateway->getWorkerByName(worker.name).id, 0);
}

class UpdateWorkerTest : public WorkerGatewayTest {
protected:
    void SetUp() override {
        WorkerGatewayTest::SetUp();
        worker.public_key = "34nrhk3hkr";
        Specs specs{};
        specs.osIdentifier = "10240";
        specs.ram = 16384;
        specs.cores = 4;
        worker.specs = specs;
        worker.name = "CentOS";
        worker.id = 1;
        worker.empty = false;
    }

    void TearDown() override {
        resetWorkerTable(db);
    }

    worker_details worker;
};

TEST_F(UpdateWorkerTest, UpdateWorkerTest_NoWorkersTable_Test){
    QSqlQuery query("DROP TABLE workers", db);
    query.exec();
    EXPECT_THROW(workerGateway->updateWorker(worker), std::logic_error);
    query.prepare("CREATE TABLE IF NOT EXISTS `balancedbanana`.`workers`\n"
                  "(\n"
                  "    `id`         BIGINT(10) UNSIGNED NOT NULL AUTO_INCREMENT,\n"
                  "    `ram`        BIGINT(10) UNSIGNED NULL DEFAULT NULL,\n"
                  "    `cores`      INT(10) UNSIGNED NULL DEFAULT NULL,\n"
                  "    `osIdentifier`   TEXT NULL DEFAULT NULL,\n"
                  "    `public_key` LONGTEXT NOT NULL,\n"
                  "    `name`       VARCHAR(255) NOT NULL,\n"
                  "    PRIMARY KEY (`id`),\n"
                  "    UNIQUE INDEX `id_UNIQUE` (`id` ASC),\n"
                  "    UNIQUE INDEX `name_UNIQUE` (`name` ASC)\n"
                  ")\n"
                  "ENGINE = InnoDB\n"
                  "DEFAULT CHARACTER SET = utf8");
    query.exec();
}

TEST_F(UpdateWorkerTest, UpdateWorkerTest_InvalidId_Test){
    worker.id = 0;
    EXPECT_THROW(workerGateway->updateWorker(worker), std::invalid_argument);
}

TEST_F(UpdateWorkerTest, UpdateWorkerTest_NoWorker_Test){
    EXPECT_THROW(workerGateway->updateWorker(worker), std::runtime_error);
}

TEST_F(UpdateWorkerTest, UpdateWorkerTest_Success_Test){
    EXPECT_EQ(workerGateway->addWorker(worker), worker.id);
    EXPECT_TRUE(wasWorkerAddSuccessful(worker, worker.id, db));

    worker_details new_worker = worker;
    new_worker.name = "Windows 10";
    workerGateway->updateWorker(new_worker);
    worker_details actualWorker = workerGateway->getWorker(worker.id);
    EXPECT_TRUE(actualWorker == new_worker);
}

std::string random_string( size_t length )
{
    auto randchar = []() -> char
    {
        const char charset[] =
                "0123456789"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[ rand() % max_index ];
    };
    std::string str(length,0);
    std::generate_n( str.begin(), length, randchar );
    return str;
}

// Generate an unicode string of length 'len' whose characters are in range [start, end]
std::wstring generateRandomUnicodeString(size_t len, size_t start, size_t end)
{
    wchar_t* ustr = new wchar_t[len+1];      // +1 for '\0'
    size_t intervalLength = end - start + 1; // +1 for inclusive range

    srand(time(NULL));
    for (auto i = 0; i < len; i++) {
        ustr[i] = (rand() % intervalLength) + start;
    }
    ustr[len] = L'\0';
    std::wstring wstr(ustr);
    delete[] ustr;
    return wstr;
}

using WorkerEncodingTest = WorkerGatewayTest;

TEST_F(WorkerEncodingTest, WorkerEncodingTest_U8Test_Test){
    /*
    worker_details worker;
    worker.public_key = "safdsadf";
    std::string name = u8"GROẞBUCHSTABEN";
    worker.name = name;
    worker.empty = false;
    worker.id = 1;
    EXPECT_EQ(workerGateway->add(worker), worker.id);
    worker_details actualWorker = workerGateway->getWorker(worker.id);
    EXPECT_EQ(workerGateway->getWorker(worker.id).name, worker.name);
    qDebug() << QString::fromStdString(actualWorker.name) << QString::fromStdString(worker.name);

     */
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<uint32_t> dis(0, std::numeric_limits<uint32_t>::max());
    std::vector<uint32_t> name(4);
    for (auto &&i : name) {
        i = dis(gen);
    }
    std::string sname((char*)name.data(), name.size() * sizeof(uint32_t));

    std::wstring output = generateRandomUnicodeString(5, 0x0400, 0x04FF);
    QSqlQuery query("INSERT INTO workers (name, public_key) VALUES (?,?)", db);
    query.addBindValue(QString::fromStdWString(output));
    query.addBindValue("something");
    query.exec();
    query.prepare("SELECT name FROM workers WHERE id = 1");
    if (query.exec() && query.next()){
        EXPECT_EQ(query.value(0).toString().toStdWString(), output);
    }

    resetWorkerTable(db);
}

