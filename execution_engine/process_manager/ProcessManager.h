#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include <string>
#include<vector>

// Manages the execution, lifecycle, and resource tracking of submitted processes
class ProcessManager {

public:

    // function to execute a compiled program
    //
    // executable:
    // path of the compiled executable
    //
    // inputFile:
    // path of the file containing test case input
    //
    // outputFile:
    // path where the program output will be stored
    //
    // workingDirectory:
    // directory in which the submitted program will execute
    //
    // memoryUsed:
    // reference used to return the memory consumed by the process
    //
    // return values:
    // 0 or another positive value -> process exit code
    // -1 -> process/manager error
    // -2 -> time limit exceeded
    static int runProcess(
        const std::string& executable,
        const std::string& inputFile,
        const std::string& outputFile,
        const std::string& workingDirectory,
        long long& memoryUsed
    );


    // Execute a program with command-line arguments.
    static int runProcess(
        const std::vector<std::string>& command,
        const std::string& inputFile,
        const std::string& outputFile,
        const std::string& workingDirectory,
        long long& memoryUsed
    );
};

#endif
