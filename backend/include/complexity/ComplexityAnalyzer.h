#ifndef COMPLEXITY_ANALYZER_H
#define COMPLEXITY_ANALYZER_H
#include <string>
#include <nlohmann/json.hpp>
class ComplexityAnalyzer {
public:
    static nlohmann::json analyze(const std::string& code, const std::string& language,
                                  const std::string& problemStatement);
};
#endif
