#include"database/Database.h"  //Includes the Database class declaration
#include<iostream> //Used for printing messages
#include<mysqlx/xdevapi.h> //MySQL library used to connect to the database
#include <nlohmann/json.hpp>

//Constructor of the Database class which receives all the information needed to connect to MySQL
Database::Database(
    const std::string& host, //MySQL server address 
    int port,  //MySQL port number
    const std::string& username, //MySQL username
    const std::string& password, //MySQL password
    const std::string& database //Name of the database
)
   //Initialize the class variables with the given values
    : host(host), port(port), username(username), password(password), database(database)
{
    //try block contains the code that may cause a connection error
    try {
        //Create a MySQL session using the given login details
        mysqlx::Session session(
            host, //Server address
            port,  //Port number
            username, password 
        );
   //Select the database that we want to use
        session.sql("USE " + database).execute();
   //Display a success message if the connection works
        std::cout << "Connected to MySQL database: " << database << std::endl;
    }
    //catch block handles MySQL connection errors
    catch (const mysqlx::Error& e) {
        // Display the error message
        std::cerr << "Database connection failed: " << e.what() << std::endl;
//Send the error back to the program
        throw;
    }
}

//Checks whether the database connection is successful
bool Database::isConnected()
{
    try {
        //Create a MySQL session using the given login details
        mysqlx::Session session(host, port, username, password);

        //Select the database that we want to use
        session.sql("USE " + database).execute();

        //Run a simple query to check whether the connection works
        session.sql("SELECT 1").execute();

        return true;
    }
    catch (...)
    {
        return false;
    }
}

// Retreives all the problems from the Database
std::vector<Problem> Database::getProblems() {
    std::vector<Problem> problems; // vector to be returned

    try {
        mysqlx::Session session( host, port, username, password);
        session.sql("USE " + database).execute();

        // executing db query to get all details of a problem
        auto result = session.sql(
            "SELECT problem_id, title, description, difficulty, constraints "
            "FROM problems "
            "ORDER BY problem_id"
        ).execute();

        // pushing retrieved values to a tuple 
        for (auto row : result) {
            Problem problem;
            problem.problem_id = row[0].get<int>();
            problem.title = row[1].get<std::string>();
            problem.description = row[2].get<std::string>();
            problem.difficulty = row[3].get<std::string>();
            problem.constraints = row[4].get<std::string>();
            problems.push_back(problem);
        }
    }
    catch (const mysqlx::Error& e) {
        std::cerr << "Failed to get problems: " << e.what() << std::endl;
    }
    return problems;
}

// Retrieves problem by its problem ID
Problem Database::getProblem(int problem_id) {
    mysqlx::Session session( host, port, username, password);
    session.sql("USE " + database).execute();

    // db query for retrieving problem by its problem id
    auto result = session.sql(
        "SELECT problem_id, title, description, difficulty, constraints "
        "FROM problems "
        "WHERE problem_id = ?"
    ).bind(problem_id).execute();

    auto row = result.fetchOne();

    if (row.isNull()) {
        throw std::runtime_error("Problem not found");
    }

    Problem problem;
    //store the values to a tuple if found
    problem.problem_id = row[0].get<int>();
    problem.title = row[1].get<std::string>();
    problem.description = row[2].get<std::string>();
    problem.difficulty = row[3].get<std::string>();
    problem.constraints = row[4].get<std::string>();

    return problem;
}

std::vector<TestCase> Database::getTestCases(int problemId) {
    std::vector<TestCase> testCases;
    try {
        mysqlx::Session session(host, port, username, password);
        session.sql("USE " + database).execute();
        // db query for retrieving all test cases for a problem id
        auto result = session.sql(
            "SELECT test_case_id, input, expected_output, is_sample "
            "FROM test_cases "
            "WHERE problem_id = ? "
            "ORDER BY order_no"
        ).bind(problemId).execute();

        // storing retrieved info to a tuple
        for (auto row : result.fetchAll()) {
            TestCase testCase;

            testCase.testCaseId = row[0].get<int>();

            if (row[1].isNull()) {
                testCase.input = "";
            } else {
                testCase.input = row[1].get<std::string>();
            }

            testCase.expectedOutput = row[2].get<std::string>();
            testCase.isSample = row[3].get<bool>();
            testCases.push_back(testCase);
        }
    }
    catch (const mysqlx::Error& e) {
        std::cerr << "Get test cases error: " << e.what() << std::endl;
    }
    return testCases;
}


