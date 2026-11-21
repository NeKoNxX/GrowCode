// Build -  
// cd ~/Desktop/GrowCode
// export GEMINI_API_KEY="API"
// rm -rf build
// mkdir build
// cd build
// cmake ..
// make -j$(sysctl -n hw.ncpu)

// Run -
// cd ~/Desktop/GrowCode
// ./build/growcode


// For other sys -
// IP:8080 in browser

// Command for find ip -  ipconfig getifaddr en0
// Command for docker activity - docker events --filter type=container
#include <iostream>
#include <unordered_set>
#include <regex>
#include <algorithm>
#include <sstream>
#include<cmath>
#include<unordered_map>
#include<string>
#include<vector>
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
#include "execution_engine/executor/RunCodeExecutor.h"
#include "complexity/ComplexityAnalyzer.h"

using namespace std;

std::unordered_map<std::string, int> sessions;
std::mutex sessionMutex;

std::string generateSessionToken() {
    static std::mt19937_64 generator(std::random_device{}());

    std::uniform_int_distribution<unsigned long long> distribution;

    std::ostringstream token;

    token << std::hex << distribution(generator)<< distribution(generator);

    return token.str();
}

bool getUserIdFromSession(const httplib::Request& req, int& userId) {
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

    std::lock_guard<std::mutex> lock(sessionMutex); // lock for mutex

    auto it = sessions.find(token);

    if (it == sessions.end()) {
        return false;
    }

    userId = it->second;

    return true;
}

