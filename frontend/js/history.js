async function loadSubmissionHistory() {
    const message = document.getElementById("history-message");
    const container = document.getElementById("submission-history");

    try {
        const response = await fetch(
            "/api/submissions",
            {
                credentials: "include"
            }
        );

        const result = await response.json();

        if (!response.ok) {
            message.textContent =
                result.error || "Unable to load submissions.";
            return;
        }

        if (result.length === 0) {
            message.textContent =
                "You have not made any submissions yet.";
            return;
        }

        message.textContent = "";

        container.innerHTML = `
            <table class="submission-table">

                <thead>
                    <tr>
                        <th>Submission ID</th>
                        <th>Problem</th>
                        <th>Language</th>
                        <th>Status</th>
                        <th>Submitted At</th>
                    </tr>
                </thead>

                <tbody>
                    ${result.map(submission => `
                        <tr>
                            <td>${submission.submission_id}</td>

                            <td>
                                ${submission.problem_title}
                            </td>

                            <td>
                                ${submission.language}
                            </td>

                            <td>
                                <span class="status ${submission.status.toLowerCase()}">
                                    ${submission.status}
                                </span>
                            </td>

                            <td>
                                ${submission.submitted_at}
                            </td>
                        </tr>
                    `).join("")}
                </tbody>

            </table>
        `;

    } catch (error) {
        console.error(error);

        message.textContent =
            "Unable to connect to the server.";
    }
}


async function loadSubmissionHistory() {
    const message = document.getElementById("history-message");
    const container = document.getElementById("submission-history");

    try {
        const response = await fetch("/api/submissions", {
            credentials: "include"
        });

        const result = await response.json();

        if (!response.ok) {
            message.textContent =
                result.error || "Unable to load submissions.";
            return;
        }

        if (!Array.isArray(result) || result.length === 0) {
            message.textContent =
                "You have not made any submissions yet.";
            return;
        }

        message.textContent = "";
        container.replaceChildren();

        const table = document.createElement("table");
        table.className = "submission-table";

        const headers = [
            "Submission ID",
            "Problem",
            "Language",
            "Status",
            "Submitted At",
            "Source Code"
        ];

        const thead = document.createElement("thead");
        const headerRow = document.createElement("tr");

        headers.forEach(text => {
            const th = document.createElement("th");
            th.textContent = text;
            headerRow.appendChild(th);
        });

        thead.appendChild(headerRow);
        table.appendChild(thead);

        const tbody = document.createElement("tbody");

        result.forEach(submission => {
            const row = document.createElement("tr");

            const values = [
                submission.submission_id,
                submission.problem_title,
                submission.language,
                submission.status,
                submission.submitted_at
            ];

            values.forEach((value, index) => {
                const cell = document.createElement("td");

                if (index === 3) {
                    const status = document.createElement("span");
                    status.className =
                        "status " + String(value || "").toLowerCase();
                    status.textContent = value ?? "";
                    cell.appendChild(status);
                } else {
                    cell.textContent = value ?? "";
                }

                row.appendChild(cell);
            });

            const codeCell = document.createElement("td");
            const viewButton = document.createElement("button");

            viewButton.type = "button";
            viewButton.textContent = "View Code";
            viewButton.addEventListener("click", () => {
                viewSubmissionCode(submission.submission_id);
            });

            codeCell.appendChild(viewButton);
            row.appendChild(codeCell);
            tbody.appendChild(row);
        });

        table.appendChild(tbody);
        container.appendChild(table);

    } catch (error) {
        console.error(error);
        message.textContent = "Unable to connect to the server.";
    }
}

async function viewSubmissionCode(submissionId) {
    const modal = document.getElementById("code-modal");
    const codeElement = document.getElementById("submission-code");
    const codeMessage = document.getElementById("code-message");

    modal.hidden = false;
    codeElement.textContent = "";
    codeMessage.textContent = "Loading source code...";

    try {
        const response = await fetch(
            `/api/submissions/${encodeURIComponent(submissionId)}/code`,
            { credentials: "include" }
        );

        const result = await response.json();

        if (!response.ok) {
            codeMessage.textContent =
                result.error || "Unable to load source code.";
            return;
        }

        codeElement.textContent = result.code ?? "";
        codeMessage.textContent = "";
    } catch (error) {
        console.error(error);
        codeMessage.textContent = "Unable to connect to the server.";
    }
}

document.addEventListener("DOMContentLoaded", () => {
    const modal = document.getElementById("code-modal");

    document.getElementById("close-code-modal").addEventListener(
        "click",
        () => { modal.hidden = true; }
    );

    document.getElementById("copy-submission-code").addEventListener(
        "click",
        async () => {
            const code = document.getElementById("submission-code").textContent;

            try {
                await navigator.clipboard.writeText(code);
                document.getElementById("code-message").textContent =
                    "Code copied.";
            } catch (error) {
                document.getElementById("code-message").textContent =
                    "Copy failed. Select and copy the code manually.";
            }
        }
    );

    modal.addEventListener("click", event => {
        if (event.target === modal) modal.hidden = true;
    });

    loadSubmissionHistory();
});


loadSubmissionHistory();