// // function to create submission for user 
// int Database::createSubmission(int userId, int problemId, int languageId, const std::string& code) {
//     try {
//         mysqlx::Session session( host, port, username, password);
//         session.sql("USE " + database).execute();

//         // query to save the submitted code of the user in the database
//         auto result = session.sql(
//             "INSERT INTO submissions "
//             "(user_id, problem_id, language_id, code, status) "
//             "VALUES (?, ?, ?, ?, 'PENDING')"
//         )
//         .bind(userId)
//         .bind(problemId)
//         .bind(languageId)
//         .bind(code)
//         .execute(); // binding received values respectively

//         return static_cast<int>(result.getAutoIncrementValue()); // returns the auto generated submission id
//     }
//     catch (const mysqlx::Error& e) {
//         std::cerr << "Submission creation failed: " << e.what() << std::endl;
//         return -1;
//     }
// }

// // function to create a job for a submission
// bool Database::createJob(int submissionId) {
//     try {
//         // create a session to connect with the database
//         mysqlx::Session session(
//             host,
//             port,
//             username,
//             password
//         );

//         // select the database to work with
//         session.sql("USE " + database).execute();

//         // query to create a job for the submitted code
//         // the job is initially placed in QUEUED status
//         // priority is set to 0 by default
//         session.sql(
//             "INSERT INTO jobs "
//             "(submission_id, status, priority) "
//             "VALUES (?, 'QUEUED', 0)"
//         )
//         .bind(submissionId) // bind the submission ID to the query
//         .execute();

//         // return true if the job was created successfully
//         return true;
//     }
//     catch (const mysqlx::Error& e) {
//         // display the error if job creation fails
//         std::cerr
//             << "Job creation failed: "
//             << e.what()
//             << std::endl;

//         // return false when job creation fails
//         return false;
//     }
// }

// combined above two functions for transaction handling
// function to create a submission and its job together
int Database::createSubmissionWithJob(int userId, int problemId, int languageId, const std::string& code) {
    try {
        // create a session to connect with the database
        mysqlx::Session session(host, port, username, password);

        // select the database to work with
        session.sql("USE " + database).execute();

        // start a transaction so submission and job are treated as one operation
        session.sql("START TRANSACTION").execute();

        try {
            // query to insert the submitted code into the submissions table
            auto submissionResult = session.sql(
                "INSERT INTO submissions "
                "(user_id, problem_id, language_id, code, status) "
                "VALUES (?, ?, ?, ?, 'PENDING')"
            )
            .bind(userId, problemId, languageId, code).execute();

            // get the auto-generated submission ID
            int submissionId = static_cast<int>(submissionResult.getAutoIncrementValue());

            // query to create a job for the newly created submission
            // the job is initially placed in QUEUED status
            // priority is set to 0
            session.sql(
                "INSERT INTO jobs "
                "(submission_id, status, priority) "
                "VALUES (?, 'QUEUED', 0)"
            )
            .bind(submissionId) // bind the generated submission ID
            .execute();

            // save both operations permanently
            session.sql("COMMIT").execute();

            // return the generated submission ID
            return submissionId;
        }
        catch (...) {
            // undo all database changes if any operation fails
            session.sql("ROLLBACK").execute();  // TRANSACTION handling

            // pass the error to the outer catch block
            throw;
        }
    }
    catch (const mysqlx::Error& e) {
        // display the error if submission or job creation fails
        std::cerr<< "Submission and job creation failed: " << e.what() << std::endl;

        // return -1 to indicate failure
        return -1;
    }
}

