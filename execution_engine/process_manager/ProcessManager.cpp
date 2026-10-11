#include "ProcessManager.h"

#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <signal.h>
#include <chrono>
#include <thread>


// Executes a program and tracks its input, output, execution time, and memory usage
int ProcessManager::runProcess(
    const std::string& executable,
    const std::string& inputFile,
    const std::string& outputFile,
    const std::string& workingDirectory,
    long long& memoryUsed
) {

    // initialize memory usage to zero
    memoryUsed = 0;


    // create a pipe for communication between
    // the parent process and child process
    int pipeFd[2];

    // create the pipe
    if (pipe(pipeFd) != 0) {
        return -1;
    }


    // create a child process
    pid_t pid = fork();


    // check whether process creation failed
    if (pid < 0) {

        // close both ends of the pipe
        close(pipeFd[0]);
        close(pipeFd[1]);

        return -1;
    }


    // child process
    if (pid == 0) {

        // child does not need the read end of the pipe
        close(pipeFd[0]);


        // create a CPU resource limit structure
        struct rlimit cpuLimit {};


        // set the soft CPU limit to 2 seconds
        cpuLimit.rlim_cur = 2;

        // set the hard CPU limit to 2 seconds
        cpuLimit.rlim_max = 2;


        // apply the CPU time limit to the child process
        setrlimit(
            RLIMIT_CPU,
            &cpuLimit
        );


        // change the current working directory
        // of the child process
        if (chdir(workingDirectory.c_str()) != 0) {
            _exit(1);
        }


        // message used to notify the parent process
        // that execution has started
        const char message[] = "EXECUTION_STARTED";


        // send the message through the pipe
        write(
            pipeFd[1],
            message,
            sizeof(message)
        );


        // close the write end of the pipe
        close(pipeFd[1]);


        // open the input file for reading
        int inputFd = open(
            inputFile.c_str(),
            O_RDONLY
        );


        // open the output file for writing
        // create the file if it does not exist
        // and clear previous contents
        int outputFd = open(
            outputFile.c_str(),
            O_WRONLY | O_CREAT | O_TRUNC,
            0644
        );


        // if either input or output file
        // could not be opened, terminate the child
        if (inputFd < 0 || outputFd < 0) {
            _exit(1);
        }


        // redirect standard input
        // to the test case input file
        dup2(inputFd, STDIN_FILENO);


        // redirect standard output
        // to the program output file
        dup2(outputFd, STDOUT_FILENO);


        // close the original file descriptors
        close(inputFd);
        close(outputFd);


        // replace the child process with
        // the submitted executable
        execl(
            executable.c_str(),
            executable.c_str(),
            static_cast<char*>(nullptr)
        );


        // if execl() returns,
        // execution of the executable failed
        _exit(1);
    }


    // parent does not need the write end of the pipe
    close(pipeFd[1]);


    // buffer used to receive the IPC message
    char message[64] = {};


    // read the execution-start message
    // sent by the child process
    ssize_t bytesRead = read(
        pipeFd[0],
        message,
        sizeof(message) - 1
    );


    // close the read end of the pipe
    close(pipeFd[0]);


    // if a message was received,
    // add the null terminator
    if (bytesRead > 0) {
        message[bytesRead] = '\0';
    }


    // display the message received through IPC
    std::cout
        << "IPC message: "
        << message
        << std::endl;


    // variable used to store the child process status
    int status = 0;


    // structure used to collect resource usage
    struct rusage usage {};


    // maximum allowed execution time
    const double timeLimit = 2.0;


    // record the time at which monitoring starts
    auto startTime =
        std::chrono::steady_clock::now();


    // continuously monitor the child process
    while (true) {

        // check whether the child process has finished
        // WNOHANG prevents the parent from blocking
        pid_t result = wait4(
            pid,
            &status,
            WNOHANG,
            &usage
        );


        // child process has finished
        if (result == pid) {

            // store maximum resident memory usage
            memoryUsed =
                static_cast<long long>(
                    usage.ru_maxrss
                );


            // check whether the process terminated normally
            if (WIFEXITED(status)) {

                // return the program's exit code
                return WEXITSTATUS(status);
            }


            // check whether the process was terminated
            // by a signal
            if (WIFSIGNALED(status)) {

                // return a signal-based exit code
                return 128 + WTERMSIG(status);
            }


            // unexpected process termination state
            return -1;
        }


        // wait4() itself failed
        if (result < 0) {
            return -1;
        }


        // get the current time
        auto currentTime =
            std::chrono::steady_clock::now();


        // calculate how long the process has been running
        double elapsed =
            std::chrono::duration<double>(
                currentTime - startTime
            ).count();


        // check whether the time limit has been exceeded
        if (elapsed >= timeLimit) {

            // forcefully terminate the child process
            kill(pid, SIGKILL);


            // wait for the terminated child process
            // and collect its resource usage
            wait4(
                pid,
                &status,
                0,
                &usage
            );


            // store maximum resident memory usage
            memoryUsed =
                static_cast<long long>(
                    usage.ru_maxrss
                );


            // -2 indicates Time Limit Exceeded
            return -2;
        }


        // wait for a short period before checking
        // the process again
        std::this_thread::sleep_for(
            std::chrono::milliseconds(10)
        );
    }
}
