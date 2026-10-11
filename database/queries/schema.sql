create database if not exists growcode_app;

use growcode_app;


/*
this is users table schema
this table stores user information such as name, username, email, password and role
username and email are required to be unique and not null
password_hash will store the hashed version of the user's password for safer authentication
role is used to identify whether the user is a normal user or an admin
*/
create table users (
    user_id int auto_increment primary key,
    name varchar(100) not null,
    username varchar(50) not null unique,
    email varchar(100) not null unique,
    password_hash varchar(255) not null,
    role enum('user', 'admin') default 'user',
    created_at timestamp default current_timestamp
);


/*
this is languages table schema
this table stores the programming languages supported by the system
extension stores the file extension associated with the language
compile_command stores the command used to compile the source code
run_command stores the command used to execute the compiled program
*/
create table languages (
    language_id int auto_increment primary key,
    name varchar(50) not null unique,
    extension varchar(20) not null,
    compile_command text,
    run_command text
);


/*
this is problems table schema
this table stores the programming problems available on the platform
title and description contain the problem information
difficulty determines whether the problem is easy, medium or hard
created_by stores the user_id of the admin/user who created the problem
foreign key is used here because every problem can be associated with a user
if the user who created the problem is deleted, created_by will be set to null
*/
create table problems (
    problem_id int auto_increment primary key,
    title varchar(200) not null,
    description longtext not null,
    difficulty enum('easy', 'medium', 'hard') not null,
    constraints text,
    created_by int,
    created_at timestamp default current_timestamp,
    foreign key (created_by) references users(user_id) on delete set null
);


/*
this is test cases table
this table stores test cases related to a particular problem
problem_id is a foreign key because every test case belongs to a problem
input contains the input given to the submitted program
expected_output contains the correct output for that input
is_sample determines whether the test case should be shown as a sample
order_no is used to maintain the order of test cases
delete cascade is used so that when a problem is deleted, all test cases
related to that problem are also automatically deleted
*/
create table test_cases (
    test_case_id int auto_increment primary key,
    problem_id int not null,
    input longtext,
    expected_output longtext not null,
    is_sample boolean default false,
    order_no int not null,
    foreign key (problem_id) references problems(problem_id) on delete cascade
);


/*
this is submissions table
this table stores the source code submitted by users for different problems
three foreign keys are used here
1. user_id is related to the users table
2. problem_id is related to the problems table
3. language_id is related to the languages table
status stores the current state of the submitted code
it can be pending, running, accepted, wrong answer, compilation error,
runtime error or time limit exceeded
*/
create table submissions (
    submission_id int auto_increment primary key,
    user_id int not null,
    problem_id int not null,
    language_id int not null,
    code longtext not null,
    status enum('pending', 'running', 'accepted', 'wrong_answer', 'compilation_error', 'runtime_error', 'time_limit_exceeded') default 'pending',
    submitted_at timestamp default current_timestamp,
    foreign key (user_id) references users(user_id) on delete cascade,
    foreign key (problem_id) references problems(problem_id) on delete cascade,
    foreign key (language_id) references languages(language_id)
);


/*
this is jobs table
this table is used to manage code execution jobs
each job is associated with exactly one submission
status shows whether the job is queued, running, completed or failed
priority is used to determine which job should be processed first
created_at stores when the job was created
picked_at stores when the execution worker picked the job
finished_at stores when execution was completed
*/
create table jobs (
    job_id int auto_increment primary key,
    submission_id int not null unique,
    status enum( 'queued', 'running', 'completed', 'failed') default 'queued',
    priority int default 0,
    created_at timestamp default current_timestamp,
    picked_at timestamp null,
    finished_at timestamp null,
    foreign key (submission_id) references submissions(submission_id) on delete cascade
);


/*
this is execution results table
this table stores the result of executing a submission against a test case
submission_id identifies which submission was executed
test_case_id identifies which test case was used
status stores the result of the execution
actual_output stores the output produced by the submitted program
time_taken stores the execution time
memory_used stores the amount of memory used during execution
*/
create table execution_results (
    result_id int auto_increment primary key,
    submission_id int not null,
    test_case_id int not null,
    status enum('accepted', 'wrong_answer', 'runtime_error', 'time_limit_exceeded') not null,
    actual_output longtext,
    time_taken double,
    memory_used bigint,
    foreign key (submission_id) references submissions(submission_id) on delete cascade,
    foreign key (test_case_id) references test_cases(test_case_id) on delete cascade
);


/*
this is user progress table
this table determines which problems a user has solved
the combination of user_id and problem_id is required to be unique
so one user can have only one progress record for one problem
is_solved determines whether the problem has been solved
first_solved_at stores the time when the problem was solved for the first time
attempts stores the number of submissions/attempts made by the user
*/
create table user_progress (
    progress_id int auto_increment primary key,
    user_id int not null,
    problem_id int not null,
    is_solved boolean default false,
    first_solved_at timestamp null,
    attempts int default 0,
    unique (user_id, problem_id),
    foreign key (user_id) references users(user_id) on delete cascade,
    foreign key (problem_id) references problems(problem_id) on delete cascade
);


/*
this is tags table
this table stores tags which can be assigned to programming problems
tag name is required to be unique so duplicate tags cannot be created
*/
create table tags (
    tag_id int auto_increment primary key,
    name varchar(50) not null unique
);


/*
this is problem_tags table
this is a junction table used to create a many-to-many relationship
between problems and tags
one problem can have multiple tags
one tag can be assigned to multiple problems
the combination of problem_id and tag_id is used as the primary key
so the same tag cannot be assigned to the same problem more than once
*/
create table problem_tags (
    problem_id int not null, tag_id int not null,
    primary key (problem_id, tag_id),
    foreign key (problem_id) references problems(problem_id) on delete cascade,
    foreign key (tag_id) references tags(tag_id) on delete cascade
);


/*
this index is created on problem_id in test_cases
it improves the speed of queries that search for test cases
belonging to a particular problem
*/
create index idx_test_cases_problem_id
on test_cases(problem_id);


/*
this index is created on user_id in submissions
it improves the speed of queries that retrieve submissions
made by a particular user
*/
create index idx_submissions_user_id
on submissions(user_id);


/*
this index is created on problem_id in submissions
it improves the speed of queries that retrieve submissions
for a particular problem
*/
create index idx_submissions_problem_id
on submissions(problem_id);


/*
this index is created on submitted_at in submissions
it improves the speed of queries that sort or filter submissions
based on their submission time
*/
create index idx_submissions_submitted_at
on submissions(submitted_at);


/*
this is a composite index on jobs status and priority
it improves the speed of finding jobs based on their current status
and priority
*/
create index idx_jobs_status_priority
on jobs(status, priority);


/*
this index is created on submission_id in execution_results
it improves the speed of retrieving all execution results
belonging to a particular submission
*/
create index idx_execution_results_submission_id
on execution_results(submission_id);


/*
this index is created on user_id in user_progress
it improves the speed of retrieving the complete progress
of a particular user
*/
create index idx_user_progress_user_id
on user_progress(user_id);
