-- expected complexity metadata for each problem.
-- Run this once against the growcode_app database.
USE growcode_app;

CREATE TABLE IF NOT EXISTS problem_complexity (
    problem_id INT NOT NULL PRIMARY KEY,
    expected_time_complexity VARCHAR(80) NOT NULL DEFAULT 'Not configured',
    expected_space_complexity VARCHAR(80) NOT NULL DEFAULT 'Not configured',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    CONSTRAINT fk_problem_complexity_problem
        FOREIGN KEY (problem_id) REFERENCES problems(problem_id) ON DELETE CASCADE
);

-- Example only: replace problem_id and complexity values to match your actual problems.
-- INSERT INTO problem_complexity (problem_id, expected_time_complexity, expected_space_complexity)
-- VALUES (1, 'O(n)', 'O(1)')
-- ON DUPLICATE KEY UPDATE
--   expected_time_complexity = VALUES(expected_time_complexity),
--   expected_space_complexity = VALUES(expected_space_complexity);



-- Run this ONCE on an existing GrowCode database after problem_complexity exists.
USE growcode_app;
ALTER TABLE problem_complexity
    ADD COLUMN recommended_algorithm VARCHAR(150) NOT NULL DEFAULT 'Not configured';
