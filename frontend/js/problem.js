/* GrowCode problem workspace. API routes and language IDs are kept unchanged. */
const FINAL_STATUSES = new Set([
    "ACCEPTED",
    "WRONG_ANSWER",
    "COMPILATION_ERROR",
    "RUNTIME_ERROR",
    "TIME_LIMIT_EXCEEDED"
]);

const LANGUAGE_NAMES = {
    1: "C++17",
    2: "Java",
    3: "Python 3"
};

function escapeHTML(value) {
    return String(value ?? "")
        .replaceAll("&", "&amp;")
        .replaceAll("<", "&lt;")
        .replaceAll(">", "&gt;")
        .replaceAll('"', "&quot;")
        .replaceAll("'", "&#39;");
}

function statusClass(status) {
    return String(status || "pending").toLowerCase().replaceAll("_", "-");
}

async function pollSubmissionStatus(submissionId, context) {
    const message = document.getElementById("submission-message");
    const submitButton = document.getElementById("submit-button");
    const resultIcon = document.getElementById("submission-result-icon");

    for (;;) {
        try {
            const response = await fetch(`/api/submissions/${submissionId}`, {
                credentials: "include"
            });
            if (!response.ok) throw new Error("Unable to get submission status");

            const result = await response.json();
            const status = String(result.status || "UNKNOWN").toUpperCase();
            message.className = `submission-message status-${statusClass(status)}`;
            message.innerHTML = `
                <span class="submission-status-icon" id="submission-result-icon" aria-hidden="true">${FINAL_STATUSES.has(status) ? (status === "ACCEPTED" ? "✓" : "!") : "…"}</span>
                <span class="submission-message-copy"><strong>${escapeHTML(status.replaceAll("_", " "))}</strong><small>Submission #${escapeHTML(submissionId)}</small></span>
            `;
            if (FINAL_STATUSES.has(status)) {
                await showComplexityReport(submissionId, status, result, context);
                break;
            }
            await new Promise(resolve => setTimeout(resolve, 1000));
        } catch (error) {
            console.error(error);
            message.className = "submission-message status-error";
            message.textContent = "Submission received, but its status could not be checked. Open Submission History to verify it.";
            break;
        }
    }
    if (submitButton) submitButton.disabled = false;
}

function complexityRank(value) {
    const v = String(value || "").replace(/\s/g, "").toLowerCase();
    if (v.includes("2^n") || v.includes("2ⁿ") || v.includes("exponential")) return 6;
    if (v.includes("n^3") || v.includes("n³")) return 5;
    if (v.includes("n^2") || v.includes("n²")) return 4;
    if (v.includes("nlogn") || v.includes("n*logn")) return 3;
    if (v.includes("logn") || v.includes("log(n)")) return 1;
    if (v.includes("o(n)")) return 2;
    if (v.includes("o(1)")) return 0;
    return 2;
}

function reportBar(label, value, rank) {
    const width = Math.max(8, Math.min(100, (rank + 1) / 7 * 100));
    return `<div class="complexity-chart-row"><div class="complexity-chart-label"><span>${escapeHTML(label)}</span><strong>${escapeHTML(value || "Unavailable")}</strong></div><div class="complexity-chart-track"><div class="complexity-chart-bar" style="width:${width}%"></div></div></div>`;
}


