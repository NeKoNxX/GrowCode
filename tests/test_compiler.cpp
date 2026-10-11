 #include "../execution-engine/compiler/Compiler.h"

#include <iostream>
#include <fstream>

int main() {

    // Define paths for testing
    std::string sourceFile = "tests/test_program.cpp";
    std::string executableFile = "tests/test_program";
    std::string errorFile = "tests/compiler_error.txt";

    // Create a dummy C++ source file for testing.
    {
        std::ofstream source(sourceFile);
     
        source << R"(
#include <iostream>

int main() {
    std::cout << "Hello from compiler test";
    return 0;
}
)";
    }

    // Compile the test program.
    bool result = Compiler::compile(sourceFile,executableFile,errorFile);

    // Evaluate the result
    if (result) {
        std::cout << "Compilation successful" << std::endl;
    }
    else {
        std::cout << "Compilation failed" << std::endl;
    }

    return 0;
}
