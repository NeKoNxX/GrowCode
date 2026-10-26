#include "ExecutionWorker.h"

#include "../compiler/Compiler.h"
#include "../process_manager/ProcessManager.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <vector>
#include <string>
#include <iterator>

namespace fs = std::filesystem;


// Removes trailing whitespace, tabs, and newlines from program output.
static std::string normalizeOutput(const std::string& output)
{
    // Create a copy of the original output.
    std::string normalized = output;

    // Remove trailing whitespace characters.
    while (!normalized.empty() &&
           (normalized.back() == ' ' ||
            normalized.back() == '\t' ||
            normalized.back() == '\n' ||
            normalized.back() == '\r'))
    {
        normalized.pop_back();
    }

    // Return cleaned output.
    return normalized;
}


// Reads the complete contents of a file.
static std::string readFileContents(const std::string& filePath)
{
    std::ifstream file(filePath);

    if (!file.is_open()) {
        return "";
    }

    return std::string(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );
}


// Constructor to initialize ExecutionWorker with the database object.
ExecutionWorker::ExecutionWorker(Database& database) : database(database)
{
}


// Processes a submitted code evaluation job.
void ExecutionWorker::processJob(int submissionId)
{
    // Display which submission is currently being processed.
    std::cout << "Worker processing Submission "
              << submissionId << std::endl;

    // Clear any diagnostic left by an earlier execution attempt.
    database.updateSubmissionError(submissionId, "");

    // Update submission status to RUNNING.
    database.updateSubmissionStatus(submissionId, "RUNNING");

    // Update the corresponding job status to RUNNING.
    database.updateJobStatus(submissionId, "RUNNING");

    // Get the problem ID associated with the submission.
    int problemId = database.getSubmissionProblemId(submissionId);

    // Retrieve the source code submitted by the user.
    std::string code = database.getSubmissionCode(submissionId);

    // Retrieve the selected programming language.
    int languageId = database.getSubmissionLanguageId(submissionId);

    // Language IDs: 1 = C++, 2 = Java, 3 = Python 3.
    if (languageId < 1 || languageId > 3)
    {
        database.updateSubmissionStatus(
            submissionId, "COMPILATION_ERROR"
        );
        database.updateUserProgress(
            submissionId, "COMPILATION_ERROR"
        );
        database.updateJobStatus(submissionId, "FAILED");

        std::cout << "Submission " << submissionId
                  << ": Unsupported language ID "
                  << languageId << std::endl;
        return;
    }

    // If no source code was found, the submission cannot be executed.
    if (code.empty())
    {
        database.updateSubmissionStatus(
            submissionId, "COMPILATION_ERROR"
        );
        database.updateUserProgress(
            submissionId, "COMPILATION_ERROR"
        );
        database.updateJobStatus(submissionId, "FAILED");
        return;
    }

    // Retrieve all test cases associated with the problem.
    std::vector<TestCase> testCases = database.getTestCases(problemId);

    // If no test cases exist, the submission cannot be evaluated.
    if (testCases.empty())
    {
        database.updateSubmissionStatus(
            submissionId, "RUNTIME_ERROR"
        );
        database.updateJobStatus(submissionId, "FAILED");
        database.updateUserProgress(
            submissionId, "RUNTIME_ERROR"
        );

        std::cout << "Submission " << submissionId
                  << ": No test cases found" << std::endl;
        return;
    }

    // Create directories used for source code, executables,
    // outputs, logs, and submission-specific execution.
    fs::create_directories("storage/user_code");
    fs::create_directories("storage/executables");
    fs::create_directories("storage/outputs");
    fs::create_directories("storage/logs");
    fs::create_directories("storage/executions");

    // Create a unique name for this submission.
    std::string base = "submission_" + std::to_string(submissionId);

    // Create a submission-specific working directory.
    fs::path workingDirectoryPath =
        fs::absolute("storage/executions/" + base);

    fs::create_directories(workingDirectoryPath);

    // Choose the source filename based on the language.
    fs::path sourcePath;

    if (languageId == 1)
    {
        // C++ source file.
        sourcePath = fs::absolute(
            "storage/user_code/" + base + ".cpp"
        );
    }
    else if (languageId == 2)
    {
        // Java public class Main must be saved as Main.java.
        sourcePath = workingDirectoryPath / "Main.java";
    }
    else
    {
        // Python 3 source file.
        sourcePath = fs::absolute(
            "storage/user_code/" + base + ".py"
        );
    }

    // Create absolute paths for execution-related files.
    fs::path executablePath =
        fs::absolute("storage/executables/" + base);

    fs::path compileErrorPath =
        fs::absolute("storage/logs/" + base + "_compile.txt");

    fs::path runtimeErrorPath =
        fs::absolute("storage/logs/" + base + "_runtime.txt");

    fs::path inputPath =
        fs::absolute("storage/user_code/" + base + "_input.txt");

    fs::path outputPath =
        fs::absolute("storage/outputs/" + base + ".txt");

    // Convert filesystem paths into strings.
    std::string sourceFile = sourcePath.string();
    std::string executableFile = executablePath.string();
    std::string compileErrorFile = compileErrorPath.string();
    std::string runtimeErrorFile = runtimeErrorPath.string();
    std::string inputFile = inputPath.string();
    std::string outputFile = outputPath.string();
    std::string workingDirectory = workingDirectoryPath.string();

    // Create the source-code file and write the user's code.
    {
        std::ofstream source(sourceFile);

        if (!source)
        {
            database.updateSubmissionStatus(
                submissionId, "COMPILATION_ERROR"
            );
            database.updateJobStatus(submissionId, "FAILED");
            database.updateUserProgress(
                submissionId, "COMPILATION_ERROR"
            );

            std::cout << "Submission " << submissionId
                      << ": Could not create source file"
                      << std::endl;
            return;
        }

        source << code;

        if (!source)
        {
            database.updateSubmissionStatus(
                submissionId, "COMPILATION_ERROR"
            );
            database.updateJobStatus(submissionId, "FAILED");
            database.updateUserProgress(
                submissionId, "COMPILATION_ERROR"
            );
            return;
        }
    }

    // Compile C++ or Java. Python runs through its interpreter.
    bool compiled = true;

    if (languageId == 1)
    {
        // Compile C++17.
        compiled = Compiler::compile(
            sourceFile,
            executableFile,
            compileErrorFile
        );
    }
    else if (languageId == 2)
    {
        // Compile Java classes into the submission's workspace.
        compiled = Compiler::compileJava(
            sourceFile,
            workingDirectory,
            compileErrorFile
        );
    }

    // If compilation fails, capture and save the compiler diagnostic.
    if (!compiled)
    {
        // Read the diagnostic written by the compiler.
        std::string errorMessage = readFileContents(compileErrorFile);

        // Save the compiler diagnostic for the submission API/frontend.
        database.updateSubmissionError(submissionId, errorMessage);

        database.updateSubmissionStatus(
            submissionId, "COMPILATION_ERROR"
        );
        database.updateUserProgress(
            submissionId, "COMPILATION_ERROR"
        );
        database.updateJobStatus(submissionId, "FAILED");

        std::cout << "Submission " << submissionId
                  << ": Compilation Error" << std::endl;
        return;
    }

    // Assume that all test cases will pass initially.
    bool allAccepted = true;

    // Default final status.
    std::string finalStatus = "ACCEPTED";

    // Execute the submitted program against every test case.
    for (const TestCase& testCase : testCases)
    {
        // Create the input file for the current test case.
        {
            std::ofstream input(inputFile);

            if (!input)
            {
                database.updateSubmissionStatus(
                    submissionId, "RUNTIME_ERROR"
                );
                database.updateJobStatus(submissionId, "FAILED");
                database.updateUserProgress(
                    submissionId, "RUNTIME_ERROR"
                );
                return;
            }

            // Write the test case input into the input file.
            input << testCase.input;

            if (!input)
            {
                database.updateSubmissionStatus(
                    submissionId, "RUNTIME_ERROR"
                );
                database.updateJobStatus(submissionId, "FAILED");
                database.updateUserProgress(
                    submissionId, "RUNTIME_ERROR"
                );
                return;
            }
        }

        // Record the start time before execution.
        auto startTime = std::chrono::steady_clock::now();

        // Variable used to store memory consumed by the process.
        long long memoryUsed = 0;

        // Build the command for the selected language.
        std::vector<std::string> command;

        if (languageId == 1)
        {
            // C++: execute the compiled binary.
            command = {
                executableFile
            };
        }
        else if (languageId == 2)
        {
            // Java: run the compiled Main class.
            command = {
                "java",
                "-cp",
                workingDirectory,
                "Main"
            };
        }
        else
        {
            // Python 3: execute the submitted script.
            command = {
                "python3",
                sourceFile
            };
        }

        // Execute the program and collect its exit code and memory usage.
        int exitCode = ProcessManager::runProcess(
            command,
            inputFile,
            outputFile,
            workingDirectory,
            memoryUsed,
            runtimeErrorFile
        );

        // Read diagnostics written to standard error by the submitted program.
        std::string runtimeErrorMessage = readFileContents(runtimeErrorFile);

        // Save runtime diagnostics for a genuine runtime failure.
        // A time limit exceeded result is handled separately below.
        if (exitCode != 0 && exitCode != -2) {
            if (runtimeErrorMessage.empty()) {
                runtimeErrorMessage =
                    "Program terminated with exit code " +
                    std::to_string(exitCode) + ".";
            }

            database.updateSubmissionError(
                submissionId,
                runtimeErrorMessage
            );
        }

        // Record the end time after execution.
        auto endTime = std::chrono::steady_clock::now();

        // Calculate execution time in seconds.
        double timeTaken =
            std::chrono::duration<double>(
                endTime - startTime
            ).count();

        // Read the output generated by the submitted program.
        std::ifstream output(outputFile);

        std::string actualOutput;

        if (output)
        {
            actualOutput = std::string(
                std::istreambuf_iterator<char>(output),
                std::istreambuf_iterator<char>()
            );
        }

        // Normalize actual and expected output.
        std::string normalizedActual = normalizeOutput(actualOutput);
        std::string normalizedExpected =
            normalizeOutput(testCase.expectedOutput);

        // Store the result of the current test case.
        std::string resultStatus;

        // -2 indicates that the process exceeded the allowed time.
        if (exitCode == -2)
        {
            database.updateSubmissionError(
                submissionId,
                "Time limit exceeded."
            );
            resultStatus = "TIME_LIMIT_EXCEEDED";
            finalStatus = "TIME_LIMIT_EXCEEDED";
            allAccepted = false;
        }
        // Any other non-zero exit code indicates an execution failure.
        else if (exitCode != 0)
        {
            resultStatus = "RUNTIME_ERROR";
            finalStatus = "RUNTIME_ERROR";
            allAccepted = false;
        }
        // Compare actual output with expected output.
        else if (normalizedActual != normalizedExpected)
        {
            resultStatus = "WRONG_ANSWER";
            finalStatus = "WRONG_ANSWER";
            allAccepted = false;
        }
        // The program executed successfully and produced expected output.
        else
        {
            resultStatus = "ACCEPTED";
        }

        // Save the result of the current test case.
        database.saveExecutionResult(
            submissionId,
            testCase.testCaseId,
            resultStatus,
            actualOutput,
            timeTaken,
            memoryUsed
        );

        // Display the result of the current test case.
        std::cout << "Submission " << submissionId
                  << " | Test Case " << testCase.testCaseId
                  << " | " << resultStatus
                  << " | Time: " << timeTaken
                  << " seconds" << std::endl;

        // Stop processing further test cases once one test case fails.
        if (!allAccepted)
        {
            break;
        }
    }

    // If every test case was accepted, update the final submission status.
    if (allAccepted)
    {
        database.updateSubmissionStatus(submissionId, "ACCEPTED");
        database.updateUserProgress(submissionId, "ACCEPTED");
        database.updateJobStatus(submissionId, "COMPLETED");

        std::cout << "Submission " << submissionId
                  << ": ACCEPTED" << std::endl;
    }
    else
    {
        // Store the final failure status.
        database.updateSubmissionStatus(submissionId, finalStatus);
        database.updateUserProgress(submissionId, finalStatus);
        database.updateJobStatus(submissionId, "COMPLETED");

        std::cout << "Submission " << submissionId
                  << ": " << finalStatus << std::endl;
    }
}
