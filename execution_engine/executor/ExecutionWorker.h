#ifndef EXECUTION_WORKER_H
#define EXECUTION_WORKER_H

#include "database/Database.h"

// ExecutionWorker is responsible for processing
// a submitted job received from the WorkerPool
class ExecutionWorker {
private:

    // reference to the database
    // used to retrieve submission information,
    // source code, test cases and store execution results
    Database& database;

public:

    // constructor to initialize the ExecutionWorker
    // with an existing database object
    ExecutionWorker(Database& database);

    // processes a submitted job using its submission ID
    void processJob(int submissionId);
};

#endif