async function showComplexityReport(submissionId, status, result, context) {
    const old = document.getElementById("complexity-report-overlay");
    if (old) old.remove();

    const passed = Number(result.passed_test_cases || 0);
    const total = Number(result.total_test_cases || 0);

    // Normalize the verdict returned by the judge.
    const verdict = String(status || "UNKNOWN")
        .trim()
        .toUpperCase();

    // Analyze complexity only when the submission is accepted.
    let analysis = null;
    let analysisError = "";

    if (verdict === "ACCEPTED") {
        try {
            const response = await fetch("/api/complexity/analyze", {
                method: "POST",
                credentials: "include",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({
                    code: context.code,
                    language_id: context.languageId,
                    problem_id: context.problemId
                })
            });

            const payload = await response.json();

            if (!response.ok) {
                throw new Error(payload.error || "Analysis unavailable");
            }

            analysis = payload;
        } catch (error) {
            analysisError =
                error.message || "Complexity analysis unavailable";
        }
    }

    const analysisAvailable = analysis?.analysis_available === true;

    const time = analysis?.time_complexity || "Unavailable";
    const space = analysis?.space_complexity || "Unavailable";

    const expectedTime =
        analysis?.expected_time_complexity || "Not configured";

    const expectedSpace =
        analysis?.expected_space_complexity || "Not configured";

    const recommendedAlgorithm =
        analysis?.recommended_algorithm || "Not configured";

    // Extract any diagnostic information already returned by the judge.
    const diagnostic =
        result.error_message ||
        result.error ||
        result.stderr ||
        result.compile_output ||
        result.message ||
        "";

    // Only accepted submissions get the normal complexity report.
    const complexitySection = verdict === "ACCEPTED"
        ? `
           <div class="complexity-report-grid">
                <article>
                    <span>
                        ${analysisAvailable
                            ? "AI-estimated time"
                            : "Expected time complexity"}
                    </span>

                    <strong>
                        ${escapeHTML(
                            analysisAvailable ? time : expectedTime
                        )}
                    </strong>

                    <small>
                        ${analysisAvailable
                            ? `Expected: ${escapeHTML(expectedTime)}`
                            : "Stored in GrowCode database"}
                    </small>
                </article>

                <article>
                    <span>
                        ${analysisAvailable
                            ? "AI-estimated space"
                            : "Expected space complexity"}
                    </span>

                    <strong>
                        ${escapeHTML(
                            analysisAvailable ? space : expectedSpace
                        )}
                    </strong>

                    <small>
                        ${analysisAvailable
                            ? `Expected: ${escapeHTML(expectedSpace)}`
                            : "Stored in GrowCode database"}
                    </small>
                </article>

                <article>
                    <span>Recommended Algorithm</span>

                    <strong>
                        ${escapeHTML(recommendedAlgorithm)}
                    </strong>

                    <small>Recommended for this problem</small>
                </article>
            </div>

            ${
                analysis
                    ? `
                        <div class="complexity-chart">
                            <h3>Complexity comparison</h3>
                            ${reportBar(
                                "Time · estimated",
                                time,
                                complexityRank(time)
                            )}
                            ${reportBar(
                                "Time · expected",
                                expectedTime,
                                complexityRank(expectedTime)
                            )}
                            ${reportBar(
                                "Space · estimated",
                                space,
                                complexityRank(space)
                            )}
                            ${reportBar(
                                "Space · expected",
                                expectedSpace,
                                complexityRank(expectedSpace)
                            )}
                            <p>
                                Bars represent a rough complexity class scale,
                                not measured execution time or memory.
                            </p>
                        </div>

                        <p class="complexity-explanation">
                            ${escapeHTML(analysis.explanation || "")}
                            <strong>
                                Confidence:
                                ${escapeHTML(analysis.confidence || "Unknown")}
                            </strong>
                        </p>
                    `
                    : `
                        <p class="complexity-analysis-error">
                            AI complexity analysis is unavailable.
                            Showing the expected complexities and recommended
                            algorithm stored in GrowCode.
                        </p>
                    `
            }
        `
        : `
            <div class="complexity-failure-details">
                <h3>Submission failed</h3>
                <p>
                    Complexity analysis was not performed because the
                    submission verdict is
                    <strong>${escapeHTML(verdict.replaceAll("_", " "))}</strong>.
                </p>
                ${
                    diagnostic
                        ? `
                            <h4>Error details</h4>
                            <pre>${escapeHTML(diagnostic)}</pre>
                        `
                        : `
                            <p>
                                No detailed diagnostic was returned by
                                the submission API.
                            </p>
                        `
                }
            </div>
        `;

    const overlay = document.createElement("div");
    overlay.id = "complexity-report-overlay";
    overlay.className = "complexity-report-overlay";

    overlay.innerHTML = `
        <section
            class="complexity-report-modal"
            role="dialog"
            aria-modal="true"
            aria-labelledby="complexity-report-title"
        >
            <button
                type="button"
                class="complexity-report-close"
                aria-label="Close report"
            >×</button>

            <div class="complexity-report-eyebrow">
                GROWCODE · SUBMISSION REPORT
            </div>

            <h2 id="complexity-report-title">
                Submission #${escapeHTML(submissionId)}
            </h2>

            <div class="complexity-report-summary">
                <span class="complexity-verdict ${statusClass(verdict)}">
                    ${escapeHTML(verdict.replaceAll("_", " "))}
                </span>
                <span>
                    <strong>${passed} / ${total}</strong>
                    <small>test cases passed</small>
                </span>
            </div>

            ${complexitySection}

            <details class="complexity-submitted-code">
                <summary>View submitted code</summary>
                <pre><code>${escapeHTML(context.code)}</code></pre>
            </details>

            ${
                verdict === "ACCEPTED"
                    ? `
                        <p class="complexity-report-disclaimer">
                            AI-generated complexity is an estimate, not a
                            formal proof. Source code is sent to the
                            configured Gemini API for analysis.
                        </p>
                    `
                    : ""
            }
        </section>
    `;

    document.body.appendChild(overlay);

    const close = () => overlay.remove();

    overlay
        .querySelector(".complexity-report-close")
        .addEventListener("click", close);

    overlay.addEventListener("click", event => {
        if (event.target === overlay) close();
    });

    document.addEventListener("keydown", function esc(event) {
        if (event.key === "Escape") {
            close();
            document.removeEventListener("keydown", esc);
        }
    });
}


