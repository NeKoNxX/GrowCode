// export GEMINI_API_KEY="YOUR_API"
#include "complexity/ComplexityAnalyzer.h"
#include <cstdlib>
#include <stdexcept>
#include <string>
#include "httplib.h"

nlohmann::json ComplexityAnalyzer::analyze(const std::string& code,
                                           const std::string& language,
                                           const std::string& problemStatement) {
    const char* key = std::getenv("GEMINI_API_KEY");
    if (key == nullptr || *key == '\0') throw std::runtime_error("GEMINI_API_KEY is not configured");
    if (code.empty() || code.size() > 30000) throw std::runtime_error("Code is empty or exceeds the 30000-byte analysis limit");

    httplib::SSLClient client("generativelanguage.googleapis.com", 443);
    client.set_connection_timeout(5, 0);
    client.set_read_timeout(20, 0);
    client.set_write_timeout(5, 0);

    const std::string prompt =
        "Act as a careful algorithm-analysis tutor. Estimate asymptotic complexity of the submitted program. "
        "Do not execute the code. Distinguish worst-case time complexity from auxiliary space complexity. "
        "Account for loops, recursion, data structures, and input size. If code or constraints are insufficient, say so. "
        "Return ONLY valid JSON with keys: time_complexity, space_complexity, explanation, confidence. "
        "Complexity fields must use Big-O notation. confidence must be Low, Medium, or High. This is an estimate, not a proof.\nLanguage: " + language +
        "\nProblem statement and constraints:\n" + problemStatement + "\nSubmitted source code:\n" + code;

    nlohmann::json request = {
        {"contents", nlohmann::json::array({{{"parts", nlohmann::json::array({{{"text", prompt}}})}}})},
        {"generationConfig", {{"responseMimeType", "application/json"}, {"temperature", 0.1}, {"maxOutputTokens", 400}}}
    };
    const std::string path = "/v1beta/models/gemini-3.1-flash-lite:generateContent?key=" + std::string(key);
    auto response = client.Post(path.c_str(), request.dump(), "application/json");
    if (!response) throw std::runtime_error("Gemini API could not be reached");
    if (response->status < 200 || response->status >= 300)
        throw std::runtime_error("Gemini API returned HTTP " + std::to_string(response->status));

    auto envelope = nlohmann::json::parse(response->body);
    if (!envelope.contains("candidates") || envelope["candidates"].empty() || !envelope["candidates"][0].contains("content"))
        throw std::runtime_error("Gemini returned no analysis content");
    std::string output;
    for (const auto& part : envelope["candidates"][0]["content"].value("parts", nlohmann::json::array()))
        if (part.contains("text") && part["text"].is_string()) output += part["text"].get<std::string>();
    auto result = nlohmann::json::parse(output);
    for (const char* field : {"time_complexity", "space_complexity", "explanation", "confidence"})
        if (!result.contains(field) || !result[field].is_string()) throw std::runtime_error("Gemini response is missing a required field");
    result["estimated"] = true;
    return result;
}
