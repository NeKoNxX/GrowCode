#include "Scheduler.h"

// Initializes the scheduler
Scheduler::Scheduler() : stopping(false){}

// Adds a job to the queue
void Scheduler::addJob(int jobId)
{
    {
        // Lock queue for thread safety
        std::lock_guard<std::mutex> lock(queueMutex);

        // Reject new jobs if shutting down
        if (stopping) {
            return;
        }

        // add the job id to the end of the queue
        jobQueue.push(jobId);
    }

    // Wake up one waiting worker thread
    condition.notify_one();
}

// function to get the next job from the scheduler queue
int Scheduler::getNextJob()
{
    // unique_lock is used because the condition variable
    // temporarily releases the mutex while the thread waits
    std::unique_lock<std::mutex> lock(queueMutex);

    // wait until either:
    // 1. the scheduler is shutting down
    // 2. a job becomes available in the queue
    condition.wait(lock, [this]() {
        return stopping || !jobQueue.empty();
    });

    // if the queue is empty, there is no job left to process
    // this normally happens when the scheduler has been shut down
    if (jobQueue.empty()) {
        return -1;
    }

    // get the job id from the front of the queue
    int jobId = jobQueue.front();

    // remove the job from the queue
    jobQueue.pop();

    // return the job id to the worker
    return jobId;
}

// function to shut down the scheduler
void Scheduler::shutdown()
{
    {
        // lock the queue before changing the stopping state
        std::lock_guard<std::mutex> lock(queueMutex);

        // mark the scheduler as stopping
        // no new jobs will be accepted
        stopping = true;
    }

    // wake up all waiting worker threads
    // so they can check the stopping condition
    condition.notify_all();
}

// function to check whether the scheduler queue is empty
bool Scheduler::empty()
{
    // lock the queue while checking its state
    std::lock_guard<std::mutex> lock(queueMutex);

    // return true if there are no jobs in the queue
    return jobQueue.empty();
}