async function submitSolution(problemId) {
    const codeEditor = document.getElementById("code-editor");
    const languageSelect = document.getElementById("language-select");
    const message = document.getElementById("submission-message");
    const button = document.getElementById("submit-button");
    const code = codeEditor.value;
    const languageId = Number(languageSelect.value);

    if (!code.trim()) {
        message.className = "submission-message status-error";
        message.textContent = "Write your solution before submitting.";
        codeEditor.focus();
        return;
    }
    if (![1, 2, 3].includes(languageId)) {
        message.className = "submission-message status-error";
        message.textContent = "Please select a supported programming language.";
        return;
    }

    button.disabled = true;
    button.innerHTML = '<span class="button-spinner" aria-hidden="true"></span> Submitting…';
    message.className = "submission-message status-running";
    message.innerHTML = '<span class="submission-status-icon" aria-hidden="true">…</span><span class="submission-message-copy"><strong>Submitting solution</strong><small>Your code is being sent to the judge.</small></span>';

    try {
        const response = await fetch("/api/submissions", {
            method: "POST",
            credentials: "include",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ problem_id: Number(problemId), language_id: languageId, code })
        });
        const result = await response.json();
        if (!response.ok) throw new Error(result.error || "Submission failed.");
        if (!result.submission_id) throw new Error("The server did not return a submission ID.");
        button.innerHTML = '<span aria-hidden="true">✓</span> Submitted';
        await pollSubmissionStatus(result.submission_id, { code, languageId, problemId: Number(problemId) });
    } catch (error) {
        console.error(error);
        message.className = "submission-message status-error";
        message.textContent = error.message || "Unable to submit solution.";
        button.disabled = false;
    } finally {
        button.innerHTML = '<span aria-hidden="true">↑</span> Submit solution';
    }
}


async function runCode() {
    const editor = document.getElementById("code-editor");
    const languageSelect = document.getElementById("language-select");
    const inputField = document.getElementById("custom-input");
    const runButton = document.getElementById("run-code-button");

    const resultPanel = document.getElementById("run-code-result");
    const statusElement = document.getElementById("run-code-status");
    const timeElement = document.getElementById("run-code-time");
    const outputElement = document.getElementById("run-code-output");
    const errorContainer = document.getElementById("run-code-error-container");
    const errorElement = document.getElementById("run-code-error");

    const code = editor.value;
    const languageId = Number(languageSelect.value);
    const customInput = inputField.value;

    if (!code.trim()) {
        alert("Write your code before running it.");
        editor.focus();
        return;
    }

    if (![1, 2, 3].includes(languageId)) {
        alert("Please select a supported programming language.");
        return;
    }

    runButton.disabled = true;
    runButton.textContent = "Running...";
    resultPanel.hidden = false;

    statusElement.textContent = "Executing...";
    timeElement.textContent = "";
    outputElement.textContent = "";
    errorElement.textContent = "";
    errorContainer.hidden = true;

    try {
        const problemId = new URLSearchParams(window.location.search).get("id");

        const problemResponse = await fetch(
            `/api/problems/${encodeURIComponent(problemId)}`
        );

        if (!problemResponse.ok) {
            throw new Error("Unable to retrieve sample test cases.");
        }

        const problem = await problemResponse.json();
        const samples = Array.isArray(problem.sample_test_cases)
            ? problem.sample_test_cases
            : [];

        const useCustomInput = customInput.trim().length > 0;

        const testCases = useCustomInput
            ? [{
                input: customInput,
                expected_output: null,
                label: "Custom Input"
            }]
            : samples.map((sample, index) => ({
                input: sample.input ?? "",
                expected_output: sample.expected_output ?? "",
                label: `Sample Test Case ${index + 1}`
            }));

        if (testCases.length === 0) {
            throw new Error(
                "No sample test cases available. Enter custom input to run your code."
            );
        }

        let totalTime = 0;
        let passedCount = 0;
        const results = [];

        for (const testCase of testCases) {
            const response = await fetch("/api/run", {
                method: "POST",
                credentials: "include",
                headers: {
                    "Content-Type": "application/json"
                },
                body: JSON.stringify({
                    code,
                    language_id: languageId,
                    input: testCase.input
                })
            });

            const result = await response.json();

            if (!response.ok) {
                throw new Error(
                    result.error || "Code execution request failed."
                );
            }

            totalTime += Number(result.execution_time || 0);

            const actualOutput = String(result.output ?? "");
            const expectedOutput = testCase.expected_output;

            const normalizeOutput = value =>
                String(value ?? "")
                    .trim()
                    .split(/\s+/)
                    .join(" ");

            const passed = useCustomInput
                ? result.status === "SUCCESS"
                : result.status === "SUCCESS" &&
                  normalizeOutput(actualOutput) ===
                  normalizeOutput(expectedOutput);

            if (passed) {
                passedCount++;
            }

            results.push({
                label: testCase.label,
                input: testCase.input,
                expected: expectedOutput,
                actual: actualOutput,
                status: result.status,
                passed,
                error: result.error || ""
            });
        }

        statusElement.textContent = useCustomInput
            ? results[0].status.replaceAll("_", " ")
            : `${passedCount} / ${testCases.length} Sample Tests Passed`;

        timeElement.textContent =
            `Total execution time: ${totalTime.toFixed(3)} s`;

        outputElement.textContent = results.map(result => {
            if (useCustomInput) {
                return result.actual || "(No output)";
            }

            return [
                `${result.label}: ${result.passed ? "PASSED" : "FAILED"}`,
                "",
                "Input:",
                result.input || "(empty)",
                "",
                "Expected Output:",
                result.expected || "(empty)",
                "",
                "Your Output:",
                result.actual || "(empty)",
                result.error ? `\nError:\n${result.error}` : ""
            ].join("\n");
        }).join("\n\n--------------------\n\n");

        const failures = results.filter(result =>
            !result.passed && result.error
        );

        if (failures.length > 0) {
            errorElement.textContent = failures
                .map(result => `${result.label}: ${result.error}`)
                .join("\n\n");

            errorContainer.hidden = false;
        }

    } catch (error) {
        statusElement.textContent = "Execution Failed";
        errorElement.textContent =
            error.message || "Unable to execute code.";

        errorContainer.hidden = false;
        console.error("Run Code error:", error);

    } finally {
        runButton.disabled = false;
        runButton.textContent = "▶ Run Code";
    }
}


