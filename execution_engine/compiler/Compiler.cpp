
#include "Compiler.h"

#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>

bool Compiler::compile(
    const std::string& sourceFile,
    const std::string& executableFile,
    const std::string& errorFile
) {
    pid_t pid = fork();

    if (pid < 0) {
        return false;
    }

    if (pid == 0) {
        int errorFd = open(
            errorFile.c_str(),
            O_WRONLY | O_CREAT | O_TRUNC,
            0644
        );

        if (errorFd < 0) {
            _exit(127);
        }

        if (dup2(errorFd, STDERR_FILENO) < 0) {
            close(errorFd);
            _exit(127);
        }

        close(errorFd);

        execlp(
            "g++",
            "g++",
            "-std=c++17",
            sourceFile.c_str(),
            "-o",
            executableFile.c_str(),
            static_cast<char*>(nullptr)
        );

        _exit(127);
    }

    int status = 0;

    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            return false;
        }
    }

    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

bool Compiler::compileJava(
    const std::string& sourceFile,
    const std::string& outputDirectory,
    const std::string& errorFile
) {
    pid_t pid = fork();

    if (pid < 0) {
        return false;
    }

    if (pid == 0) {
        int errorFd = open(
            errorFile.c_str(),
            O_WRONLY | O_CREAT | O_TRUNC,
            0644
        );

        if (errorFd < 0) {
            _exit(127);
        }

        if (dup2(errorFd, STDERR_FILENO) < 0) {
            close(errorFd);
            _exit(127);
        }

        close(errorFd);

        execlp(
            "javac",
            "javac",
            "-d",
            outputDirectory.c_str(),
            sourceFile.c_str(),
            static_cast<char*>(nullptr)
        );

        _exit(127);
    }

    int status = 0;

    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            return false;
        }
    }

    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}
