#ifndef DATABASE_H
#define DATABASE_H

#include "problems/Problem.h"
#include "problems/TestCase.h"
#include <string> 
#include<vector>
#include<optional>
#include <nlohmann/json.hpp>
//database class is used to store database connection details
class Database {
private:
    std::string host; //stores database server address
    int port; //stores port number of database
    std::string username; //stores database username
    std::string password; //stores database password
    std::string database; //stores database name

//this constructor is used to initialize all database details
public:
    Database(
        const std::string& host,// Database server address
        int port,// Database port number
        const std::string& username,// Database username
        const std::string& password, // Database password
        const std::string& database  // Database name
    );
    // Checks whether the database connection is successful and returns true if connected, otherwise false
    
    bool isConnected();

    // functions to retrieve problems and test cases from the database
    std::vector<Problem> getProblems();
    Problem getProblem(int problem_id);
    std::vector<TestCase> getTestCases(int problemId);
    
    // func to create a new submission by a user
    int createSubmission(int userId, int problemId, int languageId, const std::string& code);

    //function for job creation i.e. submit
    bool createJob(int submissionId);

    // function to create a submission and its job together
    int createSubmissionWithJob(int userId, int problemId, int languageId, const std::string& code);

    // func to retrieve submitted query
    std::vector<int> getQueuedSubmissionIds();

    std::string getSubmissionCode(int submissionId);

    int getSubmissionProblemId(int submissionId);

    int getSubmissionLanguageId(int submissionId);
    // func for updation and save
    bool updateSubmissionStatus(int submissionId, const std::string& status);
    bool updateSubmissionError(int submissionId, const std::string& errorMessage);
    bool updateJobStatus(int submissionId, const std::string& status);
    bool saveExecutionResult(int submissionId, int testCaseId, const std::string& status, const std::string& actualOutput, double timeTaken, long long memoryUsed);

    // function to get the status of a submission for a specific user
    std::string getSubmissionStatus(int submissionId, int userId);
    bool getExpectedComplexity(int problemId, std::string& timeComplexity, std::string& spaceComplexity);
    bool saveProblemComplexity(int problemId, const std::string& timeComplexity,
                               const std::string& spaceComplexity,
                               const std::string& recommendedAlgorithm);
    std::string getRecommendedAlgorithm(int problemId);
    bool deleteProblem(int problemId);
    bool deleteTestCase(int testCaseId);
    nlohmann::json getSubmissionTestResults(int submissionId, int userId);
    bool getSubmissionTestCounts(int submissionId, int userId, int& passed, int& total);
    std::string getSubmissionError(int submissionId, int userId);
    
    // function to get all submissions made by a specific user
    std::string getUserSubmissions(int userId);

    // function to get progress of a specific user
    std::string getUserProgress(int userId);

    // function to update the progress of a user
    // based on the result of a submission
    bool updateUserProgress(int submissionId, const std::string& status);

    // function to create a new user
    bool createUser(const std::string& name, const std::string& username, const std::string& email, const std::string& passwordHash);

    // function to retrieve the role of a user
    std::string getUserRole(int userId);

        // function to create a new problem
        int createProblem(
            const std::string& title,
            const std::string& description,
            const std::string& difficulty,
            const std::string& constraints,
            int createdBy
        );
    
        // function to create a new test case for a problem
        int createTestCase(
            int problemId,
            const std::string& input,
            const std::string& expectedOutput,
            bool isSample,
            int orderNo
        );
    
        // function to find a user using their username
        bool findUser(
            const std::string& username,
            std::string& storedHash,
            int& userId,
            std::string& role
        );

        // Returns a JSON array of users ranked by solved problems, then attempts.
        std::string getLeaderboard();

        std::optional<std::string> getSubmissionCode(
            int userId,
            int submissionId
        );
};

#endif