function updateEditorPlaceholder() {
    const select = document.getElementById("language-select");
    const editor = document.getElementById("code-editor");
    if (!select || !editor) return;
    const placeholders = {
        "1": "// Write your C++17 solution here…",
        "2": "// Write your Java solution here…\n// Use public class Main",
        "3": "# Write your Python 3 solution here…"
    };
    editor.placeholder = placeholders[select.value] || "Write your solution here…";
    document.getElementById("editor-language-label").textContent = LANGUAGE_NAMES[select.value] || "Code";
}

function setupEditor() {
    const editor = document.getElementById("code-editor");
    const copyButton = document.getElementById("copy-code-button");
    const resetButton = document.getElementById("reset-code-button");
    const languageSelect = document.getElementById("language-select");
    const initialCode = "";

    languageSelect.addEventListener("change", updateEditorPlaceholder);
    editor.addEventListener("keydown", event => {
        if (event.key === "Tab") {
            event.preventDefault();
            const start = editor.selectionStart;
            const end = editor.selectionEnd;
            editor.setRangeText("    ", start, end, "end");
            editor.dispatchEvent(new Event("input", { bubbles: true }));
        }
    });
    copyButton.addEventListener("click", async () => {
        try {
            await navigator.clipboard.writeText(editor.value);
            copyButton.textContent = "Copied";
            setTimeout(() => { copyButton.textContent = "Copy code"; }, 1400);
        } catch {
            editor.focus();
            editor.select();
            copyButton.textContent = "Select and copy";
        }
    });
    resetButton.addEventListener("click", () => {
        if (!editor.value || window.confirm("Clear the editor contents?")) {
            editor.value = initialCode;
            editor.focus();
        }
    });
    updateEditorPlaceholder();
}


// DISCUSSION FUNCTIONALITY

let discussionLoggedIn = false;
let currentDiscussionUserId = null;

// Check whether the current user is logged in.
async function checkDiscussionAuth() {
    try {
        const response = await fetch("/api/session", {
            credentials: "include"
        });

        const session = await response.json();

        discussionLoggedIn = Boolean(session.logged_in);

        currentDiscussionUserId = discussionLoggedIn
            ? Number(session.user_id)
            : null;

        const formCard = document.getElementById("discussion-form-card");
        const loginMessage = document.getElementById("discussion-login-message");

        if (formCard) {
            formCard.hidden = !discussionLoggedIn;
        }

        if (loginMessage) {
            loginMessage.hidden = discussionLoggedIn;
        }

    } catch (error) {
        console.error("Could not check login status:", error);
        discussionLoggedIn = false;
        currentDiscussionUserId = null;
    }
}


// Load all discussions for the current problem.
async function loadDiscussions(problemId) {
    const discussionList = document.getElementById("discussion-list");

    discussionList.innerHTML =
        '<p class="discussion-loading">Loading discussions...</p>';

    try {
        const response = await fetch(
            `/api/problems/${encodeURIComponent(problemId)}/discussions`
        );

        if (!response.ok) {
            throw new Error("Failed to load discussions.");
        }

        const discussions = await response.json();

        renderDiscussions(discussions);

    } catch (error) {
        console.error(error);

        discussionList.innerHTML =
            '<p class="discussion-empty">Unable to load discussions. Please try again.</p>';
    }
}


// Render the discussion list.
function renderDiscussions(discussions) {
    const discussionList = document.getElementById("discussion-list");
    const discussionCount = document.getElementById("discussion-count");

    discussionCount.textContent =
        `${discussions.length} ${discussions.length === 1 ? "discussion" : "discussions"}`;

    if (discussions.length === 0) {
        discussionList.innerHTML =
            '<p class="discussion-empty">No discussions yet. Be the first to start one!</p>';
        return;
    }

    discussionList.innerHTML = discussions.map(discussion => `
        <article class="discussion-card" data-discussion-id="${Number(discussion.discussion_id)}">

            <div class="discussion-card-header">
                <h3>${escapeHTML(discussion.title)}</h3>
                <span>${escapeHTML(discussion.username)}</span>
            </div>

            ${discussionLoggedIn &&
            Number(discussion.user_id) === currentDiscussionUserId ? `
                <div class="discussion-owner-actions">
                    <button
                        type="button"
                        data-action="edit-discussion"
                        data-id="${Number(discussion.discussion_id)}"
                    >
                        Edit
                    </button>

                    <button
                        type="button"
                        data-action="delete-discussion"
                        data-id="${Number(discussion.discussion_id)}"
                    >
                        Delete
                    </button>
                </div>
            ` : ""}

            <p class="discussion-content">
                ${escapeHTML(discussion.content)}
            </p>

            <div class="discussion-card-footer">
                <span>${escapeHTML(discussion.created_at)}</span>

                <button
                    type="button"
                    class="discussion-action"
                    data-action="toggle-replies"
                    data-id="${Number(discussion.discussion_id)}"
                >
                    View replies
                </button>
            </div>

            <div
                class="discussion-replies"
                id="replies-${Number(discussion.discussion_id)}"
                hidden
            ></div>

        </article>
    `).join("");
}


