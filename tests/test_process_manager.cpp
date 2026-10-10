#include "../execution-engine/process_manager/ProcessManager.h"

#include <iostream>
#include <fstream>
#include <filesystem>

int main() {

    namespace fs = std::filesystem;

    // Create the test directory
    fs::create_directories("tests/process_test");

     // Convert relative paths to absolute paths
    std::string executable = fs::absolute("tests/test_program").string();
    std::string inputFile = fs::absolute("tests/process_test/input.txt").string();
    std::string outputFile = fs::absolute("tests/process_test/output.txt").string();
    std::string workingDirectory = fs::absolute("tests/process_test").string();

    // Write the test message to the input file
    {
        std::ofstream input(inputFile);
        input << "Hello ProcessManager";
    }

    // Run the program and track its memory usage
    long long memoryUsed = 0;

    int result = ProcessManager::runProcess(
        executable,
        inputFile,
        outputFile,
        workingDirectory,
        memoryUsed
    );

    std::cout<< "Exit code: "<< result<< std::endl;

    std::cout<< "Memory used: "<< memoryUsed<< std::endl;

    // Read and print the program's output file
    std::ifstream output(outputFile);
    std::string text((std::istreambuf_iterator<char>(output)),std::istreambuf_iterator<char>());

    std::cout << "Program output: " << text << std::endl;

    return 0;
}
