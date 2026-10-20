
#ifndef COMPILER_H
#define COMPILER_H

#include <string>

class Compiler {
public:
    // Existing C++ compilation method — preserve compatibility.
    static bool compile(
        const std::string& sourceFile,
        const std::string& executableFile,
        const std::string& errorFile
    );

    // Compile a Java source file.
    static bool compileJava(
        const std::string& sourceFile,
        const std::string& outputDirectory,
        const std::string& errorFile
    );
};

#endif