// Load replies for a particular discussion.
async function loadDiscussionReplies(discussionId) {
    const repliesContainer = document.getElementById(
        `replies-${discussionId}`
    );

    repliesContainer.hidden = false;
    repliesContainer.innerHTML =
        '<p class="discussion-loading">Loading replies...</p>';

    try {
        const response = await fetch(
            `/api/discussions/${discussionId}/replies`
        );

        if (!response.ok) {
            throw new Error("Failed to load replies.");
        }

        const replies = await response.json();

        renderReplies(discussionId, replies);

    } catch (error) {
        console.error(error);

        repliesContainer.innerHTML =
            '<p class="discussion-empty">Unable to load replies.</p>';
    }
}


// Render replies and the reply form.
function renderReplies(discussionId, replies) {
    const repliesContainer = document.getElementById(
        `replies-${discussionId}`
    );

    let html = "";

    if (replies.length === 0) {
        html += '<p class="discussion-empty">No replies yet.</p>';
    } else {
        html += replies.map(reply => `
            <div class="discussion-reply">
                <div class="discussion-reply-header">
                    <strong>${escapeHTML(reply.username)}</strong>
                    <span>${escapeHTML(reply.created_at)}</span>
                </div>

                <p>${escapeHTML(reply.content)}</p>

                ${discussionLoggedIn &&
                Number(reply.user_id) === currentDiscussionUserId ? `
                    <div class="discussion-owner-actions">
                        <button
                            type="button"
                            data-action="edit-reply"
                            data-id="${Number(reply.reply_id)}"
                            data-discussion-id="${discussionId}"
                        >
                            Edit
                        </button>

                        <button
                            type="button"
                            data-action="delete-reply"
                            data-id="${Number(reply.reply_id)}"
                            data-discussion-id="${discussionId}"
                        >
                            Delete
                        </button>
                    </div>
                ` : ""}
            </div>
        `).join("");
    }

    if (discussionLoggedIn) {
        html += `
            <form class="discussion-reply-form" data-id="${discussionId}">
                <textarea
                    name="reply-content"
                    rows="2"
                    placeholder="Write a reply..."
                    required
                ></textarea>

                <button type="submit">Post reply</button>
            </form>
        `;
    } else {
        html += `
            <p class="discussion-login-reply">
                <a href="/login.html">Log in</a> to reply.
            </p>
        `;
    }

    repliesContainer.innerHTML = html;
}


// Create a new discussion.
async function createDiscussion(event, problemId) {
    event.preventDefault();

    const titleInput = document.getElementById("discussion-title");
    const contentInput = document.getElementById("discussion-content");
    const feedback = document.getElementById("discussion-feedback");
    const submitButton = document.getElementById("discussion-submit");

    const title = titleInput.value.trim();
    const content = contentInput.value.trim();

    if (!title || !content) {
        feedback.textContent = "Please fill in both fields.";
        return;
    }

    submitButton.disabled = true;
    feedback.textContent = "Posting...";

    try {
        const response = await fetch(
            `/api/problems/${encodeURIComponent(problemId)}/discussions`,
            {
                method: "POST",
                headers: {
                    "Content-Type": "application/json"
                },
                credentials: "include",
                body: JSON.stringify({ title, content })
            }
        );

        const result = await response.json();

        if (!response.ok) {
            throw new Error(result.error || "Could not post discussion.");
        }

        titleInput.value = "";
        contentInput.value = "";

        feedback.textContent = "Discussion posted successfully.";

        await loadDiscussions(problemId);

    } catch (error) {
        feedback.textContent = error.message;
    } finally {
        submitButton.disabled = false;
    }
}


// Create a reply to a discussion.
async function createDiscussionReply(event, discussionId) {
    event.preventDefault();

    const form = event.target;
    const textarea = form.querySelector("textarea");
    const content = textarea.value.trim();
    const button = form.querySelector("button");

    if (!content) {
        return;
    }

    button.disabled = true;
    button.textContent = "Posting...";

    try {
        const response = await fetch(
            `/api/discussions/${discussionId}/replies`,
            {
                method: "POST",
                headers: {
                    "Content-Type": "application/json"
                },
                credentials: "include",
                body: JSON.stringify({ content })
            }
        );

        const result = await response.json();

        if (!response.ok) {
            throw new Error(result.error || "Could not post reply.");
        }

        await loadDiscussionReplies(discussionId);

    } catch (error) {
        alert(error.message);
        button.disabled = false;
        button.textContent = "Post reply";
    }
}

