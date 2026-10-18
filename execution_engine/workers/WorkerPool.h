#ifndef WORKER_POOL_H
#define WORKER_POOL_H

#include <vector>
#include <thread>
#include <atomic>

#include "../scheduler/Scheduler.h"
#include "../executor/ExecutionWorker.h"

// Manages a group of worker threads.
// Each worker takes jobs from the Scheduler and passes them to the ExecutionWorker.
class WorkerPool {
private:

    // Keeps track of the job queue to request new jobs
    Scheduler& scheduler;

    // Compiles, runs, and checks the submitted code
    ExecutionWorker& executionWorker;

   // List of active background threads in this pool
    std::vector<std::thread> workers;

    // Total number of worker threads to create
    int workerCount;

    // Flag to track if the pool is running (thread-safe)
    std::atomic<bool> running;

    // Infinite loop that each background thread runs to process jobs
    void workerLoop();

public:

    // Initializes the pool with a scheduler, executor, and thread count
    WorkerPool(Scheduler& scheduler, ExecutionWorker& executionWorker, int workerCount);

    // Creates the background threads and starts processing jobs
    void start();

    // Shuts down the system and waits for all threads to finish safely
    void stop();
};

#endif
