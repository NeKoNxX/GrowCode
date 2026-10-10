cmake_minimum_required(VERSION 3.20)

project(GrowCode)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(growcode

    # Server
    backend/src/server/main.cpp
    
    # Database
    backend/src/database/Database.cpp

    # Authentication
    backend/src/auth/AuthManager.cpp

    # Execution Engine - Scheduler
    execution_engine/scheduler/Scheduler.cpp

    # Execution Engine - Compiler
    execution_engine/compiler/Compiler.cpp

    # Execution Engine - Process Manager
    execution_engine/process_manager/ProcessManager.cpp

    # Execution Engine - Execution Worker
    execution_engine/executor/ExecutionWorker.cpp

    # Execution Engine - Worker Pool
    execution_engine/workers/WorkerPool.cpp
)

target_include_directories(growcode PRIVATE

    # Project root
    ${CMAKE_CURRENT_SOURCE_DIR}

    # Backend headers
    ${CMAKE_CURRENT_SOURCE_DIR}/backend/include

    # MySQL Connector/C++
    /opt/homebrew/opt/mysql-connector-c++/include

    # nlohmann/json
    /opt/homebrew/opt/nlohmann-json/include

    # cpp-httplib
    /opt/homebrew/opt/cpp-httplib/include

    # OpenSSL
    /opt/homebrew/opt/openssl@3/include
)

target_link_libraries(growcode PRIVATE

    # MySQL Connector/C++
    /opt/homebrew/opt/mysql-connector-c++/lib/libmysqlcppconnx.dylib

    # OpenSSL crypto library
    /opt/homebrew/opt/openssl@3/lib/libcrypto.dylib
)
