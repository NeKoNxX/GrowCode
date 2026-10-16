async function loadProblems() {
    const container = document.getElementById("problems-container");
    const count = document.getElementById("problem-count");

    if (!container || !count) return;

    try {
        const response = await fetch("/api/problems", {
            method: "GET",
            headers: { "Accept": "application/json" }
        });

        if (!response.ok) {
            throw new Error(`Failed to fetch problems: HTTP ${response.status}`);
        }

        const problems = await response.json();

        if (!Array.isArray(problems)) {
            throw new Error("Invalid problems response: expected an array.");
        }

        count.textContent = `${problems.length} problems`;
        container.replaceChildren();

        if (problems.length === 0) {
            const emptyMessage = document.createElement("p");
            emptyMessage.textContent = "No problems available.";
            container.appendChild(emptyMessage);
            return;
        }

        problems.forEach(problem => {
            const card = document.createElement("div");
            card.className = "problem-card";
            card.tabIndex = 0;
            card.setAttribute("role", "link");
            card.setAttribute(
                "aria-label",
                `Open problem ${problem.problem_id}: ${problem.title}`
            );

            const title = document.createElement("h3");
            title.textContent = `${problem.problem_id}. ${problem.title}`;

            const description = document.createElement("p");
            description.textContent = problem.description;

            const difficulty = document.createElement("span");
            difficulty.className = "difficulty";
            difficulty.textContent = problem.difficulty;

            card.append(title, description, difficulty);

            const openProblem = () => {
                window.location.href =
                    `/problem.html?id=${encodeURIComponent(problem.problem_id)}`;
            };

            card.addEventListener("click", openProblem);

            card.addEventListener("keydown", event => {
                if (event.key === "Enter" || event.key === " ") {
                    event.preventDefault();
                    openProblem();
                }
            });

            container.appendChild(card);
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

document.addEventListener("DOMContentLoaded", loadProblems);
