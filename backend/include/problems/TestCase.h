#ifndef TEST_CASE_H
#define TEST_CASE_H

#include <string>

//Represents a single test scenario used to evaluate user submissions
struct TestCase{
    int testCaseId;
    std::string input , expectedOutput;
    bool isSample;
};

#endif
