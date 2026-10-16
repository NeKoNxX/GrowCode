document.addEventListener("DOMContentLoaded", async () => {

    const adminMessage = document.getElementById("admin-message");

    try {
        const response = await fetch("/api/admin/check", {
            credentials: "include"
        });

        if (!response.ok) {
            adminMessage.textContent =
                "Access denied. Administrator privileges are required.";

            document.querySelectorAll(".admin-section").forEach(section => {
                section.style.display = "none";
            });

            return;
        }

        adminMessage.textContent =
            "Administrator access verified.";

    } catch (error) {
        adminMessage.textContent =
            "Unable to verify administrator access.";

        document.querySelectorAll(".admin-section").forEach(section => {
            section.style.display = "none";
        });

        return;
    }


    const problemForm =
        document.getElementById("problem-form");

    problemForm.addEventListener("submit", async (event) => {

        event.preventDefault();

        const formData = new URLSearchParams();

        formData.append(
            "title",
            document.getElementById("title").value
        );

        formData.append(
            "description",
            document.getElementById("description").value
        );

        formData.append(
            "difficulty",
            document.getElementById("difficulty").value
        );

        formData.append(
            "constraints",
            document.getElementById("constraints").value
        );


        const message =
            document.getElementById("problem-message");

        try {

            const response = await fetch(
                "/api/admin/problems",
                {
                    method: "POST",
                    headers: {
                        "Content-Type":
                            "application/x-www-form-urlencoded"
                    },
                    credentials: "include",
                    body: formData.toString()
                }
            );

            const data = await response.json();

            if (!response.ok) {
                message.textContent =
                    data.error || "Failed to create problem.";
                return;
            }

            message.textContent =
                "Problem created successfully. Problem ID: "
                + data.problem_id;

            document.getElementById("problem-id").value =
                data.problem_id;

            problemForm.reset();

        } catch (error) {

            message.textContent =
                "Unable to create problem.";
        }
    });


    const testCaseForm =
        document.getElementById("test-case-form");

    testCaseForm.addEventListener("submit", async (event) => {

        event.preventDefault();

        const formData = new URLSearchParams();

        formData.append(
            "problem_id",
            document.getElementById("problem-id").value
        );

        formData.append(
            "input",
            document.getElementById("input").value
        );

        formData.append(
            "expected_output",
            document.getElementById("expected-output").value
        );

        formData.append(
            "is_sample",
            document.getElementById("is-sample").value
        );

        formData.append(
            "order_no",
            document.getElementById("order-no").value
        );


        const message =
            document.getElementById("test-case-message");

        try {

            const response = await fetch(
                "/api/admin/test-cases",
                {
                    method: "POST",
                    headers: {
                        "Content-Type":
                            "application/x-www-form-urlencoded"
                    },
                    credentials: "include",
                    body: formData.toString()
                }
            );

            const data = await response.json();

            if (!response.ok) {
                message.textContent =
                    data.error || "Failed to create test case.";
                return;
            }

            message.textContent =
                "Test case created successfully. Test Case ID: "
                + data.test_case_id;

            document.getElementById("input").value = "";
            document.getElementById("expected-output").value = "";
            document.getElementById("order-no").value = "";

        } catch (error) {

            message.textContent =
                "Unable to create test case.";
        }
    });

    async function loadTestCases(problemId) {

        const testCaseList =
            document.getElementById("test-case-list");
    
        testCaseList.textContent =
            "Loading test cases...";
    
        try {
    
            const response = await fetch(
                `/api/admin/test-cases/${problemId}`,
                {
                    credentials: "include"
                }
            );
    
            const data = await response.json();
    
            if (!response.ok) {
                testCaseList.textContent =
                    data.error || "Failed to load test cases.";
                return;
            }
    
            if (data.length === 0) {
                testCaseList.textContent =
                    "No test cases available for this problem.";
                return;
            }
    
            testCaseList.innerHTML = "";
    
            data.forEach(testCase => {
    
                const testCaseCard =
                    document.createElement("div");
    
                testCaseCard.className =
                    "admin-test-case-card";
    
                testCaseCard.innerHTML = `
                    <h3>
                        Test Case #${testCase.test_case_id}
                    </h3>
    
                    <p>
                        <strong>Input:</strong>
                    </p>
    
                    <pre>${testCase.input || "(empty)"}</pre>
    
                    <p>
                        <strong>Expected Output:</strong>
                    </p>
    
                    <pre>${testCase.expected_output}</pre>
                `;
    
                testCaseList.appendChild(testCaseCard);
            });
    
        } catch (error) {
    
            testCaseList.textContent =
                "Unable to load test cases.";
        }
    }
    
    async function loadProblems() {

        const problemList =
            document.getElementById("problem-list");

        try {

            const response = await fetch(
                "/api/admin/problems",
                {
                    credentials: "include"
                }
            );

            const data = await response.json();

            if (!response.ok) {
                problemList.textContent =
                    data.error || "Failed to load problems.";
                return;
            }

            if (data.length === 0) {
                problemList.textContent =
                    "No problems available.";
                return;
            }

            problemList.innerHTML = "";

            data.forEach(problem => {

                const problemCard =
                    document.createElement("div");

                problemCard.className =
                    "admin-problem-card";

                    problemCard.innerHTML = `
                    <h3>
                        #${problem.problem_id}
                        ${problem.title}
                    </h3>
                
                    <p>
                        <strong>Difficulty:</strong>
                        ${problem.difficulty}
                    </p>
                
                    <p>
                        ${problem.description}
                    </p>
                
                    <p>
                        <strong>Constraints:</strong>
                        ${problem.constraints || "None"}
                    </p>
                
                    <button
                        type="button"
                        class="select-problem-button"
                        data-problem-id="${problem.problem_id}"
                    >
                        Add Test Case
                    </button>
                `;

                problemList.appendChild(problemCard);

                const selectButton =
                    problemCard.querySelector(".select-problem-button");

                selectButton.addEventListener("click", async () => {

                    document.getElementById("problem-id").value =
                        problem.problem_id;
                
                    document.getElementById("problem-id").focus();
                
                    document.getElementById("test-case-form")
                        .scrollIntoView({
                            behavior: "smooth"
                        });
                
                    await loadTestCases(problem.problem_id);
                });
            });

        } catch (error) {

            problemList.textContent =
                "Unable to load problems.";
        }
    }

    async function checkAdminAccess() {
        try {
            const response = await fetch("/api/admin/check", {
                credentials: "include",
                cache: "no-store"
            });
    
            if (!response.ok) {
                window.location.replace(
                    response.status === 401 ? "/login.html" : "/"
                );
                return false;
            }
    
            return true;
        } catch (error) {
            console.error("Unable to verify admin access:", error);
            window.location.replace("/");
            return false;
        }
    }
    
    loadProblems();
});
