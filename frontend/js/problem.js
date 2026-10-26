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
                    <span>AI-estimated time</span>
                    <strong>${escapeHTML(time)}</strong>
                    <small>Expected: ${escapeHTML(expectedTime)}</small>
                </article>

                <article>
                    <span>AI-estimated space</span>
                    <strong>${escapeHTML(space)}</strong>
                    <small>Expected: ${escapeHTML(expectedSpace)}</small>
                </article>

                <article>
                    <span>Recommended Algorithm</span>
                    <strong>${escapeHTML(recommendedAlgorithm)}</strong>
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
                            ${escapeHTML(analysisError ||
                                "Complexity analysis unavailable")}.
                            Your judge verdict is unchanged.
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
                            <option value="1">C++17</option><option value="2">Java</option><option value="3">Python 3</option>
                        </select>
                        <span class="editor-toolbar-spacer"></span><button type="button" class="editor-utility-button" id="copy-code-button">Copy code</button><button type="button" class="editor-utility-button" id="reset-code-button">Clear</button>
                    </div>
                    <div class="editor-surface"><div class="editor-surface-top"><span class="editor-file-dot"></span><span id="editor-language-label">C++17</span><span class="editor-file-extension">source</span></div><textarea id="code-editor" spellcheck="false" autocapitalize="off" autocomplete="off" autocorrect="off" aria-label="Source code editor" placeholder="Write your solution here…"></textarea><div class="editor-footer"><span>Tip: Press Tab to indent</span><span>Plain text editor</span></div></div>
                    <div class="submit-area"><p class="submit-note">Your solution will be checked against the test cases.</p><button id="submit-button" type="button"><span aria-hidden="true">↑</span> Submit solution</button></div>
                    <div id="submission-message" class="submission-message" role="status" aria-live="polite"><span class="submission-status-icon" aria-hidden="true">i</span><span class="submission-message-copy"><strong>Ready to submit</strong><small>Your verdict will appear here after submission.</small></span></div>
                    <div class="editor-bottom-note"><span aria-hidden="true">⌁</span> Powered by the GrowCode execution engine</div>
                </aside>
            </div>`;

        setupEditor();
        document.getElementById("submit-button").addEventListener("click", () => submitSolution(problemId));
    } catch (error) {
        console.error(error);
        container.innerHTML = '<div class="problem-load-error"><strong>Unable to load this problem</strong><p>Check your connection and try again.</p><button type="button" onclick="location.reload()">Try again</button></div>';
    }
}

document.addEventListener("DOMContentLoaded", loadProblem);
