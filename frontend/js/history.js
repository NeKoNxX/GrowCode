/**
 * GrowCode — Submission History
 *
 * API endpoints:
 *   GET /api/submissions
 *   GET /api/submissions/{submission_id}/code
 *   GET /api/submissions/{submission_id}
 */

async function loadSubmissionHistory() {
    const message = document.getElementById("history-message");
    const container = document.getElementById("submission-history");

    if (!message || !container) {
        console.error("Submission history elements were not found.");
        return;
    }

    message.textContent = "Loading submission history...";
    container.replaceChildren();

    try {
        const response = await fetch("/api/submissions", {
            method: "GET",
            credentials: "include",
            headers: { Accept: "application/json" }
        });

        const result = await response.json();

        if (!response.ok) {
            throw new Error(
                result.error || "Unable to load submissions."
            );
        }

        if (!Array.isArray(result) || result.length === 0) {
            message.textContent =
                "You have not made any submissions yet.";
            return;
        }

        message.textContent = "";

        const table = document.createElement("table");
        table.className = "submission-table";

        const thead = document.createElement("thead");
        const headerRow = document.createElement("tr");

        [
            "Submission ID",
            "Problem",
            "Language",
            "Status",
            "Submitted At",
            "Details"
        ].forEach(label => {
            const th = document.createElement("th");
            th.textContent = label;
            headerRow.appendChild(th);
        });

        thead.appendChild(headerRow);
        table.appendChild(thead);

        const tbody = document.createElement("tbody");

        const totalSubmissions = result.length;

        result.forEach((submission, index) => {
            const row = document.createElement("tr");

            const displayNumber = totalSubmissions - index;

            [
                displayNumber,
                submission.problem_title,
                submission.language,
                submission.status,
                submission.submitted_at
            ].forEach((value, index) => {
                const cell = document.createElement("td");

                if (index === 3) {
                    const badge = document.createElement("span");

                    badge.className =
                        "status " +
                        String(value || "unknown")
                            .toLowerCase()
                            .replace(/[^a-z0-9_-]/g, "");

                    badge.textContent = value ?? "Unknown";
                    cell.appendChild(badge);
                } else {
                    cell.textContent = value ?? "";
                }

                row.appendChild(cell);
            });

            const detailsCell = document.createElement("td");
            const detailsButton = document.createElement("button");

            detailsButton.type = "button";
            detailsButton.className = "view-details-button";
            detailsButton.textContent = "View Details";

            detailsButton.addEventListener("click", () => {
                viewSubmissionDetails(submission, detailsButton);
            });

            detailsCell.appendChild(detailsButton);
            row.appendChild(detailsCell);
            tbody.appendChild(row);
        });

        table.appendChild(tbody);
        container.appendChild(table);

    } catch (error) {
        console.error("Submission history error:", error);

        message.textContent =
            error.message || "Unable to connect to the server.";
    }
}


