#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <queue>
#include <mutex>
#include <condition_variable>

class Scheduler {
private:

    // queue stores submission IDs waiting for execution
    std::queue<int> jobQueue;

    // mutex protects the queue from simultaneous access
    // by multiple worker threads
    std::mutex queueMutex;

    // condition variable allows workers to wait
    // until a job becomes available
    std::condition_variable condition;

    // indicates whether the scheduler is shutting down
    bool stopping;

public:

    // constructor to initialize the scheduler
    Scheduler();

    // adds a submission/job to the queue
    void addJob(int submissionId);

    // gets the next submission/job from the queue
    int getNextJob();

    // stops the scheduler
    void shutdown();

    // checks whether the queue is empty
    bool empty();
};

#endif
