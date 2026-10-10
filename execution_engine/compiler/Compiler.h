#ifndef COMPILER_H
#define COMPILER_H

#include <string>

// Compiler is responsible for compiling
// the source code submitted by the user
class Compiler {
public:

    // function to compile the submitted source code
    //
    // sourceFile:
    // path of the submitted C++ source file
    //
    // executableFile:
    // path where the compiled executable will be created
    //
    // errorFile:
    // path where compiler error messages will be stored
    //
    // returns true if compilation is successful
    // returns false if compilation fails
    static bool compile(
        const std::string& sourceFile,
        const std::string& executableFile,
        const std::string& errorFile
    );
};

#endif