// function to get all submission IDs whose jobs are currently queued
std::vector<int> Database::getQueuedSubmissionIds() {
    // vector to store the submission IDs
    std::vector<int> submissionIds;

    try {
        // create a session to connect with the database
        mysqlx::Session session(host, port, username, password);

        // select the database to work with
        session.sql("USE " + database).execute();

        // query to get submission IDs of all queued jobs
        // jobs are ordered according to their creation time
        auto result = session.sql(
            "SELECT submission_id "
            "FROM jobs "
            "WHERE status = 'QUEUED' "
            "ORDER BY created_at, job_id"
        ).execute();

        // store each retrieved submission ID in the vector
        for (auto row : result.fetchAll()) {
            submissionIds.push_back(row[0].get<int>());
        }
    }
    catch (const mysqlx::Error& e) {
        // display the error if queued submissions cannot be retrieved
        std::cerr
            << "Get queued submissions error: " << e.what() << std::endl;
    }

    // return all queued submission IDs
    return submissionIds;
}

// function to get the submitted code using submission ID
std::string Database::getSubmissionCode(int submissionId) {
    try {
       // create a session to connect with the database
       mysqlx::Session session(host, port, username, password);

       // select the database to work with
       session.sql("USE " + database).execute();

        // query to retrieve the submitted code
        auto result = session.sql(
            "SELECT code "
            "FROM submissions "
            "WHERE submission_id = ?"
        )
        .bind(submissionId) // bind the submission ID to the query
        .execute();

        // fetch the retrieved row
        auto row = result.fetchOne();

        // return empty string if submission does not exist
        if (row.isNull()) {
            return "";
        }

        // return the submitted source code
        return row[0].get<std::string>();
    }
    catch (const mysqlx::Error& e) {
        // display the error if submission code cannot be retrieved
        std::cerr << "Failed to get submission code: " << e.what() << std::endl;

        // return empty string when an error occurs
        return "";
    }
}

// function to get the problem ID associated with a submission
int Database::getSubmissionProblemId(int submissionId) {
    try {
        // create a session to connect with the database
       mysqlx::Session session(host, port, username, password);

       // select the database to work with
       session.sql("USE " + database).execute();

        // query to retrieve the problem ID of the submission
        auto result = session.sql(
            "SELECT problem_id "
            "FROM submissions "
            "WHERE submission_id = ?"
        )
        .bind(submissionId) // bind the submission ID to the query
        .execute();

        // fetch the retrieved row
        auto row = result.fetchOne();

        // return -1 if submission does not exist
        if (row.isNull()) {
            return -1;
        }

        // return the problem ID associated with the submission
        return row[0].get<int>();
    }
    catch (const mysqlx::Error& e) {
        // display the error if problem ID cannot be retrieved
        std::cerr << "Failed to get submission problem: " << e.what() << std::endl;

        // return -1 when an error occurs
        return -1;
    }
}

// function to get the language ID associated with a submission
int Database::getSubmissionLanguageId(int submissionId) {
    try {
        // create a session to connect with the database
       mysqlx::Session session(host, port, username, password);

       // select the database to work with
       session.sql("USE " + database).execute();

        // query to retrieve the language ID of the submission
        auto result = session.sql(
            "SELECT language_id "
            "FROM submissions "
            "WHERE submission_id = ?"
        )
        .bind(submissionId) // bind the submission ID to the query
        .execute();

        // fetch the retrieved row
        auto row = result.fetchOne();

        // return -1 if submission does not exist
        if (row.isNull()) {
            return -1;
        }

        // return the language ID associated with the submission
        return row[0].get<int>();
    }
    catch (const mysqlx::Error& e) {
        // display the error if language ID cannot be retrieved
        std::cerr << "Failed to get submission language: " << e.what() << std::endl;

        // return -1 when an error occurs
        return -1;
    }
}

