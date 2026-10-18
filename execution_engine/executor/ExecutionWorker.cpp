#include "ExecutionWorker.h"

#include "../compiler/Compiler.h"
#include "../process_manager/ProcessManager.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;


// Removes trailing whitespace, tabs, and newlines from program output
static std::string normalizeOutput(const std::string& output)
{
    // create a copy of the original output
    std::string normalized = output;

    // remove trailing whitespace characters
    while (!normalized.empty() &&
           (normalized.back() == ' ' ||
            normalized.back() == '\t' ||
            normalized.back() == '\n' ||
            normalized.back() == '\r'))
    {
        normalized.pop_back();
    }

    // return cleaned output
    return normalized;
}


// constructor to initialize ExecutionWorker
// with the database object
ExecutionWorker::ExecutionWorker(Database& database) : database(database)
{
}


// Processes a submitted code evaluation job
void ExecutionWorker::processJob(int submissionId)
{
    // display which submission is currently being processed
    std::cout<< "Worker processing Submission " << submissionId << std::endl;


    // update submission status to RUNNING, because execution has started
    database.updateSubmissionStatus(submissionId, "RUNNING");


    // update the corresponding job status to RUNNING
    database.updateJobStatus(submissionId,"RUNNING");


    // get the problem id associated with the submission
    int problemId = database.getSubmissionProblemId(submissionId);


    // retrieve the source code submitted by the user
    std::string code = database.getSubmissionCode(submissionId);


    // if no source code was found, the submission cannot be compiled
    if (code.empty())
    {
        database.updateSubmissionStatus(submissionId, "COMPILATION_ERROR");
        database.updateUserProgress(submissionId, "COMPILATION_ERROR");
        database.updateJobStatus(submissionId, "COMPILATION_ERROR");
        return;
    }


    // retrieve all test cases associated with the problem
    std::vector<TestCase> testCases = database.getTestCases(problemId);

    // if no test cases exist, the submission cannot be properly executed
    if (testCases.empty())
    {
        database.updateSubmissionStatus(submissionId, "RUNTIME_ERROR");
        database.updateJobStatus(submissionId, "FAILED");
        database.updateUserProgress(submissionId, "RUNTIME_ERROR");
        std::cout << "Submission " << submissionId << ": No test cases found" << std::endl;
        return;
    }


    // create directories used for storing
    // source code, executables, outputs and logs
    fs::create_directories("storage/user_code");
    fs::create_directories("storage/executables");
    fs::create_directories("storage/outputs");
    fs::create_directories("storage/logs");


    // create a unique base name using the submission id
    std::string base =  "submission_" + std::to_string(submissionId);

    // create the path where the submitted source code will be stored
    fs::path sourcePath = fs::absolute("storage/user_code/" + base + ".cpp");

    // create the path where the compiled executable will be stored
    fs::path executablePath = fs::absolute("storage/executables/" + base);

    // create the path where compiler errors will be stored
    fs::path compileErrorPath = fs::absolute("storage/logs/" + base + "_compile.txt");

    // create the path where test case input will be stored
    fs::path inputPath = fs::absolute("storage/user_code/" + base + "_input.txt");

    // create the path where program output will be stored
    fs::path outputPath = fs::absolute("storage/outputs/" + base + ".txt");

    // create a separate working directory for execution of this submission
    fs::path workingDirectoryPath = fs::absolute("storage/executions/" + base);


    // convert filesystem paths into strings
    std::string sourceFile = sourcePath.string();
    std::string executableFile = executablePath.string();
    std::string compileErrorFile = compileErrorPath.string();
    std::string inputFile = inputPath.string();
    std::string outputFile = outputPath.string();
    std::string workingDirectory = workingDirectoryPath.string();

    // create the working directory
    fs::create_directories(workingDirectoryPath);

    // create the source-code file
    {
        std::ofstream source(sourceFile);
        // check whether the source file was created successfully
        if (!source)
        {
            database.updateSubmissionStatus(submissionId, "COMPILATION_ERROR");
            database.updateJobStatus(submissionId, "FAILED");
            database.updateUserProgress(submissionId, "COMPILATION_ERROR");
            return;
        }

        // write the user's submitted code into the file
        source << code;
    }


    // compile the submitted source code
    bool compiled = Compiler::compile(sourceFile, executableFile, compileErrorFile);


    // if compilation fails, mark the submission as a compilation error
    if (!compiled) {
        database.updateSubmissionStatus(submissionId, "COMPILATION_ERROR");
        database.updateUserProgress(submissionId, "COMPILATION_ERROR");
        database.updateJobStatus(submissionId, "FAILED");
    
        std::cout << "Submission " << submissionId << ": Compilation Error"  << std::endl;
        return;
    }


    // assume that all test cases will pass initially
    bool allAccepted = true;

    // default final status
    std::string finalStatus = "ACCEPTED";

    // execute the submitted program against every test case
    for (const TestCase& testCase : testCases)
    {
        // create the input file for the current test case
        {
            std::ofstream input(inputFile);

            // check whether the input file was created successfully
            if (!input)
            {
                database.updateSubmissionStatus(submissionId, "RUNTIME_ERROR");
                database.updateJobStatus(submissionId, "FAILED");
                database.updateUserProgress(submissionId, "RUNTIME_ERROR");
                return;
            }

            // write the test case input into the input file
            input << testCase.input;
        }


        // record the start time before execution
        auto startTime = std::chrono::steady_clock::now();

        // variable used to store memory consumed by the process
        long long memoryUsed = 0;

        // execute the compiled program
        int exitCode = ProcessManager::runProcess(executableFile, inputFile, outputFile, workingDirectory, memoryUsed);

        // record the end time after execution
        auto endTime = std::chrono::steady_clock::now();


        // calculate execution time in seconds
        double timeTaken =std::chrono::duration<double>(endTime - startTime).count();

        // open the output generated by the submitted program
        std::ifstream output(outputFile);


        // read the complete output into a string
        std::string actualOutput{std::istreambuf_iterator<char>(output), std::istreambuf_iterator<char>()};


        // remove trailing whitespace from actual output
        std::string normalizedActual = normalizeOutput(actualOutput);

        // remove trailing whitespace from expected output
        std::string normalizedExpected = normalizeOutput(testCase.expectedOutput);

        // variable storing the result of the current test case
        std::string resultStatus;


        // -2 indicates that the process exceeded the allowed execution time
        if (exitCode == -2)
        {
            resultStatus = "TIME_LIMIT_EXCEEDED";
            finalStatus = "TIME_LIMIT_EXCEEDED";
            allAccepted = false;
        }

        // any other non-zero exit code means that the program terminated with an error
        else if (exitCode != 0)
        {
            resultStatus = "RUNTIME_ERROR";
            finalStatus = "RUNTIME_ERROR";
            allAccepted = false;
        }

        // compare actual output with expected output
        else if (normalizedActual != normalizedExpected)
        {
            resultStatus = "WRONG_ANSWER";
            finalStatus = "WRONG_ANSWER";
            allAccepted = false;
        }

        // program executed successfully and produced the expected output
        else
        {
            resultStatus = "ACCEPTED";
        }


        // save the result of the current test case in the execution_results table
        database.saveExecutionResult(submissionId,testCase.testCaseId, resultStatus, actualOutput, timeTaken,memoryUsed);


        // display the result of the current test case
        std::cout << "Submission " << submissionId << " | Test Case " << testCase.testCaseId
         << " | " << resultStatus << " | Time: " << timeTaken << " seconds" << std::endl;


        // stop processing further test cases once one test case fails
        if (!allAccepted)
        {
            break;
        }
    }

    // if every test case was accepted, mark the complete submission as ACCEPTED
    if (allAccepted)
    {
        database.updateSubmissionStatus(submissionId, "ACCEPTED");
        database.updateUserProgress(submissionId, "ACCEPTED");
        database.updateJobStatus(submissionId, "COMPLETED");

        std::cout << "Submission " << submissionId << ": ACCEPTED" << std::endl;
    }

    // otherwise store the final failure status
    else
    {
        database.updateSubmissionStatus(submissionId, finalStatus);
        database.updateUserProgress(submissionId, finalStatus);
        database.updateJobStatus(submissionId, "COMPLETED");
        std::cout << "Submission " << submissionId << ": " << finalStatus << std::endl;
    }
}