async function viewSubmissionDetails(submission, triggerButton) {
    const modal = document.getElementById("details-modal");
    const content = document.getElementById("details-content");
    const message = document.getElementById("details-message");

    if (!modal || !content || !message) {
        console.error(
            "Submission details modal is missing. Check history.html."
        );
        return;
    }

    const rawSubmissionId = submission?.submission_id;

    if (rawSubmissionId === undefined || rawSubmissionId === null) {
        message.textContent = "Invalid submission ID.";
        return;
    }

    const submissionId = encodeURIComponent(
        String(rawSubmissionId)
    );

    modal.hidden = false;
    content.replaceChildren();

    message.textContent = "Loading submission details...";

    if (triggerButton) {
        triggerButton.disabled = true;
    }

    try {
        // Fetch source code and detailed submission information.
        const [codeResponse, detailsResponse] = await Promise.all([
            fetch(`/api/submissions/${submissionId}/code`, {
                method: "GET",
                credentials: "include",
                headers: { Accept: "application/json" }
            }),

            fetch(`/api/submissions/${submissionId}`, {
                method: "GET",
                credentials: "include",
                headers: { Accept: "application/json" }
            })
        ]);

        const [codeResult, detailsResult] = await Promise.all([
            codeResponse.json(),
            detailsResponse.json()
        ]);

        if (!codeResponse.ok) {
            throw new Error(
                codeResult.error || "Unable to load submitted code."
            );
        }

        if (!detailsResponse.ok) {
            throw new Error(
                detailsResult.error ||
                "Unable to load submission details."
            );
        }

        message.textContent = "";

        // Prefer values from the details endpoint.
        // Fall back to the submission history object.
        const field = keys =>
            getFirstValue(detailsResult, keys) ??
            getFirstValue(submission, keys);

        const heading = document.createElement("h2");

        heading.textContent =
            submission.problem_title || "Submission Details";

        content.appendChild(heading);

        // Submission metadata.
        const metadata = [
            [
                "Submission ID",
                field(["submission_id"])
            ],
            [
                "Problem",
                field(["problem_title"])
            ],
            [
                "Language",
                field(["language"])
            ],
            [
                "Verdict",
                field(["status", "verdict"])
            ],
            [
                "Submitted At",
                field(["submitted_at"])
            ],
            [
                "Time Complexity",
                field([
                    "estimated_time_complexity",
                    "time_complexity"
                ])
            ],
            [
                "Space Complexity",
                field([
                    "estimated_space_complexity",
                    "space_complexity"
                ])
            ],
            [
                "Execution Time",
                formatMetric(
                    field([
                        "execution_time_ms",
                        "execution_time"
                    ]),
                    " ms"
                )
            ],
            [
                "Memory Used",
                formatMetric(
                    field([
                        "memory_used_kb",
                        "memory_usage_kb"
                    ]),
                    " KB"
                )
            ],
            [
                "Test Cases Passed",
                formatTestCount(
                    field(["passed_test_cases"]),
                    field(["total_test_cases"])
                )
            ]
        ].filter(([, value]) =>
            value !== undefined &&
            value !== null &&
            value !== ""
        );

        const metadataTable = document.createElement("table");
        metadataTable.className = "submission-details-table";

        const metadataBody = document.createElement("tbody");

        metadata.forEach(([label, value]) => {
            const row = document.createElement("tr");

            const th = document.createElement("th");
            th.textContent = label;

            const td = document.createElement("td");
            td.textContent = String(value);

            row.append(th, td);
            metadataBody.appendChild(row);
        });

        metadataTable.appendChild(metadataBody);
        content.appendChild(metadataTable);


        // Problem-level expected complexity and recommended algorithm.
        const expectedTime = field([
            "expected_time_complexity"
        ]);

        const expectedSpace = field([
            "expected_space_complexity"
        ]);

        const recommendedAlgorithm = field([
            "recommended_algorithm"
        ]);

        if (expectedTime || expectedSpace || recommendedAlgorithm) {
            const expectedHeading = document.createElement("h3");

            expectedHeading.textContent = "Problem Expectations";
            content.appendChild(expectedHeading);

            const expectedList = document.createElement("ul");

            if (expectedTime) {
                addListItem(
                    expectedList,
                    `Expected Time Complexity: ${expectedTime}`
                );
            }

            if (expectedSpace) {
                addListItem(
                    expectedList,
                    `Expected Space Complexity: ${expectedSpace}`
                );
            }

            if (recommendedAlgorithm) {
                addListItem(
                    expectedList,
                    `Recommended Algorithm: ${recommendedAlgorithm}`
                );
            }

            content.appendChild(expectedList);
        }


        // Optional complexity-analysis explanation and confidence.
        const explanation = field([
            "explanation",
            "complexity_explanation",
            "analysis_explanation"
        ]);

        const confidence = field([
            "confidence",
            "complexity_confidence",
            "analysis_confidence"
        ]);

        if (
            explanation ||
            (
                confidence !== undefined &&
                confidence !== null &&
                confidence !== ""
            )
        ) {
            const analysisHeading = document.createElement("h3");

            analysisHeading.textContent = "Complexity Analysis";
            content.appendChild(analysisHeading);

            if (explanation) {
                const explanationParagraph =
                    document.createElement("p");

                explanationParagraph.textContent =
                    String(explanation);

                content.appendChild(explanationParagraph);
            }

            if (
                confidence !== undefined &&
                confidence !== null &&
                confidence !== ""
            ) {
                const confidenceParagraph =
                    document.createElement("p");

                confidenceParagraph.textContent =
                    `Confidence: ${confidence}`;

                content.appendChild(confidenceParagraph);
            }
        }


        // Submitted source code.
        const codeHeading = document.createElement("h3");

        codeHeading.textContent = "Submitted Source Code";
        content.appendChild(codeHeading);

        const codeContainer = document.createElement("pre");
        codeContainer.className = "submission-code-display";

        const codeElement = document.createElement("code");

        codeElement.textContent = codeResult.code ?? "";

        codeContainer.appendChild(codeElement);
        content.appendChild(codeContainer);


        // Copy source code.
        const copyButton = document.createElement("button");

        copyButton.type = "button";
        copyButton.textContent = "Copy Code";

        copyButton.addEventListener("click", async () => {
            try {
                await navigator.clipboard.writeText(
                    codeResult.code ?? ""
                );

                message.textContent = "Code copied.";

            } catch (error) {
                console.error("Copy error:", error);

                message.textContent =
                    "Copy failed. Select and copy the code manually.";
            }
        });

        content.appendChild(copyButton);


        // Individual test-case results.
        const testCases = getFirstValue(detailsResult, [
            "test_cases",
            "test_case_results",
            "execution_results"
        ]);

        const testHeading = document.createElement("h3");

        testHeading.textContent = "Test Case Results";
        content.appendChild(testHeading);

        if (!Array.isArray(testCases) || testCases.length === 0) {
            const noTests = document.createElement("p");

            noTests.textContent = Array.isArray(testCases)
                ? "No test-case results have been recorded for this submission yet."
                : "Test-case details are not available from the server.";

            content.appendChild(noTests);

        } else {
            const testTable = document.createElement("table");

            testTable.className = "submission-details-table";

            const testHead = document.createElement("thead");
            const testHeaderRow = document.createElement("tr");

            [
                "Test Case",
                "Status",
                "Execution Time",
                "Memory Used"
            ].forEach(label => {
                const th = document.createElement("th");

                th.textContent = label;
                testHeaderRow.appendChild(th);
            });

            testHead.appendChild(testHeaderRow);
            testTable.appendChild(testHead);

            const testBody = document.createElement("tbody");

            testCases.forEach((test, index) => {
                const row = document.createElement("tr");

                const testId =
                    test.test_case_id ??
                    test.test_case_number ??
                    index + 1;

                const status =
                    test.status ??
                    test.verdict ??
                    test.result ??
                    "Unknown";

                const executionTime =
                    test.execution_time_ms ??
                    test.time_taken ??
                    test.execution_time;

                const memoryUsed =
                    test.memory_used_kb ??
                    test.memory_used ??
                    test.memory_usage_kb;

                const values = [
                    `Test Case ${testId}`,
                    status,
                    formatMetric(executionTime, " ms") ??
                        "Not available",
                    formatMemory(memoryUsed)
                ];

                values.forEach(value => {
                    const td = document.createElement("td");

                    td.textContent = String(value);
                    row.appendChild(td);
                });

                testBody.appendChild(row);
            });

            testTable.appendChild(testBody);
            content.appendChild(testTable);
        }

    } catch (error) {
        console.error("Submission details error:", error);

        message.textContent =
            error.message || "Unable to load submission details.";

    } finally {
        if (triggerButton) {
            triggerButton.disabled = false;
        }
    }
}