// function to update the status of a submission
bool Database::updateSubmissionStatus(int submissionId, const std::string& status) {
    try {
        // create a session to connect with the database
        mysqlx::Session session(host, port, username, password);

       // select the database to work with
       session.sql("USE " + database).execute();

        // query to update the submission status
        session.sql(
            "UPDATE submissions "
            "SET status = ? "
            "WHERE submission_id = ?"
        )
        .bind(status)          // bind the new status
        .bind(submissionId)    // bind the submission ID
        .execute();

        // return true if the status was updated successfully
        return true;
    }
    catch (const mysqlx::Error& e) {
        // display the error if status update fails
        std::cerr<< "Failed to update submission status: " << e.what() << std::endl;
            
        // return false when an error occurs
        return false;
    }
}
// function to update the status of a job
bool Database::updateJobStatus(int submissionId, const std::string& status) {
    try {
        // create a session to connect with the database
        mysqlx::Session session(host, port, username, password);

        // select the database to work with
        session.sql("USE " + database).execute();

        // if job is running, record the time when worker picked it
        if (status == "RUNNING") {

            session.sql(
                "UPDATE jobs "
                "SET status = ?, "
                "picked_at = CURRENT_TIMESTAMP "
                "WHERE submission_id = ?"
            )
            .bind(status, submissionId)
            .execute();

        }
        // if job is completed or failed, record the finishing time
        else if (
            status == "COMPLETED" ||
            status == "FAILED"
        ) {

            session.sql(
                "UPDATE jobs "
                "SET status = ?, "
                "finished_at = CURRENT_TIMESTAMP "
                "WHERE submission_id = ?"
            )
            .bind(status, submissionId)
            .execute();

        }
        // for any other status, only update the job status
        else {

            session.sql(
                "UPDATE jobs "
                "SET status = ? "
                "WHERE submission_id = ?"
            )
            .bind(status, submissionId)
            .execute();
        }

        // return true if the job status was updated successfully
        return true;
    }
    catch (const mysqlx::Error& e) {
        // display the error if job status update fails
        std::cerr << "Update job status error: " << e.what() << std::endl;

        // return false when an error occurs
        return false;
    }
}
// function to save the execution result of a test case
bool Database::saveExecutionResult(int submissionId, int testCaseId, const std::string& status, const std::string& actualOutput, double timeTaken, long long memoryUsed) {
    try {
        // create a session to connect with the database
        mysqlx::Session session(host, port, username, password);

        // select the database to work with
        session.sql("USE " + database).execute();

        // query to save the execution result
        session.sql(
            "INSERT INTO execution_results "
            "(submission_id, test_case_id, status, actual_output, "
            "time_taken, memory_used) "
            "VALUES (?, ?, ?, ?, ?, ?)"
        )
        .bind(submissionId, testCaseId, status, actualOutput, timeTaken, memoryUsed)
        .execute();

        // return true if the execution result was saved successfully
        return true;
    }
    catch (const mysqlx::Error& e) {
        // display the error if execution result cannot be saved
        std::cerr << "Save execution result error: " << e.what() << std::endl;

        // return false when an error occurs
        return false;
    }
}


// function to get the status of a submission for a specific user
std::string Database::getSubmissionStatus(int submissionId, int userId) {
    try {
       // create a session to connect with the database
       mysqlx::Session session(host, port, username, password);

       // select the database to work with
       session.sql("USE " + database).execute();

        // query to retrieve the submission status
        // submission must belong to the specified user
        auto result = session.sql(
            "SELECT status "
            "FROM submissions "
            "WHERE submission_id = ? "
            "AND user_id = ?"
        )
        .bind(submissionId) // bind the submission ID
        .bind(userId)       // bind the user ID
        .execute();

        // fetch the retrieved row
        auto row = result.fetchOne();

        // return empty string if submission does not exist
        // or does not belong to this user
        if (row.isNull()) {
            return "";
        }

        // return the current submission status
        return row[0].get<std::string>();
    }
    catch (const std::exception& e) {
        // display the error if status cannot be retrieved
        std::cerr
            << "Failed to get submission status: " << e.what() << std::endl;

        // return empty string when an error occurs
        return "";
    }
}

