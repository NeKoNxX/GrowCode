-- query for showing test case table properly
SELECT
    test_case_id,
    problem_id,
    REPLACE(REPLACE(`input`, '\r', '\\r'), '\n', '\\n') AS input,
    REPLACE(REPLACE(expected_output, '\r', '\\r'), '\n', '\\n') AS expected_output,
    is_sample,
    order_no
FROM test_cases
ORDER BY problem_id, order_no;

-- query for showing execution results table properly
SELECT
    result_id,
    submission_id,
    test_case_id,
    status,
    REPLACE(REPLACE(actual_output, '\r', '\\r'), '\n', '\\n') AS actual_output,
    time_taken,
    memory_used
FROM execution_results
ORDER BY result_id;