// Open the inline discussion editor.
function startDiscussionEdit(button) {
    const card = button.closest(".discussion-card");

    if (card.querySelector(".discussion-edit-form")) {
        return;
    }

    const discussionId = Number(card.dataset.discussionId);
    const title = card.querySelector(".discussion-card-header h3").textContent;
    const content = card.querySelector(".discussion-content").textContent;

    const form = document.createElement("form");
    form.className = "discussion-edit-form";

    form.innerHTML = `
        <label>Discussion title</label>
        <input
            type="text"
            name="title"
            maxlength="200"
            value="${escapeHTML(title)}"
            required
        >

        <label>Discussion content</label>
        <textarea name="content" rows="4" required>${escapeHTML(content)}</textarea>

        <div class="discussion-edit-actions">
            <button type="submit">Save changes</button>
            <button
                type="button"
                data-action="cancel-edit"
                data-kind="discussion"
            >
                Cancel
            </button>
        </div>
    `;

    card.classList.add("is-editing");
    card.querySelector(".discussion-card-footer").before(form);
}


// Open the inline reply editor.
function startReplyEdit(button) {
    const reply = button.closest(".discussion-reply");

    if (reply.querySelector(".reply-edit-form")) {
        return;
    }

    const content = reply.querySelector("p").textContent;
    const discussionId = Number(button.dataset.discussionId);

    const form = document.createElement("form");
    form.className = "reply-edit-form";
    form.dataset.discussionId = discussionId;

    form.innerHTML = `
        <textarea name="content" rows="3" required>${escapeHTML(content)}</textarea>

        <div class="discussion-edit-actions">
            <button type="submit">Save changes</button>
            <button
                type="button"
                data-action="cancel-edit"
                data-kind="reply"
            >
                Cancel
            </button>
        </div>
    `;

    reply.classList.add("is-editing");
    reply.appendChild(form);
}


// Discussion section
function setupDiscussionEvents(problemId) {
    const discussionForm = document.getElementById("discussion-form");
    const discussionList = document.getElementById("discussion-list");

    discussionForm.addEventListener("submit", event => {
        createDiscussion(event, problemId);
    });

    discussionList.addEventListener("click", async event => {
        const button = event.target.closest("[data-action]");

        if (!button) return;

        const action = button.dataset.action;

        // Cancel inline editing
        if (action === "cancel-edit") {
            const container = button.closest(
                button.dataset.kind === "discussion"
                    ? ".discussion-card"
                    : ".discussion-reply"
            );

            const form = button.closest("form");

            if (form) form.remove();
            if (container) container.classList.remove("is-editing");

            return;
        }

        const id = Number(button.dataset.id);

        if (!Number.isSafeInteger(id) || id <= 0) {
            return;
        }

        // Edit discussion
        if (action === "edit-discussion") {
            startDiscussionEdit(button);
            return;
        }

        // Delete discussion
        if (action === "delete-discussion") {
            if (!confirm("Are you sure you want to delete this discussion?")) {
                return;
            }

            try {
                const response = await fetch(`/api/discussions/${id}`, {
                    method: "DELETE",
                    credentials: "include"
                });

                const result = await response.json().catch(() => ({}));

                if (!response.ok) {
                    throw new Error(
                        result.error || "Failed to delete discussion."
                    );
                }

                await loadDiscussions(problemId);
            } catch (error) {
                alert(error.message);
            }

            return;
        }

        // Edit reply
        if (action === "edit-reply") {
            startReplyEdit(button);
            return;
        }

        // Delete reply
        if (action === "delete-reply") {
            if (!confirm("Are you sure you want to delete this reply?")) {
                return;
            }

            const discussionId = Number(button.dataset.discussionId);

            try {
                const response = await fetch(
                    `/api/discussion-replies/${id}`,
                    {
                        method: "DELETE",
                        credentials: "include"
                    }
                );

                const result = await response.json().catch(() => ({}));

                if (!response.ok) {
                    throw new Error(
                        result.error || "Failed to delete reply."
                    );
                }

                await loadDiscussionReplies(discussionId);
            } catch (error) {
                alert(error.message);
            }

            return;
        }

        // Show or hide replies
        if (action === "toggle-replies") {
            const repliesContainer = document.getElementById(`replies-${id}`);

            if (!repliesContainer.hidden) {
                repliesContainer.hidden = true;
                button.textContent = "View replies";
                return;
            }

            button.textContent = "Hide replies";
            await loadDiscussionReplies(id);
        }
    });

    discussionList.addEventListener("submit", async event => {
        const form = event.target;

        // Update discussion
        if (form.matches(".discussion-edit-form")) {
            event.preventDefault();

            const card = form.closest(".discussion-card");
            const discussionId = Number(card.dataset.discussionId);

            const title = form.elements.title.value.trim();
            const content = form.elements.content.value.trim();

            if (!title || !content) {
                alert("Title and content cannot be empty.");
                return;
            }

            const saveButton = form.querySelector('[type="submit"]');
            saveButton.disabled = true;

            try {
                const response = await fetch(
                    `/api/discussions/${discussionId}`,
                    {
                        method: "PUT",
                        headers: {
                            "Content-Type": "application/json"
                        },
                        credentials: "include",
                        body: JSON.stringify({ title, content })
                    }
                );

                const result = await response.json().catch(() => ({}));

                if (!response.ok) {
                    throw new Error(
                        result.error || "Failed to update discussion."
                    );
                }

                await loadDiscussions(problemId);
            } catch (error) {
                alert(error.message);
                saveButton.disabled = false;
            }

            return;
        }

        // Update reply
        if (form.matches(".reply-edit-form")) {
            event.preventDefault();

            const reply = form.closest(".discussion-reply");
            const editButton = reply.querySelector(
                '[data-action="edit-reply"]'
            );

            if (!editButton) return;

            const replyId = Number(editButton.dataset.id);
            const discussionId = Number(form.dataset.discussionId);
            const content = form.elements.content.value.trim();

            if (!content) {
                alert("Reply content cannot be empty.");
                return;
            }

            const saveButton = form.querySelector('[type="submit"]');
            saveButton.disabled = true;

            try {
                const response = await fetch(
                    `/api/discussion-replies/${replyId}`,
                    {
                        method: "PUT",
                        headers: {
                            "Content-Type": "application/json"
                        },
                        credentials: "include",
                        body: JSON.stringify({ content })
                    }
                );

                const result = await response.json().catch(() => ({}));

                if (!response.ok) {
                    throw new Error(
                        result.error || "Failed to update reply."
                    );
                }

                await loadDiscussionReplies(discussionId);
            } catch (error) {
                alert(error.message);
                saveButton.disabled = false;
            }

            return;
        }

        // Create reply
        if (form.matches(".discussion-reply-form")) {
            const discussionId = Number(form.dataset.id);

            if (!Number.isSafeInteger(discussionId) || discussionId <= 0) {
                return;
            }

            createDiscussionReply(event, discussionId);
        }
    });
}