// function to get all submissions made by a specific user
std::string Database::getUserSubmissions(int userId) {
    try {
        // create a session to connect with the database
        mysqlx::Session session(host, port, username, password);

        // select the database to work with
        session.sql("USE " + database).execute();

        // query to retrieve all submissions made by the user
        // problem and language details are retrieved using JOIN
        auto result = session.sql(
            "SELECT "
            "s.submission_id, "
            "p.title, "
            "l.name, "
            "s.status, "
            "DATE_FORMAT(s.submitted_at, '%Y-%m-%d %H:%i:%s') "
            "FROM submissions s "
            "JOIN problems p "
            "ON s.problem_id = p.problem_id "
            "JOIN languages l "
            "ON s.language_id = l.language_id "
            "WHERE s.user_id = ? "
            "ORDER BY s.submitted_at DESC"
        )
        .bind(userId) // bind the user ID to the query
        .execute();

        // create an empty JSON array to store submissions
        nlohmann::json submissions = nlohmann::json::array();

        mysqlx::Row row;

        // retrieve each submission row
        while ((row = result.fetchOne())) {
            // add submission details to the JSON array
            submissions.push_back({
                {"submission_id", row[0].get<int>()},
                {"problem_title", row[1].get<std::string>()},
                {"language", row[2].get<std::string>()},
                {"status", row[3].get<std::string>()},
                {"submitted_at", row[4].get<std::string>()}
            });
        }

        // convert the JSON array into a string
        return submissions.dump();
    }
    catch (const mysqlx::Error& e) {
        // display the error if submissions cannot be retrieved
        std::cerr << "Failed to get user submissions: " << e.what() << std::endl;

        // return an empty JSON array when an error occurs
        return "[]";
    }
}

// function to get overall progress of a specific user
std::string Database::getUserProgress(int userId) {
    try {

        // create a session to connect with the database
        mysqlx::Session session(host, port, username, password);

        // select the database to work with
        session.sql("USE " + database).execute();

        // query to calculate the overall progress of the user
        // COUNT(DISTINCT ...) counts the number of unique problems solved
        // COUNT(s.submission_id) counts the total number of submissions
        // SUM(CASE ...) counts submissions based on their status
        auto result = session.sql(
            "SELECT "
            "CAST(COUNT(DISTINCT CASE "
            "WHEN s.status = 'ACCEPTED' "
            "THEN s.problem_id END) AS UNSIGNED), "

            "CAST(COUNT(s.submission_id) AS UNSIGNED), "

            "CAST(COALESCE(SUM(CASE "
            "WHEN s.status = 'ACCEPTED' "
            "THEN 1 ELSE 0 END), 0) AS UNSIGNED), "

            "CAST(COALESCE(SUM(CASE "
            "WHEN s.status = 'WRONG_ANSWER' "
            "THEN 1 ELSE 0 END), 0) AS UNSIGNED), "

            "CAST(COALESCE(SUM(CASE "
            "WHEN s.status = 'COMPILATION_ERROR' "
            "THEN 1 ELSE 0 END), 0) AS UNSIGNED), "

            "CAST(COALESCE(SUM(CASE "
            "WHEN s.status = 'RUNTIME_ERROR' "
            "THEN 1 ELSE 0 END), 0) AS UNSIGNED), "

            "CAST(COALESCE(SUM(CASE "
            "WHEN s.status = 'TIME_LIMIT_EXCEEDED' "
            "THEN 1 ELSE 0 END), 0) AS UNSIGNED) "

            // get submission data only for the specified user
            "FROM submissions s "
            "WHERE s.user_id = ?"
        )
        .bind(userId) // bind the user id to the query
        .execute();

        // fetch the single row containing the calculated progress
        auto row = result.fetchOne();

        // if no row is returned, return an empty JSON object
        if (row.isNull()) {
            return "{}";
        }

        // create a JSON object containing the user's progress
        nlohmann::json progress = {
            {"problems_solved", row[0].get<uint64_t>()},
            {"total_attempts", row[1].get<uint64_t>()},
            {"accepted", row[2].get<uint64_t>()},
            {"wrong_answer", row[3].get<uint64_t>()},
            {"compilation_error", row[4].get<uint64_t>()},
            {"runtime_error", row[5].get<uint64_t>()},
            {"time_limit_exceeded", row[6].get<uint64_t>()}
        };

        // convert the JSON object into a string
        // this string can later be returned through the API
        return progress.dump();
    }
    catch (const mysqlx::Error& e) {

        // display the database error if the query fails
        std::cerr << "Failed to get user progress: " << e.what() << std::endl;

        // return an empty JSON object if an error occurs
        return "{}";
    }
}

