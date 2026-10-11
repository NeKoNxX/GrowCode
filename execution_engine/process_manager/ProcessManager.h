#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include <string>

// ProcessManager is responsible for creating,
// executing and controlling the submitted program process
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

};

#endif
