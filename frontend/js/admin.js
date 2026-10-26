document.addEventListener("DOMContentLoaded", async () => {
    const adminMessage = document.getElementById("admin-message");
    const problemList = document.getElementById("problem-list");
    const testCaseList = document.getElementById("test-case-list");
    const problemIdInput = document.getElementById("problem-id");

    async function apiRequest(url, options = {}) {
        const response = await fetch(url, { credentials: "include", ...options });
        let data = {};
        try { data = await response.json(); } catch (_) {}
        if (!response.ok) throw new Error(data.error || `Request failed (${response.status})`);
        return data;
    }

    try {
        await apiRequest("/api/admin/check", { cache: "no-store" });
        adminMessage.textContent = "Administrator access verified.";
    } catch (error) {
        adminMessage.textContent = "Access denied. Administrator privileges are required.";
        document.querySelectorAll(".admin-section").forEach(section => { section.style.display = "none"; });
        return;
    }

    const problemForm = document.getElementById("problem-form");
    problemForm.addEventListener("submit", async event => {
        event.preventDefault();
        const message = document.getElementById("problem-message");
        message.textContent = "Creating problem...";
        const formData = new URLSearchParams({
            title: document.getElementById("title").value.trim(),
            description: document.getElementById("description").value.trim(),
            difficulty: document.getElementById("difficulty").value,
            constraints: document.getElementById("constraints").value,
            expected_time_complexity: document.getElementById("expected-time-complexity").value.trim(),
            expected_space_complexity: document.getElementById("expected-space-complexity").value.trim(),
            recommended_algorithm: document.getElementById("recommended-algorithm").value.trim()
        });
        try {
            const data = await apiRequest("/api/admin/problems", {
                method: "POST",
                headers: { "Content-Type": "application/x-www-form-urlencoded" },
                body: formData.toString()
            });
            message.textContent = `Problem created successfully. Problem ID: ${data.problem_id}`;
            problemIdInput.value = data.problem_id;
            problemForm.reset();
            await loadProblems();
        } catch (error) {
            message.textContent = error.message || "Unable to create problem.";
        }
    });

    const testCaseForm = document.getElementById("test-case-form");
    testCaseForm.addEventListener("submit", async event => {
        event.preventDefault();
        const message = document.getElementById("test-case-message");
        message.textContent = "Adding test case...";
        const formData = new URLSearchParams({
            problem_id: problemIdInput.value,
            input: document.getElementById("input").value,
            expected_output: document.getElementById("expected-output").value,
            is_sample: document.getElementById("is-sample").value,
            order_no: document.getElementById("order-no").value
        });
        try {
            const data = await apiRequest("/api/admin/test-cases", {
                method: "POST",
                headers: { "Content-Type": "application/x-www-form-urlencoded" },
                body: formData.toString()
            });
            message.textContent = `Test case added successfully. Test Case ID: ${data.test_case_id}`;
            const selectedProblem = Number(problemIdInput.value);
            document.getElementById("input").value = "";
            document.getElementById("expected-output").value = "";
            document.getElementById("order-no").value = "";
            await loadTestCases(selectedProblem);
            await loadProblems();
        } catch (error) {
            message.textContent = error.message || "Unable to add test case.";
        }
    });

    async function loadTestCases(problemId) {
        testCaseList.replaceChildren();
        const loading = document.createElement("p");
        loading.className = "admin-muted";
        loading.textContent = "Loading test cases...";
        testCaseList.appendChild(loading);
        try {
            const data = await apiRequest(`/api/admin/test-cases/${problemId}`);
            testCaseList.replaceChildren();
            if (!data.length) {
                const empty = document.createElement("p");
                empty.className = "admin-muted";
                empty.textContent = "No test cases available for this problem.";
                testCaseList.appendChild(empty);
                return;
            }
            data.forEach(testCase => {
                const card = document.createElement("article");
                card.className = "admin-test-case-card";
                const heading = document.createElement("h3");
                heading.textContent = `Test Case #${testCase.test_case_id}`;
                const inputLabel = document.createElement("p");
                inputLabel.textContent = "Input:";
                const input = document.createElement("pre");
                input.textContent = testCase.input || "(empty)";
                const outputLabel = document.createElement("p");
                outputLabel.textContent = "Expected Output:";
                const output = document.createElement("pre");
                output.textContent = testCase.expected_output ?? "";
                const visibility = document.createElement("p");
                visibility.textContent = `Type: ${testCase.is_sample ? "Sample (visible to users)" : "Hidden test"}`;
                const remove = document.createElement("button");
                remove.type = "button";
                remove.className = "admin-button admin-button-danger";
                remove.textContent = "Delete Test Case";
                remove.addEventListener("click", async () => {
                    if (!window.confirm(`Delete test case #${testCase.test_case_id}? This cannot be undone.`)) return;
                    remove.disabled = true;
                    try {
                        await apiRequest(`/api/admin/test-cases/${testCase.test_case_id}`, { method: "DELETE" });
                        await loadTestCases(problemId);
                        await loadProblems();
                    } catch (error) {
                        window.alert(error.message || "Failed to delete test case.");
                        remove.disabled = false;
                    }
                });
                card.append(heading, visibility, inputLabel, input, outputLabel, output, remove);
                testCaseList.appendChild(card);
            });
        } catch (error) {
            testCaseList.replaceChildren();
            const message = document.createElement("p");
            message.className = "admin-muted";
            message.textContent = error.message || "Unable to load test cases.";
            testCaseList.appendChild(message);
        }
    }

    async function loadProblems() {
        problemList.replaceChildren();
        const loading = document.createElement("p");
        loading.className = "admin-muted";
        loading.textContent = "Loading problems...";
        problemList.appendChild(loading);
        try {
            const data = await apiRequest("/api/admin/problems");
            problemList.replaceChildren();
            if (!data.length) {
                const empty = document.createElement("p");
                empty.className = "admin-muted";
                empty.textContent = "No problems available.";
                problemList.appendChild(empty);
                return;
            }
            data.forEach(problem => {
                const card = document.createElement("article");
                card.className = "admin-problem-card";
                const title = document.createElement("h3");
                title.textContent = `#${problem.problem_id} ${problem.title}`;
                const difficulty = document.createElement("p");
                difficulty.textContent = `Difficulty: ${problem.difficulty}`;
                const description = document.createElement("p");
                description.textContent = problem.description || "No description.";
                const constraints = document.createElement("p");
                constraints.textContent = `Constraints: ${problem.constraints || "None"}`;
                const time = document.createElement("p");
                time.textContent = `Expected time: ${problem.expected_time_complexity || "Not configured"}`;
                const space = document.createElement("p");
                space.textContent = `Expected space: ${problem.expected_space_complexity || "Not configured"}`;
                const algorithm = document.createElement("p");
                algorithm.textContent = `Recommended algorithm: ${problem.recommended_algorithm || "Not configured"}`;
                const select = document.createElement("button");
                select.type = "button";
                select.className = "select-problem-button";
                select.textContent = "Manage Test Cases";
                select.addEventListener("click", async () => {
                    problemIdInput.value = problem.problem_id;
                    document.getElementById("test-case-form").scrollIntoView({ behavior: "smooth" });
                    await loadTestCases(problem.problem_id);
                });
                const remove = document.createElement("button");
                remove.type = "button";
                remove.className = "admin-button admin-button-danger";
                remove.textContent = "Delete Problem";
                remove.addEventListener("click", async () => {
                    const confirmed = window.confirm(
                        `Delete “${problem.title}” (ID ${problem.problem_id})? Associated test cases and related records governed by database cascades may also be deleted. This cannot be undone.`
                    );
                    if (!confirmed) return;
                    remove.disabled = true;
                    try {
                        await apiRequest(`/api/admin/problems/${problem.problem_id}`, { method: "DELETE" });
                        if (Number(problemIdInput.value) === Number(problem.problem_id)) {
                            problemIdInput.value = "";
                            testCaseList.replaceChildren();
                            const hint = document.createElement("p");
                            hint.className = "admin-muted";
                            hint.textContent = "Select a problem to view its test cases.";
                            testCaseList.appendChild(hint);
                        }
                        await loadProblems();
                    } catch (error) {
                        window.alert(error.message || "Failed to delete problem.");
                        remove.disabled = false;
                    }
                });
                card.append(title, difficulty, description, constraints, time, space, algorithm, select, remove);
                problemList.appendChild(card);
            });
        } catch (error) {
            problemList.replaceChildren();
            const message = document.createElement("p");
            message.className = "admin-muted";
            message.textContent = error.message || "Unable to load problems.";
            problemList.appendChild(message);
        }
    }

    loadProblems();
});
