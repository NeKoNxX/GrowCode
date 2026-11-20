# GrowCode
### A Multi-User Resource-Aware Code Execution and Management Platform

GrowCode is a locally hosted, multi-user coding platform with an integrated online judge and administrative control center. Users can browse programming problems, write solutions in supported programming languages, submit code, and receive automated verdicts after compilation and execution against predefined test cases.

The platform combines **Operating System (OS)** and **Database Management System (DBMS)** concepts with **Docker-based sandboxed execution**. Its C++ backend manages submission processing, scheduling, worker threads, database operations, and isolated execution environments. MySQL provides persistent storage, while the administrative dashboard supports platform management, submission inspection, heuristic code-similarity analysis, and AI-assisted code review.

---

## Table of Contents

- [Objectives](#objectives)
- [Team Members](#team-members)
- [Technology Stack](#technology-stack)
- [System Architecture](#system-architecture)
- [Major Modules and Features](#major-modules-and-features)
- [Docker Sandbox Architecture](#sandbox-execution-flow)
- [Operating System Concepts](#operating-system-concepts)
- [Database Design and DBMS Concepts](#database-design-and-dbms-concepts)
- [Installation and Setup](#installation-and-setup)
- [Running GrowCode](#running-growcode)
- [Current Project Status](#current-project-status)
- [Repository Structure](#repository-structure)

---

## Objectives

- Develop a locally hosted, multi-user coding platform.
- Implement automated code compilation and evaluation.
- Execute submissions inside Docker containers.
- Manage users, problems, test cases, submissions, execution results, and user progress using MySQL.
- Demonstrate OS concepts such as process management, scheduling, multithreading, synchronization, IPC, and resource management.
- Demonstrate DBMS concepts such as relational data modelling, keys, constraints, normalization, SQL queries, and transactions.
- Provide submission history and user progress tracking.
- Analyze submitted code for estimated time and space complexity.
- Display expected complexity and recommended algorithms configured for individual problems.
- Provide an administrative dashboard for managing users, problems, test cases, and submissions.
- Support heuristic similarity analysis and AI-assisted code review for administrative inspection.

---

## Team Members

| S. No. | Name |
|---|---|
| 1 | Mayank Chhetri |
| 2 | Ashutosh Upreti |
| 3 | Anuj Negi |

---

## Technology Stack

| Component | Technology |
|---|---|
| Backend | C++ |
| Frontend | HTML, CSS, JavaScript |
| Database | MySQL |
| Database Connectivity | MySQL Connector/C++ (X DevAPI) |
| HTTP Server | cpp-httplib |
| JSON Processing | nlohmann/json |
| Build System | CMake |
| Compiler | G++ |
| Containerization | Docker |
| Version Control | Git and GitHub |
| Development Environment | Visual Studio Code |

### Sandbox Runtime Environment

The sandbox image is designed to provide the language runtimes needed by submitted programs:

| Runtime | Version / Configuration |
|---|---|
| Base image | Ubuntu 24.04 ARM64 |
| C++ | G++ 13.3 |
| Java | OpenJDK / Javac 21 |
| Python | Python 3.12 |
| Container user | Non-root `runner` (UID 1001) |
| Image tag | `growcode-sandbox:latest` |

These are the configured image details from the project implementation. Confirm the current Dockerfile and built image before relying on them for a deployment on a different architecture.

---

## System Architecture

GrowCode follows a client-server architecture. Multiple users access the frontend, which communicates with a C++ backend through HTTP APIs. The backend coordinates database operations and submission execution.

```text
                 Multiple Users
                       |
                       v
             Frontend (HTML/CSS/JS)
                       |
                       v
                   HTTP APIs
                       |
                       v
                 C++ Backend
                       |
              +--------+---------+
              |                  |
              v                  v
        MySQL Database     Scheduler / Job Queue
                                 |
                                 v
                             Worker Pool
                                 |
                                 v
                           SandboxManager
                                 |
                                 v
                         Docker CLI / Engine
                                 |
                                 v
                         Docker Sandbox
                       (C++ / Java / Python)
                                 |
                                 v
                       Compile and Execute
                                 |
                                 v
                        Test Case Evaluation
                                 |
                                 v
                         Verdict Generation
                                 |
                                 v
                       Persist Submission Result
                                 |
                                 v
                            Frontend

                   Administrative Dashboard
                              |
             +----------------+----------------+
             |                |                |
             v                v                v
       User/Problem       Submission       Code Review /
        Management         Inspection       Similarity
```

---

## Major Modules and Features

### 1. User Management and Authentication

- User registration, login, and logout.
- Session management.
- Role-based access for regular users and administrators.
- Protected administrative APIs and pages.
- User-specific submission history.
- Administrative user search and account management.
- Session-aware navigation that displays Login/Register or Logout as appropriate and exposes the Admin link only to administrators.

Student authentication uses a Student ID and password.

### 2. Problem Management

- Browse programming problems.
- View problem descriptions, constraints, sample inputs, and expected outputs.
- Admin problem creation and management.
- Test-case management, including sample and hidden test cases.
- Configure expected time and space complexity.
- Configure recommended algorithms.
- Search, filter, and sort problems in the administrative dashboard.

### 3. Code Submission

- Browser-based code editor.
- Programming language selection.
- Code submission through HTTP APIs.
- Submission status tracking.
- Submission history.
- User progress tracking.

### 4. Online Judge

The integrated online judge automates evaluation of submitted programs.

**Execution workflow:**

1. Receive source code through the backend API.
2. Create or prepare an execution workspace.
3. Compile the submitted program using the selected language runtime.
4. Execute it against predefined test cases in the configured sandbox.
5. Capture program output and execution information.
6. Compare actual output with expected output.
7. Generate the corresponding verdict.
8. Store execution results in MySQL.
9. Return the result to the frontend.

#### Verdicts

| Verdict | Description |
|---|---|
| `ACCEPTED` | All test cases passed. |
| `WRONG_ANSWER` | Output did not match the expected result. |
| `COMPILATION_ERROR` | Source code compilation failed. |
| `RUNTIME_ERROR` | Program terminated abnormally. |
| `TIME_LIMIT_EXCEEDED` | Program exceeded its execution time limit. |

### 5. Docker-Based Sandbox Execution

Docker is a core part of GrowCode's code-execution architecture. It provides a containerized runtime environment for executing user-submitted programs separately from the main application process.

The backend's sandbox-management component prepares and invokes Docker containers for program execution. The intended execution environment includes:

- A dedicated sandbox image: `growcode-sandbox:latest`.
- Ubuntu 24.04 ARM64 base image.
- G++ 13.3 for C++ compilation and execution.
- OpenJDK / Javac 21 for Java.
- Python 3.12 for Python.
- A non-root container user named `runner` (UID 1001).
- A per-execution workspace mounted at `/workspace`.
- Disabled container networking using `--network none`.
- Container resource restrictions and cleanup.

#### Sandbox Execution Flow

```text
Submission Received
        |
        v
Backend Scheduler
        |
        v
Worker Pool
        |
        v
SandboxManager
        |
        v
Prepare Execution Workspace
        |
        v
Start Docker Container
        |
        v
Compile Source Code
        |
        v
Execute Against Test Cases
        |
        v
Capture Output / Exit Status / Resource Data
        |
        v
Stop and Clean Up Container
        |
        v
Return Result to Online Judge
```

#### Docker Configuration and Implementation References

| Item | Project location / configuration |
|---|---|
| Sandbox Dockerfile | `docker/sandbox/Dockerfile` |
| Sandbox manager | `backend/src/sandbox/SandboxManager.cpp` |
| Sandbox image | `growcode-sandbox:latest` |
| Container workspace | `/workspace` |
| Container user | `runner` |
| Network mode | `none` |
| Container invocation | Docker CLI launched by the backend |

The current implementation uses the backend's process-management facilities to invoke Docker. The exact Docker command, mounts, resource flags, and cleanup behavior should be checked against `SandboxManager.cpp` and the Dockerfile when maintaining or deploying the project.

### 6. OS-Based Execution and Process Management

The execution and scheduling components demonstrate core Operating System concepts.

| OS Concept | API / Mechanism |
|---|---|
| Process creation | `fork()` |
| Program execution | `exec()` / `execvp()` |
| Process monitoring | `wait()` / `wait4()` |
| Inter-Process Communication | `pipe()` |
| File descriptor redirection | `dup2()` |
| Resource limit configuration | `setrlimit()` |
| Process termination | `kill()` |

The backend uses process-management mechanisms to manage execution tasks and invoke external programs such as Docker. Docker provides the container boundary for submitted code; process APIs alone are not equivalent to a secure sandbox.

### 7. Scheduling and Concurrency

- Job queue for submission processing.
- Scheduler for managing execution jobs.
- Worker pool for concurrent task processing.
- Worker threads.
- Mutexes for shared-resource synchronization.
- Condition variables for coordinating worker activity.
- Concurrent processing of multiple submissions.

### 8. Database Management

MySQL provides persistent storage and structured data management.

**Data managed:**

- User and authentication data.
- Programming problems.
- Test cases.
- Code submissions.
- Execution results.
- User progress.
- Expected complexity and recommended algorithm information.
- Administrative activity records, where configured.

**DBMS concepts demonstrated:**

- Primary keys and foreign keys.
- Referential integrity and constraints.
- Database normalization.
- Indexing.
- SQL queries and transactions.
- Relationships between application entities.
- Concurrent database operations.

### 9. Complexity Analysis

- Analyze submissions for estimated time complexity.
- Estimate space complexity.
- Compare estimated complexity against expected complexity configured for the problem.
- Display a complexity comparison chart.
- Retrieve recommended algorithms associated with individual problems from MySQL.
- Display explanations and confidence information when returned by the analysis service.

Complexity estimates are analytical predictions, not formal proofs of asymptotic complexity. Their availability and accuracy depend on the configured analysis service and database records.

### 10. Administrative Control Center

The admin dashboard provides a centralized interface for platform administration.

- Dashboard statistics for total problems and difficulty distribution.
- Problem creation and management.
- Sample and hidden test-case management.
- User search and management.
- Viewing submissions associated with selected users.
- Inspecting submitted source code.
- Administrative activity log.
- Access restricted to authenticated administrators.

### 11. Code Review and Similarity Analysis

The admin dashboard includes tools to inspect submitted code.

#### Similarity Analysis

- Compare a selected submission with other submissions for the same problem.
- Exclude the selected submission itself from candidate comparisons.
- Exclude submissions authored by users whose role is `ADMIN` from candidate comparisons.
- Token-based source analysis intended to reduce the influence of common C++ boilerplate.
- Display similarity scores and ranked matches.
- Show selected and matched source code side by side.
- Highlight matching source lines when exact or normalized line matches are found.

The checker is heuristic. A similarity score is not proof of plagiarism. Boilerplate filtering and token normalization can reduce some false positives, but the checker must be tested against both identical and unrelated solutions. The configured score threshold should be calibrated using representative test data.

#### AI-Assisted Code Review

- Request an AI-assisted review of a selected submission.
- Display review feedback in the admin dashboard.

AI review is advisory and must not be treated as proof of AI authorship or as a substitute for human review.

---

## Operating System Concepts

GrowCode demonstrates practical OS concepts through its scheduler, worker pool, process management, and containerized execution workflow.

- **Process management:** create, execute, monitor, and terminate processes.
- **CPU scheduling:** coordinate queued execution jobs.
- **Multithreading:** process multiple jobs through worker threads.
- **Synchronization:** protect shared state using mutexes and condition variables.
- **Inter-Process Communication:** use pipes and redirected file descriptors where required.
- **Resource management:** configure execution limits and collect resource information.
- **Containerization:** run submitted programs inside Docker containers with configured restrictions.

---

## Database Design and DBMS Concepts

The MySQL database stores the platform's persistent application data.

The project demonstrates:

- Relational data modelling.
- Primary keys and foreign keys.
- Referential integrity.
- Constraints and normalization.
- SQL queries and joins.
- Indexing.
- Transactions and concurrent database access.
- Persistent storage of submissions and execution results.

---

## Installation and Setup

The following instructions describe the local development setup. Paths and configuration should be checked against the current repository before running commands.

### Prerequisites

- macOS or a compatible Linux environment.
- C++ compiler with C++17 support.
- CMake.
- MySQL Server.
- MySQL Connector/C++.
- Docker Desktop or Docker Engine.
- Git.

The sandbox image details above target ARM64. On another CPU architecture, verify the Docker build platform and image compatibility.

### 1. Clone the Repository

```bash
git clone https://github.com/ashutosh691/GrowCode.git
cd GrowCode
```

### 2. Start MySQL

For macOS users with MySQL installed through Homebrew:

```bash
brew services start mysql
```

### 3. Start Docker

Open Docker Desktop on macOS, or start the Docker service on Linux.

Verify Docker is available:

```bash
docker version
docker info
```

### 4. Initialize the Database

Log in to MySQL:

```bash
mysql -u root -p
```

Then execute the initialization script from the project root:

```sql
source database/queries.sql;
```

If the SQL file is located elsewhere, use its actual path.

### 5. Configure Database Connectivity

Configure the database connection parameters according to your local MySQL installation:

- Host
- Port
- Username
- Password
- Database name

**Security:** Do not commit database passwords, API keys, or other credentials. Prefer environment variables or a local configuration file excluded from version control. Review source files for hard-coded credentials before publishing the repository.

### 6. Build the Docker Sandbox Image

From the repository root, if the Dockerfile is located at `docker/sandbox/Dockerfile`:

```bash
docker build -t growcode-sandbox:latest -f docker/sandbox/Dockerfile .
```

Confirm that the image exists:

```bash
docker images
```

If the Dockerfile expects a different build context, use the context specified by the project's Docker configuration.

### 7. Build the Backend

Use the CMake configuration present in the repository. For a backend with its own CMake project:

```bash
cd backend
cmake -S . -B build
cmake --build build -j$(sysctl -n hw.ncpu)
```

If the project uses a different build directory or top-level CMake configuration, follow the repository's current build setup.

---

## Running GrowCode

1. Ensure MySQL is running.
2. Ensure Docker is running and the `growcode-sandbox:latest` image is available.
3. Start the backend executable produced by the CMake build.

In the current local development workflow, the server has been launched from the backend build directory using the `growcode` executable. Confirm the executable path against the latest build output.

The application is available at:

**http://localhost:8080**

Open this URL in a browser to access GrowCode.

For access from other devices on the same local network, configure the server to listen on the appropriate network interface and use the host computer's LAN IP address. Ensure firewall rules permit the required connection.

---

## Current Project Status

The following areas have implementation work in the project. Their presence does not imply that every edge case has passed regression testing.

### Implemented Development Areas

- [x] Project structure and requirements definition.
- [x] System architecture and workflow design.
- [x] MySQL database schema and initialization script.
- [x] C++ database connectivity.
- [x] Submission and job management.
- [x] OS-based process execution.
- [x] Scheduler and worker pool.
- [x] Docker sandbox integration.
- [x] Online judge integration.
- [x] Backend API development.
- [x] Authentication and session management.
- [x] Role-based administrative access.
- [x] Frontend implementation.
- [x] Submission history and user progress tracking.
- [x] Complexity-analysis integration.
- [x] Expected complexity and recommended algorithm retrieval.
- [x] Administrative dashboard.
- [x] Problem and test-case management interface.
- [x] User management and submission inspection interface.
- [x] Administrative activity log interface.
- [x] Similarity-analysis interface and backend integration.
- [x] Source-line comparison and highlighting interface.
- [x] AI code-review interface and integration.


#### Testing Checklist

- [x] Clean build and application startup.
- [x] MySQL connectivity and persistence.
- [x] Docker daemon availability and sandbox image build.
- [x] Docker container startup and cleanup.
- [x] C++ compilation and execution inside the sandbox.
- [x] Java compilation and execution inside the sandbox.
- [x] Python execution inside the sandbox.
- [x] Container non-root user and filesystem permissions.
- [x] CPU, memory, process, and execution-time limits.
- [x] Registration, login, session persistence, and logout.
- [x] Role-based access control for USER and ADMIN.
- [x] Problem browsing, filtering, and administration.
- [x] Sample and hidden test-case behavior.
- [x] Accepted and Wrong Answer submissions.
- [x] Compilation Error and Runtime Error handling.
- [x] Time Limit Exceeded and process termination.
- [x] Concurrent submissions and worker-pool behavior.
- [x] Process cleanup and execution-workspace isolation.
- [x] Submission history and progress consistency.
- [x] Admin user management and audit logging.
- [x] Similarity candidate filtering and score validation.
- [x] Exact and normalized source-line highlighting.
- [x] AI code-review error handling.
- [x] Unauthorized API access and security checks.
- [x] Full end-to-end regression testing.

---

## Repository Structure

The following is a high-level view of the relevant project components. Exact filenames and directories may vary with the current repository revision.

```text
GrowCode/
├── backend/
│   ├── include/
│   ├── src/
│   │   ├── database/
│   │   ├── sandbox/
│   │   │   └── SandboxManager.cpp
│   │   └── server/
│   └── CMakeLists.txt
├── docker/
│   └── sandbox/
│       └── Dockerfile
├── database/
│   └── queries.sql
├── frontend/
│   ├── css/
│   ├── js/
│   └── ...
└── README.md
```

---

## Repository

**GitHub:** https://github.com/ashutosh691/GrowCode

---

*GrowCode — Bringing Operating Systems, Database Management, Docker, and Competitive Programming Together.*