// function to update the progress of a user
// based on the result of a submission

bool Database::updateUserProgress(
    int submissionId,
    const std::string& status
) {
    try {
        // Create a session to connect with the database
        mysqlx::Session session(host, port, username, password);

        // Select the database to work with
        session.sql("USE " + database).execute();

        // Retrieve the user and problem associated with the submission
        auto result = session.sql(
            "SELECT user_id, problem_id "
            "FROM submissions "
            "WHERE submission_id = ?"
        )
        .bind(submissionId)
        .execute();

        // Fetch the submission information
        auto row = result.fetchOne();

        // Stop if the submission does not exist
        if (row.isNull()) {
            return false;
        }

        // Get the user ID and problem ID
        int userId = row[0].get<int>();
        int problemId = row[1].get<int>();

        // Insert a progress record or update the existing record
        session.sql(
            "INSERT INTO user_progress "
            "(user_id, problem_id, is_solved, solved, "
            "first_solved_at, attempts) "
            "VALUES (?, ?, ?, ?, "
            "CASE WHEN ? = 'ACCEPTED' "
            "THEN CURRENT_TIMESTAMP ELSE NULL END, 1) "

            "ON DUPLICATE KEY UPDATE "
            "attempts = attempts + 1, "

            "is_solved = CASE "
            "WHEN ? = 'ACCEPTED' THEN TRUE "
            "ELSE is_solved "
            "END, "

            "solved = CASE "
            "WHEN ? = 'ACCEPTED' THEN TRUE "
            "ELSE solved "
            "END, "

            "first_solved_at = CASE "
            "WHEN ? = 'ACCEPTED' "
            "AND first_solved_at IS NULL "
            "THEN CURRENT_TIMESTAMP "
            "ELSE first_solved_at "
            "END"
        )
        .bind(userId)
        .bind(problemId)
        .bind(status == "ACCEPTED")
        .bind(status == "ACCEPTED")
        .bind(status)
        .bind(status)
        .bind(status)
        .bind(status)
        .execute();

        return true;
    }
    catch (const mysqlx::Error& e) {
        std::cerr << "Failed to update user progress: "
                  << e.what() << std::endl;
        return false;
    }
}



// This function creates a new user in the users table.
//
// It returns:
// true  -> user was created successfully
// false -> user creation failed
bool Database::createUser(
    const std::string& name,
    const std::string& usernameValue,
    const std::string& email,
    const std::string& passwordHash
) {
    try {

        // Create a new MySQL session using
        // the database connection details.
        mysqlx::Session session(
            host,
            port,
            username,
            password
        );

        // Select the database that will be used.
        session.sql(
            "USE " + database
        ).execute();

        // Insert the new user's details
        // into the users table.
        //
        // The password is stored as a hash,
        // not as plain text.
        session.sql(
            "INSERT INTO users "
            "(name, username, email, password_hash) "
            "VALUES (?, ?, ?, ?)"
        )

        // Bind the actual values to the
        // placeholders in the SQL query.
        .bind(name)
        .bind(usernameValue)
        .bind(email)
        .bind(passwordHash)

        // Execute the INSERT query.
        .execute();

        // If the query executes successfully,
        // return true.
        return true;
    }
    catch (const mysqlx::Error& e) {

        // Display the database error if
        // user creation fails.
        std::cerr
            << "User creation failed: "
            << e.what()
            << std::endl;

        // Return false to indicate failure.
        return false;
    }
}