std::string getSessionToken(const httplib::Request& req) {
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

bool isAdmin(const httplib::Request& req,Database& database) {
    int userId = 0;

    if (!getUserIdFromSession(req, userId)) {
        return false;
    }

    return database.getUserRole(userId) == "ADMIN";
}

vector<string> sourceTokens(const string& source) {
    string code = source;

    // 1. Remove block comments and line comments.
    code = regex_replace(
        code,
        regex(R"(/\*[\s\S]*?\*/|//[^\n]*)"),
        " "
    );

    // 2. Remove preprocessor directives.
    code = regex_replace(
        code,
        regex(R"(^\s*#.*$)", regex::multiline),
        " "
    );

    // 3. Remove namespace boilerplate.
    code = regex_replace(
        code,
        regex(R"(using\s+namespace\s+std\s*;)"),
        " "
    );

    // 4. Remove main() signature.
    code = regex_replace(
        code,
        regex(R"((int|signed|signed\s+int)\s+main\s*\([^)]*\)\s*\{)"),
        " "
    );

    // 5. Remove fast-I/O boilerplate.
    code = regex_replace(
        code,
        regex(R"(ios\s*::\s*sync_with_stdio\s*\(\s*false\s*\)\s*;)"),
        " "
    );

    code = regex_replace(
        code,
        regex(R"(cin\s*\.\s*tie\s*\(\s*(nullptr|0)\s*\)\s*;)"),
        " "
    );

    // 6. Remove common main() return statement.
    code = regex_replace(
        code,
        regex(R"(return\s+0\s*;)"),
        " "
    );

    // 7. Remove common typedef/using aliases.
    code = regex_replace(
        code,
        regex(R"(using\s+(ll|ull|vi|vl|pii|vll)\s*=\s*[^;]+;)"),
        " "
    );

    // 8. C++ keywords that must remain unchanged.
    static const unordered_set<string> keywords = {
        "if", "else", "for", "while", "do", "switch", "case",
        "break", "continue", "return", "void", "int", "long",
        "short", "float", "double", "char", "bool", "auto",
        "const", "static", "struct", "class", "public",
        "private", "protected", "namespace", "new", "delete",
        "try", "catch", "throw", "sizeof", "true", "false",
        "nullptr", "typedef", "using", "enum", "inline",
        "unsigned", "signed", "volatile", "extern"
    };

    // 9. Extract strings, characters, identifiers, numbers,
    //    operators and structural punctuation.
    const regex tokenRegex(
        R"("(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'|[A-Za-z_][A-Za-z0-9_]*|[0-9]+(?:\.[0-9]+)?|->\*|>>=|<<=|->|\+\+|--|\+=|-=|\*=|/=|%=|==|!=|<=|>=|&&|\|\||<<|>>|::|[{}()\[\];,.:?~!%^&*+\-/|<>=])"
    );

    vector<string> tokens;

    auto begin = sregex_iterator(
        code.begin(),
        code.end(),
        tokenRegex
    );

    auto end = sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        string token = it->str();

        // Normalize identifiers while preserving keywords.
        if (regex_match(token, regex(R"([A-Za-z_][A-Za-z0-9_]*)"))) {
            if (keywords.count(token)) {
                tokens.push_back(token);
            } else {
                tokens.push_back("IDENT");
            }
        }

        // Normalize numeric literals.
        else if (regex_match(token, regex(R"([0-9]+(?:\.[0-9]+)?)"))) {
            tokens.push_back("NUMBER");
        }

        // Normalize string literals.
        else if (!token.empty() && token.front() == '"') {
            tokens.push_back("STRING");
        }

        // Normalize character literals.
        else if (!token.empty() && token.front() == '\'') {
            tokens.push_back("CHAR");
        }

        // Preserve operators and structural punctuation.
        else {
            tokens.push_back(token);
        }
    }

    return tokens;
}


double ngramSimilarity(
    const vector<string>& a,
    const vector<string>& b,
    int n
) {
    if (a.size() < n || b.size() < n)
        return 0.0;

    unordered_set<string> setA, setB;

    auto buildNgrams = [&](const vector<string>& tokens,
                           unordered_set<string>& result) {
        for (int i = 0; i + n <= (int)tokens.size(); i++) {
            string gram;

            for (int j = 0; j < n; j++) {
                gram += tokens[i + j];
                gram += "|";
            }

            result.insert(gram);
        }
    };

    buildNgrams(a, setA);
    buildNgrams(b, setB);

    int intersection = 0;

    for (const auto& gram : setA) {
        if (setB.count(gram))
            intersection++;
    }

    int unionSize = setA.size() + setB.size() - intersection;

    if (unionSize == 0)
        return 0.0;

    return static_cast<double>(intersection) / unionSize;
}

double tokenSimilarity(
    const vector<string>& a,
    const vector<string>& b
) {
    double similarity3 = ngramSimilarity(a, b, 3);
    double similarity5 = ngramSimilarity(a, b, 5);

    return (0.4 * similarity3) + (0.6 * similarity5);
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
    // Serve frontend HTML, CSS, JavaScript, and other static files.
    if (!server.set_mount_point("/", "./frontend")) {
        std::cerr << "Failed to mount frontend directory" << std::endl;
        return 1;
    }

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

            std::string expectedTime, expectedSpace;
            database.getExpectedComplexity(problem_id, expectedTime, expectedSpace);
            nlohmann::json json = {
                {"problem_id", problem.problem_id},
                {"title", problem.title},
                {"description", problem.description},
                {"difficulty", problem.difficulty},
                {"constraints", problem.constraints},
                {"expected_time_complexity", expectedTime},
                {"expected_space_complexity", expectedSpace},
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
                    std::string expectedTime, expectedSpace;
                    database.getExpectedComplexity(problem.problem_id, expectedTime, expectedSpace);
                    std::string recommendedAlgorithm = database.getRecommendedAlgorithm(problem.problem_id);
                    response.push_back({
                        {"problem_id", problem.problem_id},
                        {"title", problem.title},
                        {"description", problem.description},
                        {"difficulty", problem.difficulty},
                        {"constraints", problem.constraints},
                        {"expected_time_complexity", expectedTime},
                        {"expected_space_complexity", expectedSpace},
                        {"recommended_algorithm", recommendedAlgorithm}
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
                        {"expected_output", testCase.expectedOutput},
                        {"is_sample", testCase.isSample}
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

    server.Delete(
        R"(/api/admin/problems/(\d+))",
        [&](const httplib::Request& req, httplib::Response& res) {
            if (!isAdmin(req, database)) {
                res.status = 403;
                res.set_content("{\"error\":\"Admin access required\"}", "application/json");
                return;
            }
            try {
                int problemId = std::stoi(req.matches[1].str());
                if (problemId <= 0 || !database.deleteProblem(problemId)) {
                    res.status = 404;
                    res.set_content("{\"error\":\"Problem not found or could not be deleted\"}", "application/json");
                    return;
                }
                res.set_content("{\"message\":\"Problem deleted successfully\"}", "application/json");
            } catch (const std::exception& e) {
                std::cerr << "Problem deletion failed: " << e.what() << std::endl;
                res.status = 500;
                res.set_content("{\"error\":\"Failed to delete problem\"}", "application/json");
            }
        }
    );

    server.Delete(
        R"(/api/admin/test-cases/(\d+))",
        [&](const httplib::Request& req, httplib::Response& res) {
            if (!isAdmin(req, database)) {
                res.status = 403;
                res.set_content("{\"error\":\"Admin access required\"}", "application/json");
                return;
            }
            try {
                int testCaseId = std::stoi(req.matches[1].str());
                if (testCaseId <= 0 || !database.deleteTestCase(testCaseId)) {
                    res.status = 404;
                    res.set_content("{\"error\":\"Test case not found or could not be deleted\"}", "application/json");
                    return;
                }
                res.set_content("{\"message\":\"Test case deleted successfully\"}", "application/json");
            } catch (const std::exception& e) {
                std::cerr << "Test case deletion failed: " << e.what() << std::endl;
                res.status = 500;
                res.set_content("{\"error\":\"Failed to delete test case\"}", "application/json");
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
    
                std::string expectedTime = req.get_param_value("expected_time_complexity");
                std::string expectedSpace = req.get_param_value("expected_space_complexity");
                std::string recommendedAlgorithm = req.get_param_value("recommended_algorithm");
                if (expectedTime.empty() || expectedSpace.empty() || recommendedAlgorithm.empty()) {
                    database.deleteProblem(problemId);
                    res.status = 400;
                    res.set_content("{\"error\":\"Expected time complexity, expected space complexity, and recommended algorithm are required\"}", "application/json");
                    return;
                }

                if (!database.saveProblemComplexity(problemId, expectedTime, expectedSpace, recommendedAlgorithm)) {
                    // Avoid leaving a problem without its requested metadata.
                    database.deleteProblem(problemId);
                    res.status = 500;
                    res.set_content("{\"error\":\"Problem created but complexity metadata could not be saved\"}", "application/json");
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

    // ---------------- ADMIN USER MANAGEMENT ----------------

    server.Get("/api/admin/users",
        [&](const httplib::Request& req, httplib::Response& res) {
            if (!isAdmin(req, database)) {
                res.status = 403;
                res.set_content(R"({"error":"Admin access required"})",
                                "application/json");
                return;
            }

            try {
                std::string search = req.has_param("search")
                    ? req.get_param_value("search") : "";

                int limit = req.has_param("limit")
                    ? std::stoi(req.get_param_value("limit")) : 50;

                int offset = req.has_param("offset")
                    ? std::stoi(req.get_param_value("offset")) : 0;

                res.set_content(
                    database.getAdminUsers(search, limit, offset).dump(),
                    "application/json"
                );
            } catch (const std::exception& e) {
                std::cerr << "[ADMIN USERS ERROR] "
                          << e.what() << std::endl;
            
                res.status = 500;
                res.set_content(
                    R"({"error":"Failed to load admin users"})",
                    "application/json"
                );
            }
            catch (...) {
                std::cerr << "[ADMIN USERS ERROR] Unknown exception"
                          << std::endl;
            
                res.status = 500;
                res.set_content(
                    R"({"error":"Unknown admin users error"})",
                    "application/json"
                );
            }
        }
    );

    server.Put(R"(/api/admin/users/(\d+)/status)",
        [&](const httplib::Request& req, httplib::Response& res) {
            int adminId = 0;

            if (!getUserIdFromSession(req, adminId) ||
                !isAdmin(req, database)) {
                res.status = 403;
                res.set_content(R"({"error":"Admin access required"})",
                                "application/json");
                return;
            }

            try {
                int targetId = std::stoi(req.matches[1].str());

                if (targetId <= 0 || targetId == adminId) {
                    res.status = 400;
                    res.set_content(
                        R"({"error":"Invalid target. You cannot disable your own account."})",
                        "application/json"
                    );
                    return;
                }

                auto body = nlohmann::json::parse(req.body);

                if (!body.contains("active") ||
                    !body["active"].is_boolean()) {
                    res.status = 400;
                    res.set_content(R"({"error":"active boolean required"})",
                                    "application/json");
                    return;
                }

                bool active = body["active"].get<bool>();

                if (!database.setUserActive(targetId, active)) {
                    res.status = 404;
                    res.set_content(R"({"error":"User not found"})",
                                    "application/json");
                    return;
                }

                // Invalidate all sessions belonging to this account.
                if (!active) {
                    std::lock_guard<std::mutex> lock(sessionMutex);

                    for (auto it = sessions.begin(); it != sessions.end();) {
                        if (it->second == targetId) {
                            it = sessions.erase(it);
                        } else {
                            ++it;
                        }
                    }
                }

                database.writeAdminAuditLog(
                    adminId,
                    active ? "USER_ENABLED" : "USER_DISABLED",
                    targetId,
                    nlohmann::json{{"active", active}}.dump(),
                    req.remote_addr
                );

                res.set_content(
                    nlohmann::json{
                        {"message", active ? "Account enabled" : "Account disabled"}
                    }.dump(),
                    "application/json"
                );
            } catch (...) {
                res.status = 400;
                res.set_content(R"({"error":"Invalid request"})",
                                "application/json");
            }
        }
    );

    server.Delete(R"(/api/admin/users/(\d+))",
        [&](const httplib::Request& req, httplib::Response& res) {
            int adminId = 0;

            if (!getUserIdFromSession(req, adminId) ||
                !isAdmin(req, database)) {
                res.status = 403;
                res.set_content(R"({"error":"Admin access required"})",
                                "application/json");
                return;
            }

            try {
                int targetId = std::stoi(req.matches[1].str());

                if (targetId <= 0 || targetId == adminId) {
                    res.status = 400;
                    res.set_content(
                        R"({"error":"You cannot delete your own account"})",
                        "application/json"
                    );
                    return;
                }

                database.writeAdminAuditLog(
                    adminId,
                    "USER_DELETE_REQUESTED",
                    targetId,
                    R"({"operation":"account deletion"})",
                    req.remote_addr
                );

                if (!database.deleteUserAccount(targetId)) {
                    res.status = 404;
                    res.set_content(R"({"error":"User not found"})",
                                    "application/json");
                    return;
                }

                {
                    std::lock_guard<std::mutex> lock(sessionMutex);

                    for (auto it = sessions.begin(); it != sessions.end();) {
                        if (it->second == targetId) {
                            it = sessions.erase(it);
                        } else {
                            ++it;
                        }
                    }
                }

                res.set_content(R"({"message":"User deleted"})",
                                "application/json");
            } catch (...) {
                res.status = 400;
                res.set_content(R"({"error":"Invalid user ID"})",
                                "application/json");
            }
        }
    );

    server.Get(R"(/api/admin/users/(\d+)/submissions)",
        [&](const httplib::Request& req, httplib::Response& res) {
            if (!isAdmin(req, database)) {
                res.status = 403;
                res.set_content(R"({"error":"Admin access required"})",
                                "application/json");
                return;
            }

            try {
                int userId = std::stoi(req.matches[1].str());

                int limit = req.has_param("limit")
                    ? std::stoi(req.get_param_value("limit")) : 50;

                int offset = req.has_param("offset")
                    ? std::stoi(req.get_param_value("offset")) : 0;

                res.set_content(
                    database.getAdminUserSubmissions(userId, limit, offset).dump(),
                    "application/json"
                );
            } catch (...) {
                res.status = 400;
                res.set_content(R"({"error":"Invalid request"})",
                                "application/json");
            }
        }
    );

    server.Get(R"(/api/admin/submissions/(\d+))",
        [&](const httplib::Request& req, httplib::Response& res) {
            if (!isAdmin(req, database)) {
                res.status = 403;
                res.set_content(R"({"error":"Admin access required"})",
                                "application/json");
                return;
            }

            try {
                int submissionId = std::stoi(req.matches[1].str());
                auto details = database.getAdminSubmissionDetails(submissionId);

                if (!details.has_value()) {
                    res.status = 404;
                    res.set_content(R"({"error":"Submission not found"})",
                                    "application/json");
                    return;
                }

                res.set_content(details->dump(), "application/json");
            } catch (...) {
                res.status = 400;
                res.set_content(R"({"error":"Invalid submission ID"})",
                                "application/json");
            }
        }
    );

    // Admin-only AI review.
    // Uses the existing ComplexityAnalyzer integration.
    server.Post("/api/admin/ai-review",
        [&](const httplib::Request& req, httplib::Response& res) {
            int adminId = 0;

            if (!getUserIdFromSession(req, adminId) ||
                !isAdmin(req, database)) {
                res.status = 403;
                res.set_content(R"({"error":"Admin access required"})",
                                "application/json");
                return;
            }

            try {
                auto body = nlohmann::json::parse(req.body);

                if (!body.contains("code") ||
                    !body.contains("language_id") ||
                    !body["code"].is_string() ||
                    !body["language_id"].is_number_integer()) {
                    res.status = 400;
                    res.set_content(
                        R"({"error":"code and language_id are required"})",
                        "application/json"
                    );
                    return;
                }

                std::string code = body["code"].get<std::string>();
                int languageId = body["language_id"].get<int>();

                if (code.empty() || code.size() > 50000) {
                    res.status = 400;
                    res.set_content(
                        R"({"error":"Code must be between 1 and 50000 bytes"})",
                        "application/json"
                    );
                    return;
                }

                const char* language =
                    languageId == 1 ? "C++17" :
                    languageId == 2 ? "Java" :
                    languageId == 3 ? "Python 3" : nullptr;

                if (!language) {
                    res.status = 400;
                    res.set_content(R"({"error":"Unsupported language"})",
                                    "application/json");
                    return;
                }

                auto review = ComplexityAnalyzer::analyze(
                    code,
                    language,
                    "Admin source-code review. Assess correctness risks, "
                    "readability, complexity and potential improvements. "
                    "Do not claim this proves AI authorship or misconduct."
                );

                database.writeAdminAuditLog(
                    adminId,
                    "AI_CODE_REVIEW",
                    0,
                    R"({"review_type":"source_code"})",
                    req.remote_addr
                );

                res.set_content(review.dump(), "application/json");
            } catch (...) {
                res.status = 503;
                res.set_content(
                    R"({"error":"AI review service unavailable"})",
                    "application/json"
                );
            }
        }
    );

    server.Get("/api/admin/audit",
        [&](const httplib::Request& req, httplib::Response& res) {
            if (!isAdmin(req, database)) {
                res.status = 403;
                res.set_content(R"({"error":"Admin access required"})",
                                "application/json");
                return;
            }

            res.set_content(
                database.getAdminAuditLogs(100).dump(),
                "application/json"
            );
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
            
            if (found && passwordValid && !database.isUserActive(userId)) {
                res.status = 403;
                res.set_content(
                    R"({"error":"This account has been disabled"})",
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


    server.Post("/api/run", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            int userId = 0;

            if (!getUserIdFromSession(req, userId)) {
                res.status = 401;
                res.set_content(
                    R"({"error":"Authentication required"})",
                    "application/json"
                );
                return;
            }

            auto json = nlohmann::json::parse(req.body);

            if (!json.is_object() ||
                !json.contains("code") ||
                !json.contains("language_id") ||
                !json.contains("input") ||
                !json["code"].is_string() ||
                !json["language_id"].is_number_integer() ||
                !json["input"].is_string()) {

                res.status = 400;
                res.set_content(
                    R"({"error":"Invalid or missing fields"})",
                    "application/json"
                );
                return;
            }

            std::string code = json["code"].get<std::string>();
            int languageId = json["language_id"].get<int>();
            std::string input = json["input"].get<std::string>();

            if (code.empty()) {
                res.status = 400;
                res.set_content(
                    R"({"error":"Source code cannot be empty"})",
                    "application/json"
                );
                return;
            }

            if (code.size() > 65536 || input.size() > 65536) {
                res.status = 413;
                res.set_content(
                    R"({"error":"Code or input exceeds the 64 KB limit"})",
                    "application/json"
                );
                return;
            }

            if (languageId < 1 || languageId > 3) {
                res.status = 400;
                res.set_content(
                    R"({"error":"Unsupported programming language"})",
                    "application/json"
                );
                return;
            }

            RunCodeResult result = RunCodeExecutor::execute(
                code,
                languageId,
                input
            );

            nlohmann::json response = {
                {"status", result.status},
                {"output", result.output},
                {"error", result.error},
                {"execution_time", result.executionTime},
                {"memory_used", result.memoryUsed}
            };

            res.status = 200;
            res.set_content(
                response.dump(),
                "application/json"
            );
        }
        catch (const nlohmann::json::exception& e) {
            res.status = 400;
            res.set_content(
                R"({"error":"Invalid JSON request"})",
                "application/json"
            );
        }
        catch (const std::exception& e) {
            std::cerr << "Run code error: " << e.what() << std::endl;

            res.status = 500;
            res.set_content(
                R"({"error":"Internal server error"})",
                "application/json"
            );
        }
    });


    
    // Optional AI complexity analysis.
    // Stored problem metadata is returned even if Gemini is unavailable.
    server.Post("/api/complexity/analyze",
        [&](const httplib::Request& req, httplib::Response& res) {
            try {
                int userId = 0;

                if (!getUserIdFromSession(req, userId)) {
                    res.status = 401;
                    res.set_content(
                        R"({"error":"Authentication required"})",
                        "application/json"
                    );
                    return;
                }

                auto body = nlohmann::json::parse(req.body);

                if (!body.contains("code") ||
                    !body.contains("language_id") ||
                    !body.contains("problem_id") ||
                    !body["code"].is_string()) {

                    res.status = 400;
                    res.set_content(
                        R"({"error":"code, language_id and problem_id are required"})",
                        "application/json"
                    );
                    return;
                }

                const int problemId = body["problem_id"].get<int>();
                const int languageId = body["language_id"].get<int>();
                const std::string code = body["code"].get<std::string>();

                const char* language =
                    languageId == 1 ? "C++17" :
                    languageId == 2 ? "Java" :
                    languageId == 3 ? "Python 3" : nullptr;

                if (language == nullptr) {
                    res.status = 400;
                    res.set_content(
                        R"({"error":"Unsupported language"})",
                        "application/json"
                    );
                    return;
                }

                // 1. Retrieve stored information FIRST.
                std::string expectedTime = "Not configured";
                std::string expectedSpace = "Not configured";

                database.getExpectedComplexity(
                    problemId,
                    expectedTime,
                    expectedSpace
                );

                std::string recommendedAlgorithm =
                    database.getRecommendedAlgorithm(problemId);

                if (recommendedAlgorithm.empty()) {
                    recommendedAlgorithm = "Not configured";
                }

                // 2. Prepare a response containing database information.
                nlohmann::json response = {
                    {"recommended_algorithm", recommendedAlgorithm},
                    {"expected_time_complexity", expectedTime},
                    {"expected_space_complexity", expectedSpace},
                    {"analysis_available", false}
                };

                // 3. Run Gemini independently.
                try {
                    Problem problem = database.getProblem(problemId);

                    auto analysis = ComplexityAnalyzer::analyze(
                        code,
                        language,
                        problem.description +
                        "\nConstraints: " +
                        problem.constraints
                    );

                    if (analysis.is_object()) {
                        response.update(analysis);
                    }

                    // Reapply database values so they cannot be overwritten.
                    response["recommended_algorithm"] =
                        recommendedAlgorithm;

                    response["expected_time_complexity"] =
                        expectedTime;

                    response["expected_space_complexity"] =
                        expectedSpace;

                    response["analysis_available"] = true;
                }
                catch (const std::exception& e) {
                    std::cerr
                        << "Gemini complexity analysis unavailable: "
                        << e.what()
                        << std::endl;

                    response["analysis_available"] = false;
                    response["analysis_error"] =
                        "AI complexity analysis is temporarily unavailable.";
                }

                // 4. Return HTTP 200 even when Gemini fails,
                // provided the stored database information was retrieved.
                res.status = 200;
                res.set_content(response.dump(), "application/json");
            }
            catch (const std::exception& e) {
                std::cerr
                    << "Complexity endpoint error: "
                    << e.what()
                    << std::endl;

                res.status = 500;
                res.set_content(
                    R"({"error":"Unable to retrieve complexity information."})",
                    "application/json"
                );
            }
        }
    );


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
    
                int passedTestCases = 0;
                int totalTestCases = 0;

                database.getSubmissionTestCounts(
                    submissionId,
                    userId,
                    passedTestCases,
                    totalTestCases
                );

                std::string errorMessage =
                    database.getSubmissionError(submissionId, userId);

                nlohmann::json response = {
                    {"submission_id", submissionId},
                    {"status", status},
                    {"passed_test_cases", passedTestCases},
                    {"total_test_cases", totalTestCases},
                    {"error_message", errorMessage},
                    {
                        "test_cases",
                        database.getSubmissionTestResults(submissionId, userId)
                    }
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

    // DISCUSSION API ROUTES

    // 1. Get all discussions for a problem
    server.Get(
        R"(/api/problems/(\d+)/discussions)",
        [&](const httplib::Request& req, httplib::Response& res) {
            try {
                int problemId = std::stoi(req.matches[1].str());

                if (problemId <= 0) {
                    res.status = 400;
                    res.set_content(
                        R"({"error":"Invalid problem ID"})",
                        "application/json"
                    );
                    return;
                }

                std::string discussions =
                    database.getDiscussions(problemId);

                res.set_content(discussions, "application/json");
            }
            catch (const std::exception& e) {
                std::cerr << "Get discussions error: "
                        << e.what() << std::endl;

                res.status = 500;
                res.set_content(
                    R"({"error":"Failed to retrieve discussions"})",
                    "application/json"
                );
            }
        }
    );


    // 2. Create a discussion for a problem
    server.Post(
        R"(/api/problems/(\d+)/discussions)",
        [&](const httplib::Request& req, httplib::Response& res) {
            try {
                int userId = 0;

                if (!getUserIdFromSession(req, userId)) {
                    res.status = 401;
                    res.set_content(
                        R"({"error":"Authentication required"})",
                        "application/json"
                    );
                    return;
                }

                int problemId = std::stoi(req.matches[1].str());

                if (problemId <= 0) {
                    res.status = 400;
                    res.set_content(
                        R"({"error":"Invalid problem ID"})",
                        "application/json"
                    );
                    return;
                }

                auto body = nlohmann::json::parse(req.body);

                if (!body.is_object() ||
                    !body.contains("title") ||
                    !body.contains("content") ||
                    !body["title"].is_string() ||
                    !body["content"].is_string()) {

                    res.status = 400;
                    res.set_content(
                        R"({"error":"Title and content are required"})",
                        "application/json"
                    );
                    return;
                }

                std::string title = body["title"].get<std::string>();
                std::string content = body["content"].get<std::string>();

                if (title.empty() || content.empty() ||
                    title.size() > 200) {

                    res.status = 400;
                    res.set_content(
                        R"({"error":"Title must be 1-200 characters and content cannot be empty"})",
                        "application/json"
                    );
                    return;
                }

                int discussionId = database.createDiscussion(
                    userId,
                    problemId,
                    title,
                    content
                );

                if (discussionId <= 0) {
                    res.status = 500;
                    res.set_content(
                        R"({"error":"Failed to create discussion"})",
                        "application/json"
                    );
                    return;
                }

                nlohmann::json response = {
                    {"message", "Discussion created successfully"},
                    {"discussion_id", discussionId}
                };

                res.status = 201;
                res.set_content(response.dump(), "application/json");
            }
            catch (const nlohmann::json::exception& e) {
                res.status = 400;
                res.set_content(
                    R"({"error":"Invalid JSON request body"})",
                    "application/json"
                );
            }
            catch (const std::exception& e) {
                std::cerr << "Create discussion error: "
                        << e.what() << std::endl;

                res.status = 500;
                res.set_content(
                    R"({"error":"Failed to create discussion"})",
                    "application/json"
                );
            }
        }
    );


    // 3. Get all replies for a discussion
    server.Get(
        R"(/api/discussions/(\d+)/replies)",
        [&](const httplib::Request& req, httplib::Response& res) {
            try {
                int discussionId = std::stoi(req.matches[1].str());

                if (discussionId <= 0) {
                    res.status = 400;
                    res.set_content(
                        R"({"error":"Invalid discussion ID"})",
                        "application/json"
                    );
                    return;
                }

                std::string replies =
                    database.getDiscussionReplies(discussionId);

                res.set_content(replies, "application/json");
            }
            catch (const std::exception& e) {
                std::cerr << "Get replies error: "
                        << e.what() << std::endl;

                res.status = 500;
                res.set_content(
                    R"({"error":"Failed to retrieve replies"})",
                    "application/json"
                );
            }
        }
    );


    // 4. Create a reply for a discussion
    server.Post(
        R"(/api/discussions/(\d+)/replies)",
        [&](const httplib::Request& req, httplib::Response& res) {
            try {
                int userId = 0;

                if (!getUserIdFromSession(req, userId)) {
                    res.status = 401;
                    res.set_content(
                        R"({"error":"Authentication required"})",
                        "application/json"
                    );
                    return;
                }

                int discussionId = std::stoi(req.matches[1].str());

                if (discussionId <= 0) {
                    res.status = 400;
                    res.set_content(
                        R"({"error":"Invalid discussion ID"})",
                        "application/json"
                    );
                    return;
                }

                auto body = nlohmann::json::parse(req.body);

                if (!body.is_object() ||
                    !body.contains("content") ||
                    !body["content"].is_string()) {

                    res.status = 400;
                    res.set_content(
                        R"({"error":"Reply content is required"})",
                        "application/json"
                    );
                    return;
                }

                std::string content = body["content"].get<std::string>();

                if (content.empty()) {
                    res.status = 400;
                    res.set_content(
                        R"({"error":"Reply content cannot be empty"})",
                        "application/json"
                    );
                    return;
                }

                int replyId = database.createDiscussionReply(
                    userId,
                    discussionId,
                    content
                );

                if (replyId <= 0) {
                    res.status = 500;
                    res.set_content(
                        R"({"error":"Failed to create reply"})",
                        "application/json"
                    );
                    return;
                }

                nlohmann::json response = {
                    {"message", "Reply created successfully"},
                    {"reply_id", replyId}
                };

                res.status = 201;
                res.set_content(response.dump(), "application/json");
            }
            catch (const nlohmann::json::exception& e) {
                res.status = 400;
                res.set_content(
                    R"({"error":"Invalid JSON request body"})",
                    "application/json"
                );
            }
            catch (const std::exception& e) {
                std::cerr << "Create reply error: "
                        << e.what() << std::endl;

                res.status = 500;
                res.set_content(
                    R"({"error":"Failed to create reply"})",
                    "application/json"
                );
            }
        }
    );

    // Update a discussion
    server.Put(
        R"(/api/discussions/(\d+))",
        [&](const httplib::Request& req, httplib::Response& res) {
            try {
                int userId = 0;

                if (!getUserIdFromSession(req, userId)) {
                    res.status = 401;
                    res.set_content(
                        R"({"error":"Authentication required"})",
                        "application/json"
                    );
                    return;
                }

                int discussionId = std::stoi(req.matches[1].str());

                if (discussionId <= 0) {
                    res.status = 400;
                    res.set_content(
                        R"({"error":"Invalid discussion ID"})",
                        "application/json"
                    );
                    return;
                }

                auto body = nlohmann::json::parse(req.body);

                if (!body.is_object() ||
                    !body.contains("title") ||
                    !body.contains("content") ||
                    !body["title"].is_string() ||
                    !body["content"].is_string()) {

                    res.status = 400;
                    res.set_content(
                        R"({"error":"Title and content are required"})",
                        "application/json"
                    );
                    return;
                }

                std::string title = body["title"].get<std::string>();
                std::string content = body["content"].get<std::string>();

                if (title.find_first_not_of(" \t\n\r") == std::string::npos ||
                    content.find_first_not_of(" \t\n\r") == std::string::npos) {

                    res.status = 400;
                    res.set_content(
                        R"({"error":"Title and content cannot be empty"})",
                        "application/json"
                    );
                    return;
                }

                bool updated = database.updateDiscussion(
                    userId, discussionId, title, content
                );

                if (!updated) {
                    res.status = 404;
                    res.set_content(
                        R"({"error":"Discussion not found or you do not own it"})",
                        "application/json"
                    );
                    return;
                }

                res.set_content(
                    R"({"message":"Discussion updated successfully"})",
                    "application/json"
                );
            }
            catch (const nlohmann::json::exception& e) {
                res.status = 400;
                res.set_content(
                    R"({"error":"Invalid JSON request body"})",
                    "application/json"
                );
            }
            catch (const std::exception& e) {
                std::cerr << "Update discussion route error: "
                        << e.what() << std::endl;

                res.status = 500;
                res.set_content(
                    R"({"error":"Failed to update discussion"})",
                    "application/json"
                );
            }
        }
    );

    // Delete a discussion
    server.Delete(
        R"(/api/discussions/(\d+))",
        [&](const httplib::Request& req, httplib::Response& res) {
            try {
                int userId = 0;

                if (!getUserIdFromSession(req, userId)) {
                    res.status = 401;
                    res.set_content(
                        R"({"error":"Authentication required"})",
                        "application/json"
                    );
                    return;
                }

                int discussionId = std::stoi(req.matches[1].str());

                if (discussionId <= 0) {
                    res.status = 400;
                    res.set_content(
                        R"({"error":"Invalid discussion ID"})",
                        "application/json"
                    );
                    return;
                }

                bool deleted = database.deleteDiscussion(
                    userId, discussionId
                );

                if (!deleted) {
                    res.status = 404;
                    res.set_content(
                        R"({"error":"Discussion not found or you do not own it"})",
                        "application/json"
                    );
                    return;
                }

                res.set_content(
                    R"({"message":"Discussion deleted successfully"})",
                    "application/json"
                );
            }
            catch (const std::exception& e) {
                std::cerr << "Delete discussion route error: "
                        << e.what() << std::endl;

                res.status = 500;
                res.set_content(
                    R"({"error":"Failed to delete discussion"})",
                    "application/json"
                );
            }
        }
    );

    // Update a reply
    server.Put(
        R"(/api/discussion-replies/(\d+))",
        [&](const httplib::Request& req, httplib::Response& res) {
            try {
                int userId = 0;

                if (!getUserIdFromSession(req, userId)) {
                    res.status = 401;
                    res.set_content(
                        R"({"error":"Authentication required"})",
                        "application/json"
                    );
                    return;
                }

                int replyId = std::stoi(req.matches[1].str());

                if (replyId <= 0) {
                    res.status = 400;
                    res.set_content(
                        R"({"error":"Invalid reply ID"})",
                        "application/json"
                    );
                    return;
                }

                auto body = nlohmann::json::parse(req.body);

                if (!body.is_object() ||
                    !body.contains("content") ||
                    !body["content"].is_string()) {

                    res.status = 400;
                    res.set_content(
                        R"({"error":"Reply content is required"})",
                        "application/json"
                    );
                    return;
                }

                std::string content = body["content"].get<std::string>();

                if (content.find_first_not_of(" \t\n\r") == std::string::npos) {
                    res.status = 400;
                    res.set_content(
                        R"({"error":"Reply content cannot be empty"})",
                        "application/json"
                    );
                    return;
                }

                bool updated = database.updateDiscussionReply(
                    userId, replyId, content
                );

                if (!updated) {
                    res.status = 404;
                    res.set_content(
                        R"({"error":"Reply not found or you do not own it"})",
                        "application/json"
                    );
                    return;
                }

                res.set_content(
                    R"({"message":"Reply updated successfully"})",
                    "application/json"
                );
            }
            catch (const nlohmann::json::exception& e) {
                res.status = 400;
                res.set_content(
                    R"({"error":"Invalid JSON request body"})",
                    "application/json"
                );
            }
            catch (const std::exception& e) {
                std::cerr << "Update reply route error: "
                        << e.what() << std::endl;

                res.status = 500;
                res.set_content(
                    R"({"error":"Failed to update reply"})",
                    "application/json"
                );
            }
        }
    );

    // Delete a reply
    server.Delete(
        R"(/api/discussion-replies/(\d+))",
        [&](const httplib::Request& req, httplib::Response& res) {
            try {
                int userId = 0;

                if (!getUserIdFromSession(req, userId)) {
                    res.status = 401;
                    res.set_content(
                        R"({"error":"Authentication required"})",
                        "application/json"
                    );
                    return;
                }

                int replyId = std::stoi(req.matches[1].str());

                if (replyId <= 0) {
                    res.status = 400;
                    res.set_content(
                        R"({"error":"Invalid reply ID"})",
                        "application/json"
                    );
                    return;
                }

                bool deleted = database.deleteDiscussionReply(
                    userId, replyId
                );

                if (!deleted) {
                    res.status = 404;
                    res.set_content(
                        R"({"error":"Reply not found or you do not own it"})",
                        "application/json"
                    );
                    return;
                }

                res.set_content(
                    R"({"message":"Reply deleted successfully"})",
                    "application/json"
                );
            }
            catch (const std::exception& e) {
                std::cerr << "Delete reply route error: "
                        << e.what() << std::endl;

                res.status = 500;
                res.set_content(
                    R"({"error":"Failed to delete reply"})",
                    "application/json"
                );
            }
        }
    );

    server.Post("/api/admin/similarity",
        [&](const httplib::Request& req, httplib::Response& res) {
            if (!isAdmin(req, database)) {
                res.status = 403;
                res.set_content(R"({"error":"Admin access required"})",
                                "application/json");
                return;
            }
    
            try {
                auto body = nlohmann::json::parse(req.body);
    
                if (!body.contains("submission_id") ||
                    !body["submission_id"].is_number_integer()) {
                    res.status = 400;
                    res.set_content(
                        R"({"error":"submission_id required"})",
                        "application/json"
                    );
                    return;
                }
    
                int targetId = body["submission_id"].get<int>();
                auto target = database.getAdminSubmissionDetails(targetId);
    
                if (!target.has_value()) {
                    res.status = 404;
                    res.set_content(R"({"error":"Submission not found"})",
                                    "application/json");
                    return;
                }
    
                auto targetTokens =
                    sourceTokens((*target)["code"].get<std::string>());
    
                auto allSources = database.getAllSubmissionSources();
                nlohmann::json matches = nlohmann::json::array();
    
                for (const auto& item : allSources) {
                    int otherId = item["submission_id"].get<int>();

                    // Exclude the selected submission.
                    if (otherId == targetId) continue;

                    // Compare submissions for the same problem only.
                    if (item["problem_id"] != (*target)["problem_id"])
                        continue;

                    // Only compare Accepted submissions.
                    if (item["status"].get<std::string>() != "ACCEPTED")
                        continue;

                    // Exclude admin-authored submissions.
                    if (item["role"].get<std::string>() == "ADMIN")
                        continue;
                
                    // Compare only submissions for the same problem.
                    if (item["problem_id"] != (*target)["problem_id"]) {
                        continue;
                    }
    
                    auto otherTokens =
                        sourceTokens(item["code"].get<std::string>());
    
                    double score = tokenSimilarity(targetTokens, otherTokens);
    
                    if (score >= 0.35) {
                        matches.push_back({
                            {"submission_id", otherId},
                            {"user_id", item["user_id"]},
                            {"similarity_percent", static_cast<int>(score * 100)}
                        });
                    }
                }
    
                std::sort(
                    matches.begin(),
                    matches.end(),
                    [](const nlohmann::json& a, const nlohmann::json& b) {
                        return a["similarity_percent"].get<int>() >
                               b["similarity_percent"].get<int>();
                    }
                );
    
                res.set_content(
                    nlohmann::json{
                        {"submission_id", targetId},
                        {"matches", matches},
                        {"warning",
                         "Similarity is a heuristic signal, not proof of plagiarism."}
                    }.dump(),
                    "application/json"
                );
            } catch (...) {
                res.status = 400;
                res.set_content(R"({"error":"Similarity analysis failed"})",
                                "application/json");
            }
        }
    );

    server.set_mount_point("/", "../frontend");

    std::cout << "GrowCode server starting on http://localhost:8080" << std::endl;
        
    server.listen("0.0.0.0", 8080);

    return 0;
}
