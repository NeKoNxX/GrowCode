async function loadProblems() {
    const container = document.getElementById("problems-container");
    const count = document.getElementById("problem-count");

    if (!container || !count) {
        console.error("Problem list container or problem count element not found.");
        return;
    }

    try {
        const response = await fetch("/api/problems", {
            method: "GET",
            headers: {
                "Accept": "application/json"
            }
        });

        if (!response.ok) {
            throw new Error(`Failed to fetch problems: HTTP ${response.status}`);
        }

        const problems = await response.json();

        if (!Array.isArray(problems)) {
            throw new Error("Invalid problems response: expected an array.");
        }

        // Update the total number of problems.
        count.textContent = `${problems.length} problems`;

        // Clear the existing problem list.
        container.replaceChildren();

        if (problems.length === 0) {
            const emptyMessage = document.createElement("p");
            emptyMessage.textContent = "No problems available.";
            container.appendChild(emptyMessage);
            return;
        }

        // Create one horizontal row for each problem.
        problems.forEach((problem, index) => {
            const row = document.createElement("div");

            row.className = "problem-card";
            row.tabIndex = 0;
            row.setAttribute("role", "link");

            // Display a sequential number, not the database ID.
            const displayNumber = index + 1;

            row.setAttribute(
                "aria-label",
                `Open problem ${displayNumber}: ${problem.title}`
            );

            // Display only the sequential number and title.
            const title = document.createElement("h3");
            title.textContent = `${displayNumber}. ${problem.title}`;

            row.appendChild(title);

            // Use the actual database ID when opening the problem.
            const openProblem = () => {
                window.location.href =
                    `/problem.html?id=${encodeURIComponent(problem.problem_id)}`;
            };

            row.addEventListener("click", openProblem);

            // Support keyboard navigation.
            row.addEventListener("keydown", event => {
                if (event.key === "Enter" || event.key === " ") {
                    event.preventDefault();
                    openProblem();
                }
            });

            container.appendChild(row);
        });

    } catch (error) {
        console.error("Failed to load problems:", error);

        count.textContent = "";

        const errorMessage = document.createElement("p");
        errorMessage.textContent =
            "Unable to load problems from the server.";

        container.replaceChildren(errorMessage);
    }
}

// Load problems after the page has finished parsing.
document.addEventListener("DOMContentLoaded", loadProblems);