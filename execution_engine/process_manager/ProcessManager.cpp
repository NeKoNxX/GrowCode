
#include "ProcessManager.h"

#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <signal.h>
#include <chrono>
#include <thread>
#include <cerrno>

// Executes a program and tracks its input, output, execution time, and memory usage.
// This wrapper preserves compatibility with the existing C++ execution path.
int ProcessManager::runProcess(
    const std::string& executable,
    const std::string& inputFile,
    const std::string& outputFile,
    const std::string& workingDirectory,
    long long& memoryUsed,
    const std::string& errorFile
) {
    return runProcess(
        std::vector<std::string>{executable},
        inputFile,
        outputFile,
        workingDirectory,
        memoryUsed,
        errorFile
    );
}

// Executes a program with command-line arguments.
int ProcessManager::runProcess(
    const std::vector<std::string>& command,
    const std::string& inputFile,
    const std::string& outputFile,
    const std::string& workingDirectory,
    long long& memoryUsed,
    const std::string& errorFile
) {
    // Initialize memory usage to zero.
    memoryUsed = 0;

    // Validate the command.
    if (command.empty() || command[0].empty()) {
        return -1;
    }

    // Create a pipe for communication between the parent and child.
    int pipeFd[2];

    if (pipe(pipeFd) != 0) {
        return -1;
    }

    // Create a child process.
    pid_t pid = fork();

    // Check whether process creation failed.
    if (pid < 0) {
        close(pipeFd[0]);
        close(pipeFd[1]);
        return -1;
    }

    // Child process.
    if (pid == 0) {
        // Child does not need the read end of the pipe.
        close(pipeFd[0]);

        // Set CPU resource limits.
        struct rlimit cpuLimit {};

        cpuLimit.rlim_cur = 2;
        cpuLimit.rlim_max = 2;

        setrlimit(RLIMIT_CPU, &cpuLimit);

        // Change the child's working directory.
        if (chdir(workingDirectory.c_str()) != 0) {
            _exit(1);
        }

        // Notify the parent that execution is starting.
        const char message[] = "EXECUTION_STARTED";

        write(pipeFd[1], message, sizeof(message));
        close(pipeFd[1]);

        // Open input and standard-output files.
        int inputFd = open(inputFile.c_str(), O_RDONLY);

        int outputFd = open(
            outputFile.c_str(),
            O_WRONLY | O_CREAT | O_TRUNC,
            0644
        );

        // If requested, capture standard error separately from program output.
        int errorFd = -1;
        if (!errorFile.empty()) {
            errorFd = open(
                errorFile.c_str(),
                O_WRONLY | O_CREAT | O_TRUNC,
                0644
            );
        }

        if (inputFd < 0 || outputFd < 0 ||
            (!errorFile.empty() && errorFd < 0)) {
            if (inputFd >= 0) close(inputFd);
            if (outputFd >= 0) close(outputFd);
            if (errorFd >= 0) close(errorFd);
            _exit(1);
        }

        // Redirect standard input and standard output.
        if (dup2(inputFd, STDIN_FILENO) < 0 ||
            dup2(outputFd, STDOUT_FILENO) < 0) {
            close(inputFd);
            close(outputFd);
            if (errorFd >= 0) close(errorFd);
            _exit(1);
        }

        // Redirect standard error when a diagnostic file was supplied.
        if (errorFd >= 0 && dup2(errorFd, STDERR_FILENO) < 0) {
            close(inputFd);
            close(outputFd);
            close(errorFd);
            _exit(1);
        }

        // Close the original file descriptors after redirection.
        close(inputFd);
        close(outputFd);
        if (errorFd >= 0) close(errorFd);

        // Build the argument array required by execvp().
        // The command strings remain alive in the child process.
        std::vector<char*> args;
        args.reserve(command.size() + 1);

        for (const std::string& arg : command) {
            args.push_back(const_cast<char*>(arg.c_str()));
        }

        args.push_back(nullptr);

        // Replace the child process with the requested program.
        // execvp searches PATH for commands such as java and python3.
        execvp(args[0], args.data());

        // If execvp returns, launching the program failed.
        _exit(127);
    }

    // Parent does not need the write end of the pipe.
    close(pipeFd[1]);

    // Receive the execution-start message.
    char message[64] = {};

    ssize_t bytesRead = read(
        pipeFd[0],
        message,
        sizeof(message) - 1
    );

    close(pipeFd[0]);

    if (bytesRead > 0) {
        message[bytesRead] = '\0';
    }

    std::cout << "IPC message: " << message << std::endl;

    // Child process status and resource usage.
    int status = 0;
    struct rusage usage {};

    // Maximum allowed wall-clock execution time.
    const double timeLimit = 2.0;

    auto startTime = std::chrono::steady_clock::now();

    // Continuously monitor the child process.
    while (true) {
        pid_t result = wait4(pid, &status, WNOHANG, &usage);

        // Child process has finished.
        if (result == pid) {
            memoryUsed = static_cast<long long>(usage.ru_maxrss);

            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            }

            if (WIFSIGNALED(status)) {
                return 128 + WTERMSIG(status);
            }

            return -1;
        }

        // wait4() failed.
        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }

            return -1;
        }

        auto currentTime = std::chrono::steady_clock::now();

        double elapsed = std::chrono::duration<double>(
            currentTime - startTime
        ).count();

        // Time limit exceeded.
        if (elapsed >= timeLimit) {
            kill(pid, SIGKILL);

            // Reap the terminated child and collect resource usage.
            while (wait4(pid, &status, 0, &usage) < 0) {
                if (errno != EINTR) {
                    return -1;
                }
            }

            memoryUsed = static_cast<long long>(usage.ru_maxrss);

            // -2 indicates Time Limit Exceeded.
            return -2;
        }

        // Wait briefly before checking again.
        std::this_thread::sleep_for(
            std::chrono::milliseconds(10)
        );
    }
}