// Returns the first defined, non-null, non-empty value.
function getFirstValue(object, keys) {
    for (const key of keys) {
        if (
            object &&
            object[key] !== undefined &&
            object[key] !== null &&
            object[key] !== ""
        ) {
            return object[key];
        }
    }

    return undefined;
}


// Formats a metric only when the API supplies a value.
function formatMetric(value, unit) {
    if (
        value === undefined ||
        value === null ||
        value === ""
    ) {
        return undefined;
    }

    return `${value}${unit}`;
}


// Database memory_used is expected to be expressed in KB here.
// Confirm the unit used by ProcessManager before interpreting the value.
function formatMemory(value) {
    if (
        value === undefined ||
        value === null ||
        value === ""
    ) {
        return "Not available";
    }

    return `${value} KB`;
}


// Formats the number of accepted test cases.
function formatTestCount(passed, total) {
    if (
        passed === undefined ||
        passed === null ||
        passed === "" ||
        total === undefined ||
        total === null ||
        total === ""
    ) {
        return undefined;
    }

    return `${passed} / ${total}`;
}


// Adds text safely to a list.
function addListItem(list, text) {
    const item = document.createElement("li");

    item.textContent = text;
    list.appendChild(item);
}


// Modal close behaviour.
document.addEventListener("DOMContentLoaded", () => {
    const modal = document.getElementById("details-modal");
    const closeButton = document.getElementById(
        "close-details-modal"
    );

    if (closeButton && modal) {
        closeButton.addEventListener("click", () => {
            modal.hidden = true;
        });
    }

    if (modal) {
        modal.addEventListener("click", event => {
            if (event.target === modal) {
                modal.hidden = true;
            }
        });

        document.addEventListener("keydown", event => {
            if (
                event.key === "Escape" &&
                !modal.hidden
            ) {
                modal.hidden = true;
            }
        });
    }

    loadSubmissionHistory();
});