// This function retrieves the role of a specific user.
//
// It returns:
// "USER"  -> normal user
// "ADMIN" -> administrator
// ""      -> user not found or database error
std::string Database::getUserRole(int userId)
{
    try
    {
        // Create a new MySQL session using
        // the database connection details.
        mysqlx::Session session(host, port, username, password);

        // Select the GrowCode database.
        session.sql("USE " + database).execute();

        // Retrieve the role of the user
        // whose user_id matches the given ID.
        auto result = session.sql(
            "SELECT role "
            "FROM users "
            "WHERE user_id = ?"
        )
        .bind(userId)
        .execute();

        // Fetch the first row returned by the query.
        auto row = result.fetchOne();

        // If no user was found,
        // return an empty string.
        if (row.isNull())
        {
            return "";
        }

        // Return the user's role.
        return row[0].get<std::string>();
    }
    catch (const mysqlx::Error& e)
    {
        // Display the database error if
        // retrieving the role fails.
        std::cerr
            << "Failed to get user role: "
            << e.what()
            << std::endl;

        // Return an empty string to indicate failure.
        return "";
    }
}


// This function creates a new problem in the problems table.
//
// It returns:
// problem ID  -> if the problem was created successfully
// 0           -> if problem creation failed
int Database::createProblem(
    const std::string& title,
    const std::string& description,
    const std::string& difficulty,
    const std::string& constraints,
    int createdBy
) {
    try {

        // Create a new MySQL session using
        // the database connection details.
        mysqlx::Session session(
            host,
            port,
            username,
            password
        );

        // Select the GrowCode database.
        session.sql(
            "USE " + database
        ).execute();

        // Insert the new problem into the problems table.
        session.sql(
            "INSERT INTO problems "
            "(title, description, difficulty, constraints, created_by) "
            "VALUES (?, ?, ?, ?, ?)"
        )
        .bind(title)
        .bind(description)
        .bind(difficulty)
        .bind(constraints)
        .bind(createdBy)
        .execute();

        // Retrieve the ID automatically generated
        // by the AUTO_INCREMENT column.
        auto result = session.sql(
            "SELECT LAST_INSERT_ID()"
        ).execute();

        auto row = result.fetchOne();

        // If no ID was returned, problem creation failed.
        if (row.isNull()) {
            return 0;
        }

        // Return the newly created problem ID.
        return row[0].get<int>();
    }
    catch (const mysqlx::Error& e) {

        // Display the database error.
        std::cerr
            << "Problem creation failed: "
            << e.what()
            << std::endl;

        // Return 0 to indicate failure.
        return 0;
    }
}

// This function creates a new test case
// for an existing problem.
//
// It returns:
// test case ID -> if creation succeeds
// 0            -> if creation fails
int Database::createTestCase(
    int problemId,
    const std::string& input,
    const std::string& expectedOutput,
    bool isSample,
    int orderNo
) {
    try {

        // Create a new MySQL session.
        mysqlx::Session session(
            host,
            port,
            username,
            password
        );

        // Select the GrowCode database.
        session.sql(
            "USE " + database
        ).execute();

        // Insert the test case into the test_cases table.
        session.sql(
            "INSERT INTO test_cases "
            "(problem_id, input, expected_output, is_sample, order_no) "
            "VALUES (?, ?, ?, ?, ?)"
        )
        .bind(problemId)
        .bind(input)
        .bind(expectedOutput)
        .bind(isSample)
        .bind(orderNo)
        .execute();

        // Retrieve the ID automatically generated
        // by the AUTO_INCREMENT column.
        auto result = session.sql(
            "SELECT LAST_INSERT_ID()"
        ).execute();

        auto row = result.fetchOne();

        // If no ID was returned, test-case creation failed.
        if (row.isNull()) {
            return 0;
        }

        // Return the newly created test case ID.
        return row[0].get<int>();
    }
    catch (const mysqlx::Error& e) {

        // Display the database error.
        std::cerr
            << "Test case creation failed: "
            << e.what()
            << std::endl;

        // Return 0 to indicate failure.
        return 0;
    }
}

