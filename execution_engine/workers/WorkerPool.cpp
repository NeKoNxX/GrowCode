#include "WorkerPool.h"

#include <iostream>


// constructor to initialize the WorkerPool
WorkerPool::WorkerPool(Scheduler& scheduler, ExecutionWorker& executionWorker, int workerCount)
    : scheduler(scheduler), executionWorker(executionWorker), workerCount(workerCount), running(false)
{
}


// function to start the worker pool
void WorkerPool::start()
{
    // if the worker pool is already running,
    // do not create another set of worker threads
    if (running) {
        return;
    }

    // mark the worker pool as running
    running = true;

    // create the required number of worker threads
    for (int i = 0; i < workerCount; ++i) {
        // each thread executes workerLoop()
        workers.emplace_back(&WorkerPool::workerLoop, this);
    }
}


// function to stop the worker pool
void WorkerPool::stop()
{
    // if the worker pool is already stopped, there is nothing to do
    if (!running) {
        return;
    }

    // mark the worker pool as stopped
    running = false;

    // shut down the scheduler, this wakes up workers waiting for jobs
    scheduler.shutdown();

    // wait for all worker threads to finish
    for (auto& worker : workers) {

        // join the thread if it is still running
        if (worker.joinable()) {
            worker.join();
        }
    }

    // remove all completed worker threads from the vector
    workers.clear();
}


// function executed by every worker thread
void WorkerPool::workerLoop()
{
    // continuously process jobs
    while (true) {

        // get the next submission from the scheduler, if the queue is empty, this call can block until a job becomes available or the scheduler shuts down
        int submissionId = scheduler.getNextJob();


        // -1 indicates that the scheduler has been shut down and there are no more jobs to process
        if (submissionId == -1) {
            break;
        }


        // display which worker thread picked the submission
        std::cout << "Worker " << std::this_thread::get_id() << " picked Submission " << submissionId << std::endl;

        // send the submission to ExecutionWorker for compilation and execution
        executionWorker.processJob(submissionId);
    }
}
