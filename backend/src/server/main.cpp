// Build -  
// cd ~/Desktop/growcode
// rm -rf build
// mkdir build
// cd build
// cmake ..
// make -j$(sysctl -n hw.ncpu)

// Run -
// cd ~/Desktop/growcode
// ./build/growcode


// For other sys -
// IP:8080 in browser

// Command for find ip -  ipconfig getifaddr en0
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <mutex>
#include <random>
#include "httplib.h"
#include "database/Database.h"
#include "auth/AuthManager.h"
#include "execution_engine/scheduler/Scheduler.h"
#include "execution_engine/workers/WorkerPool.h"
#include "execution_engine/executor/ExecutionWorker.h"

std::unordered_map<std::string, int> sessions;
std::mutex sessionMutex;

std::string generateSessionToken() {
    static std::mt19937_64 generator(
        std::random_device{}()
    );

    std::uniform_int_distribution<unsigned long long> distribution;

    std::ostringstream token;

    token << std::hex << distribution(generator)<< distribution(generator);

    return token.str();
}

bool getUserIdFromSession(
    const httplib::Request& req,
    int& userId
) {
    if (!req.has_header("Cookie")) {
        return false;
    }

    std::string cookie = req.get_header_value("Cookie");

    const std::string key = "session_token=";
    std::size_t start = cookie.find(key);

    if (start == std::string::npos) {
        return false;
    }

    start += key.length();

    std::size_t end = cookie.find(';', start);

    std::string token;

    if (end == std::string::npos) {
        token = cookie.substr(start);
    } else {
        token = cookie.substr(start, end - start);
    }

    if (token.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(sessionMutex);

    auto it = sessions.find(token);

    if (it == sessions.end()) {
        return false;
    }

    userId = it->second;

    return true;
}

std::string getSessionToken(
    const httplib::Request& req
) {
    if (!req.has_header("Cookie")) {
        return "";
    }

    std::string cookie = req.get_header_value("Cookie");
    const std::string key = "session_token=";

    std::size_t start = cookie.find(key);

    if (start == std::string::npos) {
        return "";
    }

    start += key.length();

    std::size_t end = cookie.find(';', start);

    if (end == std::string::npos) {
        return cookie.substr(start);
    }

    return cookie.substr(start, end - start);
}

bool isAdmin(
    const httplib::Request& req,
    Database& database
) {
    int userId = 0;

    if (!getUserIdFromSession(req, userId)) {
        return false;
    }

    return database.getUserRole(userId) == "ADMIN";
}

int main() {
    Database database(
        "127.0.0.1",
        33060,
        "root",
        "Pass",
        "growcode_app"
    );

    Scheduler scheduler;
    ExecutionWorker executionWorker(database);
    WorkerPool workerPool(scheduler, executionWorker, 2);

    std::vector<int> queuedSubmissions =
    database.getQueuedSubmissionIds();

    for (int submissionId : queuedSubmissions) {
        scheduler.addJob(submissionId);
    }

    workerPool.start();

    httplib::Server server;

    server.Get("/", [&](const httplib::Request&, httplib::Response& res) {
        res.set_content(
            "<!DOCTYPE html>"
            "<html>"
            "<head>"
            "<title>GrowCode</title>"
            "</head>"
            "<body>"
            "<h1>GrowCode</h1>"
            "<p>GrowCode Server is running.</p>"
            "<p>Database connection successful.</p>"
            "</body>"
            "</html>",
            "text/html"
        );
    });

    
server.Get("/api/problems", [&](const httplib::Request&, httplib::Response& res) {
    try {
        std::vector<Problem> problems = database.getProblems();

        nlohmann::json json = nlohmann::json::array();

        for (const auto& problem : problems) {
            json.push_back({
                {"problem_id", problem.problem_id},
                {"title", problem.title},
                {"description", problem.description},
                {"difficulty", problem.difficulty},
                {"constraints", problem.constraints}
            });
        }

        res.set_content(json.dump(), "application/json");
    }
    catch (const std::exception& e) {
        std::cerr << "Problem retrieval error: " << e.what() << std::endl;

        res.status = 500;
        res.set_content(
            R"({"error":"Failed to retrieve problems"})",
            "application/json"
        );
    }
});

server.Get(R"(/api/problems/(\d+))",
    [&](const httplib::Request& req, httplib::Response& res) {
        try {
            int problem_id = std::stoi(req.matches[1].str());

            Problem problem = database.getProblem(problem_id);
            std::vector<TestCase> testCases =
                database.getTestCases(problem_id);

            nlohmann::json json = {
                {"problem_id", problem.problem_id},
                {"title", problem.title},
                {"description", problem.description},
                {"difficulty", problem.difficulty},
                {"constraints", problem.constraints},
                {"sample_test_cases", nlohmann::json::array()}
            };

            for (const auto& testCase : testCases) {
                if (!testCase.isSample) {
                    continue;
                }

                json["sample_test_cases"].push_back({
                    {"input", testCase.input},
                    {"expected_output", testCase.expectedOutput}
                });
            }

            res.set_content(json.dump(), "application/json");
        }
        catch (const std::exception& e) {
            std::cerr << "Problem retrieval error: "
                      << e.what() << std::endl;

            res.status = 404;
            res.set_content(
                R"({"error":"Problem not found"})",
                "application/json"
            );
        }
    }
);


    server.Post("/api/register", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string name = req.get_param_value("name");
            std::string username = req.get_param_value("username");
            std::string email = req.get_param_value("email");
            std::string password = req.get_param_value("password");
    
            if (name.empty() ||
                username.empty() ||
                email.empty() ||
                password.empty()) {
    
                res.status = 400;
    
                res.set_content(
                    "{\"error\":\"All fields are required\"}",
                    "application/json"
                );
    
                return;
            }
    
            std::string passwordHash =
                AuthManager::hashPassword(password);
    
            if (passwordHash.empty()) {
                res.status = 500;
    
                res.set_content(
                    "{\"error\":\"Password hashing failed\"}",
                    "application/json"
                );
    
                return;
            }
    
            bool created = database.createUser(
                name,
                username,
                email,
                passwordHash
            );
    
            if (!created) {
                res.status = 409;
    
                res.set_content(
                    "{\"error\":\"Username or email already exists\"}",
                    "application/json"
                );
    
                return;
            }
    
            res.set_content(
                "{\"message\":\"Registration successful\"}",
                "application/json"
            );
        }
        catch (const std::exception& e) {
            res.status = 500;
    
            res.set_content(
                "{\"error\":\"Registration failed\"}",
                "application/json"
            );
    
            std::cerr << e.what() << std::endl;
        }
    });

    server.Get(
        "/api/admin/check",
        [&](const httplib::Request& req, httplib::Response& res) {
    
            if (!isAdmin(req, database)) {
                res.status = 403;
                res.set_content(
                    "{\"error\":\"Admin access required\"}",
                    "application/json"
                );
                return;
            }
    
            nlohmann::json response = {
                {"message", "Admin access verified"},
                {"role", "ADMIN"}
            };
    
            res.set_content(
                response.dump(),
                "application/json"
            );
        }
    );

    server.Get(
        "/api/admin/problems",
        [&](const httplib::Request& req, httplib::Response& res) {
    
            if (!isAdmin(req, database)) {
                res.status = 403;
                res.set_content(
                    "{\"error\":\"Admin access required\"}",
                    "application/json"
                );
                return;
            }
    
            try {
                std::vector<Problem> problems =
                    database.getProblems();
    
                nlohmann::json response =
                    nlohmann::json::array();
    
                for (const auto& problem : problems) {
                    response.push_back({
                        {"problem_id", problem.problem_id},
                        {"title", problem.title},
                        {"description", problem.description},
                        {"difficulty", problem.difficulty},
                        {"constraints", problem.constraints}
                    });
                }
    
                res.set_content(
                    response.dump(),
                    "application/json"
                );
            }
            catch (const std::exception& e) {
                res.status = 500;
                res.set_content(
                    "{\"error\":\"Failed to retrieve problems\"}",
                    "application/json"
                );
    
                std::cerr << e.what() << std::endl;
            }
        }
    );

    server.Get(
        R"(/api/admin/test-cases/(\d+))",
        [&](const httplib::Request& req, httplib::Response& res) {
    
            if (!isAdmin(req, database)) {
                res.status = 403;
                res.set_content(
                    "{\"error\":\"Admin access required\"}",
                    "application/json"
                );
                return;
            }
    
            try {
                int problemId =
                    std::stoi(req.matches[1].str());
    
                std::vector<TestCase> testCases =
                    database.getTestCases(problemId);
    
                nlohmann::json response =
                    nlohmann::json::array();
    
                for (const auto& testCase : testCases) {
                    response.push_back({
                        {"test_case_id", testCase.testCaseId},
                        {"input", testCase.input},
                        {"expected_output", testCase.expectedOutput}
                    });
                }
    
                res.set_content(
                    response.dump(),
                    "application/json"
                );
            }
            catch (const std::exception& e) {
                res.status = 500;
                res.set_content(
                    "{\"error\":\"Failed to retrieve test cases\"}",
                    "application/json"
                );
    
                std::cerr << e.what() << std::endl;
            }
        }
    );

    server.Post(
        "/api/admin/problems",
        [&](const httplib::Request& req, httplib::Response& res) {
    
            if (!isAdmin(req, database)) {
                res.status = 403;
                res.set_content(
                    "{\"error\":\"Admin access required\"}",
                    "application/json"
                );
                return;
            }
    
            try {
                std::string title = req.get_param_value("title");
                std::string description = req.get_param_value("description");
                std::string difficulty = req.get_param_value("difficulty");
                std::string constraints = req.get_param_value("constraints");
    
                if (
                    title.empty() ||
                    description.empty() ||
                    difficulty.empty()
                ) {
                    res.status = 400;
                    res.set_content(
                        "{\"error\":\"Title, description and difficulty are required\"}",
                        "application/json"
                    );
                    return;
                }
    
                int userId = 0;
    
                if (!getUserIdFromSession(req, userId)) {
                    res.status = 401;
                    res.set_content(
                        "{\"error\":\"Login required\"}",
                        "application/json"
                    );
                    return;
                }
    
                int problemId = database.createProblem(
                    title,
                    description,
                    difficulty,
                    constraints,
                    userId
                );
    
                if (problemId == 0) {
                    res.status = 500;
                    res.set_content(
                        "{\"error\":\"Failed to create problem\"}",
                        "application/json"
                    );
                    return;
                }
    
                nlohmann::json response = {
                    {"message", "Problem created successfully"},
                    {"problem_id", problemId}
                };
    
                res.set_content(
                    response.dump(),
                    "application/json"
                );
            }
            catch (const std::exception& e) {
                res.status = 500;
                res.set_content(
                    "{\"error\":\"Failed to create problem\"}",
                    "application/json"
                );
    
                std::cerr << e.what() << std::endl;
            }
        }
    );

    server.Post(
        "/api/admin/test-cases",
        [&](const httplib::Request& req, httplib::Response& res) {
    
            if (!isAdmin(req, database)) {
                res.status = 403;
                res.set_content(
                    "{\"error\":\"Admin access required\"}",
                    "application/json"
                );
                return;
            }
    
            try {
                std::string problemIdValue = req.get_param_value("problem_id");
    
                std::string input = req.get_param_value("input");
    
                std::string expectedOutput = req.get_param_value("expected_output");
    
                std::string isSampleValue = req.get_param_value("is_sample");
    
                std::string orderValue = req.get_param_value("order_no");
    
                if (
                    problemIdValue.empty() ||
                    expectedOutput.empty() ||
                    orderValue.empty()
                ) {
                    res.status = 400;
                    res.set_content(
                        "{\"error\":\"Problem ID, expected output and order are required\"}",
                        "application/json"
                    );
                    return;
                }
    
                int problemId = std::stoi(problemIdValue);
                int orderNo = std::stoi(orderValue);
    
                bool isSample =
                    (isSampleValue == "true" ||
                     isSampleValue == "1");
    
                int testCaseId = database.createTestCase(
                    problemId,
                    input,
                    expectedOutput,
                    isSample,
                    orderNo
                );
    
                if (testCaseId == 0) {
                    res.status = 500;
                    res.set_content(
                        "{\"error\":\"Failed to create test case\"}",
                        "application/json"
                    );
                    return;
                }
    
                nlohmann::json response = {
                    {"message", "Test case created successfully"},
                    {"test_case_id", testCaseId}
                };
    
                res.set_content(
                    response.dump(),
                    "application/json"
                );
            }
            catch (const std::exception& e) {
                res.status = 500;
                res.set_content(
                    "{\"error\":\"Failed to create test case\"}",
                    "application/json"
                );
    
                std::cerr << e.what() << std::endl;
            }
        }
    );

    server.Post("/api/login", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string username = req.get_param_value("username");
            std::string password = req.get_param_value("password");
    
            std::string storedHash;
            std::string role;
            int userId = 0;

            bool found = database.findUser(
                username,
                storedHash,
                userId,
                role
            );
    
            bool passwordValid = AuthManager::verifyPassword(
                password,
                storedHash
            );
            
            if (!found || !passwordValid) {
            
                res.status = 401;
            
                res.set_content(
                    "{\"error\":\"Invalid username or password\"}",
                    "application/json"
                );
            
                return;
            }
    
            std::string sessionToken = generateSessionToken();

            {
                std::lock_guard<std::mutex> lock(sessionMutex);
                sessions[sessionToken] = userId;
            }

            res.set_header(
                "Set-Cookie",
                "session_token=" + sessionToken + "; Path=/; HttpOnly"
            );

            nlohmann::json response = {
                {"message", "Login successful"},
                {"username", username},
                {"role", role}
            };
    
            res.set_content(
                response.dump(),
                "application/json"
            );
        }
        catch (const std::exception& e) {
            res.status = 500;
    
            res.set_content(
                "{\"error\":\"Login failed\"}",
                "application/json"
            );
    
            std::cerr << e.what() << std::endl;
        }
    });

    server.Get("/api/session", [&](const httplib::Request& req, httplib::Response& res) {
        int userId = 0;
    
        if (!getUserIdFromSession(req, userId)) {
            res.status = 401;
            res.set_content(
                R"({"logged_in":false})",
                "application/json"
            );
            return;
        }
    
        // Get the role from the database, not from frontend input.
        std::string role = database.getUserRole(userId);
    
        nlohmann::json response = {
            {"logged_in", true},
            {"user_id", userId},
            {"role", role}
        };
    
        res.set_content(response.dump(), "application/json");
    });

    server.Post("/api/logout", [&](const httplib::Request& req, httplib::Response& res) {
        std::string token = getSessionToken(req);
    
        if (!token.empty()) {
            std::lock_guard<std::mutex> lock(sessionMutex);
            sessions.erase(token);
        }
    
        res.set_header(
            "Set-Cookie",
            "session_token=; Path=/; Max-Age=0; HttpOnly"
        );
    
        res.set_content(
            "{\"message\":\"Logout successful\"}",
            "application/json"
        );
    });
    
    server.Get("/api/progress", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            int userId = 0;
    
            if (!getUserIdFromSession(req, userId)) {
                res.status = 401;
                res.set_content(
                    "{\"error\":\"Authentication required\"}",
                    "application/json"
                );
                return;
            }
    
            std::string progress =
                database.getUserProgress(userId);
    
            res.set_content(
                progress,
                "application/json"
            );
        }
        catch (const std::exception& e) {
            std::cerr
                << "Failed to get progress: "
                << e.what()
                << std::endl;
    
            res.status = 500;
            res.set_content(
                "{\"error\":\"Internal server error\"}",
                "application/json"
            );
        }
    });

    server.Get("/api/submissions", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            int userId = 0;
    
            if (!getUserIdFromSession(req, userId)) {
                res.status = 401;
                res.set_content(
                    "{\"error\":\"Authentication required\"}",
                    "application/json"
                );
                return;
            }
    
            std::string submissions =
                database.getUserSubmissions(userId);
    
            res.set_content(
                submissions,
                "application/json"
            );
        }
        catch (const std::exception& e) {
            std::cerr
                << "Failed to get submissions: "
                << e.what()
                << std::endl;
    
            res.status = 500;
            res.set_content(
                "{\"error\":\"Internal server error\"}",
                "application/json"
            );
        }
    });


    server.Get(
        R"(/api/submissions/(\d+)/code)",
        [&](const httplib::Request& req, httplib::Response& res) {
            try {
                int userId = 0;

                // verify that the user is logged in
                if (!getUserIdFromSession(req, userId)) {
                    res.status = 401;
                    res.set_content(
                        R"({"error":"Authentication required"})",
                        "application/json"
                    );
                    return;
                }

                // extract the submission ID from the URL
                int submissionId = std::stoi(req.matches[1].str());

                if (submissionId <= 0) {
                    res.status = 400;
                    res.set_content(
                        R"({"error":"Invalid submission ID"})",
                        "application/json"
                    );
                    return;
                }

                // retrieve code only for this user's submission
                auto code = database.getSubmissionCode(
                    userId,
                    submissionId
                );

                if (!code.has_value()) {
                    res.status = 404;
                    res.set_content(
                        R"({"error":"Submission not found"})",
                        "application/json"
                    );
                    return;
                }

                // serialize source code safely as JSON
                nlohmann::json response = {
                    {"code", code.value()}
                };

                res.set_content(
                    response.dump(),
                    "application/json"
                );
            }
            catch (const std::exception& e) {
                std::cerr
                    << "Failed to retrieve submission code: "
                    << e.what()
                    << std::endl;

                res.status = 500;
                res.set_content(
                    R"({"error":"Internal server error"})",
                    "application/json"
                );
            }
        }
    );


    server.Post("/api/submissions", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            int userId = 0;
    
            if (!getUserIdFromSession(req, userId)) {
                res.status = 401;
                res.set_content(
                    "{\"error\":\"Authentication required\"}",
                    "application/json"
                );
                return;
            }
    
            auto json = nlohmann::json::parse(req.body);
    
            if (!json.contains("problem_id") ||
                !json.contains("language_id") ||
                !json.contains("code")) {
    
                res.status = 400;
                res.set_content(
                    "{\"error\":\"Missing required fields\"}",
                    "application/json"
                );
                return;
            }
    
            int problemId = json["problem_id"].get<int>();
            int languageId = json["language_id"].get<int>();
            std::string code = json["code"].get<std::string>();
    
            int submissionId = database.createSubmissionWithJob(
                userId,
                problemId,
                languageId,
                code
            );
            
            if (submissionId <= 0) {
                res.status = 500;
                res.set_content(
                    "{\"error\":\"Failed to create submission and execution job\"}",
                    "application/json"
                );
                return;
            }
            
            scheduler.addJob(submissionId);
    
            nlohmann::json response = {
                {"message", "Submission received"},
                {"submission_id", submissionId}
            };
    
            res.status = 201;
            res.set_content(
                response.dump(),
                "application/json"
            );
        }
        catch (const std::exception& e) {
            std::cerr
                << "Submission error: "
                << e.what()
                << std::endl;
    
            res.status = 500;
            res.set_content(
                "{\"error\":\"Internal server error\"}",
                "application/json"
            );
        }
    });

    server.Get(
        R"(/api/submissions/(\d+))",
        [&](const httplib::Request& req, httplib::Response& res) {
            try {
                int userId = 0;
            
                if (!getUserIdFromSession(req, userId)) {
                    res.status = 401;
            
                    res.set_content(
                        "{\"error\":\"Authentication required\"}",
                        "application/json"
                    );
            
                    return;
                }
            
                int submissionId =
                    std::stoi(req.matches[1].str());
            
                std::string status =
                    database.getSubmissionStatus(
                        submissionId,
                        userId
                    );
            
                if (status.empty()) {
                    res.status = 404;
            
                    res.set_content(
                        "{\"error\":\"Submission not found\"}",
                        "application/json"
                    );
            
                    return;
                }
    
                nlohmann::json response = {
                    {"submission_id", submissionId},
                    {"status", status}
                };
    
                res.set_content(
                    response.dump(),
                    "application/json"
                );
            }
            catch (const std::exception& e) {
                std::cerr
                    << "Submission status error: "
                    << e.what()
                    << std::endl;
    
                res.status = 500;
    
                res.set_content(
                    "{\"error\":\"Failed to get submission status\"}",
                    "application/json"
                );
            }
        }
    );

    server.Get("/api/leaderboard", [&](const httplib::Request&, httplib::Response& res) {
        try {
            res.set_content(
                database.getLeaderboard(),
                "application/json"
            );
        }
        catch (const std::exception& e) {
            std::cerr
                << "Failed to get leaderboard: "
                << e.what()
                << std::endl;

            res.status = 500;
            res.set_content(
                "{\"error\":\"Unable to load leaderboard\"}",
                "application/json"
            );
        }
    });

    server.set_mount_point("/", "../frontend");

    std::cout << "GrowCode server starting on http://localhost:8080" << std::endl;
        
    server.listen("0.0.0.0", 8080);

    return 0;
}