// This function finds a user using their username.
//
// It retrieves:
// user ID
// password hash
// user role
//
// It returns:
// true  -> user was found
// false -> user was not found or database error occurred
bool Database::findUser(
    const std::string& usernameValue,
    std::string& storedHash,
    int& userId,
    std::string& role
) {
    try {

        // Create a new MySQL session.
        mysqlx::Session session(
            host,
            port,
            username,
            password
        );

        // Select the GrowCode database.
        session.sql(
            "USE " + database
        ).execute();

        // Find the user using their username.
        auto result = session.sql(
            "SELECT user_id, password_hash, role "
            "FROM users "
            "WHERE username = ?"
        )
        .bind(usernameValue)
        .execute();

        // Retrieve the first matching row.
        auto row = result.fetchOne();

        // If no user was found,
        // return false.
        if (row.isNull()) {
            return false;
        }

        // Store the user's ID.
        userId = row[0].get<int>();

        // Store the hashed password.
        storedHash = row[1].get<std::string>();

        // Store the user's role.
        role = row[2].get<std::string>();

        // User was successfully found.
        return true;
    }
    catch (const mysqlx::Error& e) {

        // Display the database error.
        std::cerr
            << "Failed to find user: "
            << e.what()
            << std::endl;

        // Return false to indicate failure.
        return false;
    }
}

std::string Database::getLeaderboard()
{
    try {
        mysqlx::Session session(
            host,
            port,
            username,
            password
        );

        session.sql("USE " + database).execute();

        auto result = session.sql(
            "SELECT "
            "u.user_id, u.name, u.username, "
            "CAST(COALESCE(SUM(CASE "
            "WHEN up.solved = TRUE THEN 1 ELSE 0 END), 0) AS UNSIGNED) "
            "AS problems_solved, "
            "CAST(COALESCE(SUM(up.attempts), 0) AS UNSIGNED) "
            "AS total_attempts "
            "FROM users u "
            "LEFT JOIN user_progress up ON up.user_id = u.user_id "
            "WHERE u.role = 'USER' "
            "GROUP BY u.user_id, u.name, u.username "
            "ORDER BY problems_solved DESC, total_attempts ASC, u.username ASC"
        ).execute();

        nlohmann::json leaderboard = nlohmann::json::array();
        auto rows = result.fetchAll();
        int rank = 1;

        for (const auto& row : rows) {
            leaderboard.push_back({
                {"rank", rank++},
                {"user_id", row[0].get<int>()},
                {"name", row[1].get<std::string>()},
                {"username", row[2].get<std::string>()},
                {"problems_solved", row[3].get<uint64_t>()},
                {"total_attempts", row[4].get<uint64_t>()}
            });
        }

        return leaderboard.dump();
    }
    catch (const mysqlx::Error& e) {
        std::cerr
            << "Failed to get leaderboard: "
            << e.what()
            << std::endl;

        return "[]";
    }
}


std::optional<std::string> Database::getSubmissionCode(
    int userId,
    int submissionId
) {
    // create a session to connect with the database
    mysqlx::Session session(host, port, username, password);

    // select the database to work with
    session.sql("USE " + database).execute();

    // retrieve source code only if the submission belongs to this user
    auto result = session.sql(
        "SELECT code "
        "FROM submissions "
        "WHERE submission_id = ? "
        "AND user_id = ?"
    )
    .bind(submissionId, userId)
    .execute();

    // retrieve the matching submission
    mysqlx::Row row = result.fetchOne();

    // return an empty optional if the submission is not found
    if (!row) {
        return std::nullopt;
    }

    // return the original submitted source code
    return row[0].get<std::string>();
}