async function loadProblem() {
    const container = document.getElementById("problem-container");
    const problemId = new URLSearchParams(window.location.search).get("id");

    if (!problemId || !/^\d+$/.test(problemId)) {
        container.innerHTML = '<div class="problem-load-error"><strong>Problem ID missing</strong><p>Open a problem from the Problems page to continue.</p><a href="/">Browse problems</a></div>';
        return;
    }

    try {
        const response = await fetch(`/api/problems/${encodeURIComponent(problemId)}`);
        if (!response.ok) throw new Error("Problem not found");
        const problem = await response.json();
        document.title = `${problem.title || `Problem ${problemId}`} · GrowCode`;

        const samples = Array.isArray(problem.sample_test_cases) ? problem.sample_test_cases : [];
        const difficulty = String(problem.difficulty || "Practice");
        const difficultyClass = difficulty.toLowerCase().replace(/[^a-z0-9]+/g, "-");

        container.innerHTML = `
            <div class="problem-breadcrumb"><a href="/">Problems</a><span aria-hidden="true">/</span><span>Problem ${escapeHTML(problem.problem_id || problemId)}</span></div>
            <div class="problem-workspace-grid">
                <article class="problem-panel">
                    <header class="problem-title-block">
                        <div class="problem-title-row"><span class="problem-number">${escapeHTML(problem.problem_id || problemId)}</span><span class="problem-type-label">CODING CHALLENGE</span></div>
                        <h1>${escapeHTML(problem.title || "Untitled problem")}</h1>
                        <div class="problem-meta"><span class="difficulty difficulty-${escapeHTML(difficultyClass)}">${escapeHTML(difficulty)}</span><span class="meta-dot"></span><span>Multiple languages supported</span></div>
                    </header>
                    <div class="problem-divider"></div>
                    <section class="problem-section">
                        <div class="section-title"><span class="section-icon" aria-hidden="true">≡</span><h2>Problem statement</h2></div>
                        <p class="problem-description">${escapeHTML(problem.description || "No description has been provided.")}</p>
                    </section>
                    <section class="problem-section constraints-section">
                        <div class="section-title"><span class="section-icon" aria-hidden="true">⌘</span><h2>Constraints</h2></div>
                        <div class="constraints-content">${escapeHTML(problem.constraints || "No additional constraints specified.")}</div>
                    </section>
                    <section class="problem-section expected-complexity-section">
                        <div class="section-title"><span class="section-icon" aria-hidden="true">ƒ</span><h2>Expected complexity</h2></div>
                        <div class="expected-complexity-values"><span>Time <strong>${escapeHTML(problem.expected_time_complexity || "Not configured")}</strong></span><span>Space <strong>${escapeHTML(problem.expected_space_complexity || "Not configured")}</strong></span></div>
                    </section>
                    <section class="problem-section examples-section">
                        <div class="section-title"><span class="section-icon" aria-hidden="true">▤</span><h2>Examples</h2><span class="section-count">${samples.length}</span></div>
                        ${samples.length ? samples.map((sample, index) => `
                            <div class="sample-test-case">
                                <div class="sample-heading"><strong>Example ${index + 1}</strong><span>Sample input</span></div>
                                <div class="sample-code-label">Input</div><pre>${escapeHTML(sample.input || "(empty)")}</pre>
                                <div class="sample-code-label">Output</div><pre>${escapeHTML(sample.expected_output ?? "")}</pre>
                            </div>`).join("") : '<p class="empty-samples">No sample test cases available for this problem.</p>'}
                    </section>
                </article>

                <aside class="editor-panel" aria-label="Solution editor">
                    <div class="editor-panel-header"><div><span class="editor-eyebrow">WORKSPACE</span><h2>Your solution</h2></div><span class="editor-ready"><span></span> Ready</span></div>
                    <div class="editor-toolbar">
                        <label for="language-select">Language</label>
                        <select id="language-select" aria-label="Programming language">
                            <option value="1">C++17</option>
                            <option value="2">Java</option>
                            <option value="3">Python 3</option>
                        </select>

                        <span class="editor-toolbar-spacer"></span>

                        <button type="button" class="editor-utility-button" id="copy-code-button">Copy code</button>
                        <button type="button" class="editor-utility-button" id="reset-code-button">Clear</button>
                        <button type="button" class="editor-utility-button" id="run-code-button">▶ Run Code</button>
                        <button type="button" id="submit-button">↑ Submit</button>
                    </div>
                    <div class="editor-surface"><div class="editor-surface-top"><span class="editor-file-dot"></span><span id="editor-language-label">C++17</span><span class="editor-file-extension">source</span></div><textarea id="code-editor" spellcheck="false" autocapitalize="off" autocomplete="off" autocorrect="off" aria-label="Source code editor" placeholder="Write your solution here…"></textarea><div class="editor-footer"><span>Tip: Press Tab to indent</span><span>Plain text editor</span></div></div>
                    <div class="custom-input-section">
                        <label for="custom-input">Custom Input</label>
                        <textarea
                            id="custom-input"
                            rows="5"
                            placeholder="Enter your input here..."
                            spellcheck="false"
                        ></textarea>
                    </div>

                    <div id="run-code-result" class="run-code-result" hidden>
                        <div class="run-code-result-header">
                            <strong id="run-code-status">Execution Result</strong>
                            <span id="run-code-time"></span>
                        </div>
                        <div>
                            <label>Output</label>
                            <pre id="run-code-output"></pre>
                        </div>
                        <div id="run-code-error-container" hidden>
                            <label>Error</label>
                            <pre id="run-code-error"></pre>
                        </div>
                    </div>

                    <div class="submit-area">
                        <p class="submit-note">Your solution will be checked against the test cases.</p>
                    </div>                    <div id="submission-message" class="submission-message" role="status" aria-live="polite"><span class="submission-status-icon" aria-hidden="true">i</span><span class="submission-message-copy"><strong>Ready to submit</strong><small>Your verdict will appear here after submission.</small></span></div>
                    <div class="editor-bottom-note"><span aria-hidden="true">⌁</span> Powered by the GrowCode execution engine</div>
                                </aside>
            </div>

            <!-- Discussion section -->
            <section class="discussion-section" id="discussion-section">

                <div class="discussion-header">
                    <div>
                        <span class="editor-eyebrow">COMMUNITY</span>
                        <h2>Discussions</h2>
                        <p>Ask questions and discuss approaches to this problem.</p>
                    </div>

                    <span id="discussion-count" class="discussion-count">0 discussions</span>
                </div>

                <div class="discussion-form-card" id="discussion-form-card">

                    <h3>Start a discussion</h3>

                    <form id="discussion-form">

                        <label for="discussion-title">Title</label>

                        <input
                            type="text"
                            id="discussion-title"
                            maxlength="200"
                            placeholder="What would you like to discuss?"
                            required
                        >

                        <label for="discussion-content">Your question</label>

                        <textarea
                            id="discussion-content"
                            rows="4"
                            placeholder="Explain your question or share your approach..."
                            required
                        ></textarea>

                        <div class="discussion-form-footer">
                            <p id="discussion-feedback" role="status"></p>

                            <button type="submit" id="discussion-submit">
                                Post discussion
                            </button>
                        </div>

                    </form>

                    <p id="discussion-login-message" hidden>
                        Please <a href="/login.html">log in</a> to start a discussion.
                    </p>

                </div>

                <div id="discussion-list" class="discussion-list">
                    <p class="discussion-loading">Loading discussions...</p>
                </div>

            </section>`;

        setupEditor();

        document.getElementById("run-code-button")
            .addEventListener("click", runCode);
        
        document.getElementById("submit-button")
            .addEventListener("click", () => submitSolution(problemId));
        // Initialize discussions.
        await checkDiscussionAuth();
        setupDiscussionEvents(problemId);
        await loadDiscussions(problemId);
    } catch (error) {
        console.error(error);
        container.innerHTML = '<div class="problem-load-error"><strong>Unable to load this problem</strong><p>Check your connection and try again.</p><button type="button" onclick="location.reload()">Try again</button></div>';
    }
}

document.addEventListener("DOMContentLoaded", loadProblem);
