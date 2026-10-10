#ifndef COMPILER_H
#define COMPILER_H

#include <string>

// Compiles user source code
class Compiler {
public:

    // Compiles C++ source into an executable and logs any errors.
    // Returns true if successful, false if it fails.
    static bool compile(
        const std::string& sourceFile,
        const std::string& executableFile,
        const std::string& errorFile
    );
};

#endif
