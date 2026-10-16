async function getCurrentUserId() {
    try {
        const response = await fetch("/api/session", {
            method: "GET",
            credentials: "include",
            cache: "no-store",
            headers: { "Accept": "application/json" }
        });

        if (!response.ok) return null;

        const session = await response.json();

        if (!session.logged_in || session.user_id == null) {
            return null;
        }

        return Number(session.user_id);
    } catch (error) {
        console.error("Unable to check login session:", error);
        return null;
    }
}

async function loadLeaderboard() {
    const message = document.getElementById("leaderboard-message");
    const tableBody = document.getElementById("leaderboard-body");

    try {
        // Fetch the session and leaderboard independently.
        const [currentUserId, response] = await Promise.all([
            getCurrentUserId(),
            fetch("/api/leaderboard", {
                method: "GET",
                credentials: "include",
                headers: { "Accept": "application/json" }
            })
        ]);

        const result = await response.json();

        if (!response.ok) {
            throw new Error(result.error || "Unable to load leaderboard.");
        }

        const users = Array.isArray(result) ? result : result.users;

        if (!Array.isArray(users)) {
            throw new Error("The server returned an invalid leaderboard response.");
        }

        document.getElementById("participant-count").textContent = users.length;

        if (users.length > 0) {
            document.getElementById("top-user").textContent =
                users[0].name || users[0].username || "—";
            document.getElementById("top-solved").textContent =
                Number(users[0].problems_solved || 0);
        } else {
            document.getElementById("top-user").textContent = "—";
            document.getElementById("top-solved").textContent = "0";
        }

        tableBody.replaceChildren();

        if (users.length === 0) {
            const row = document.createElement("tr");
            const cell = document.createElement("td");
            cell.colSpan = 4;
            cell.className = "leaderboard-empty";
            cell.textContent = "No users to rank yet. Start solving problems!";
            row.appendChild(cell);
            tableBody.appendChild(row);
        } else {
            users.forEach((user, index) => {
                const row = document.createElement("tr");

                if (index < 3) {
                    row.classList.add(`leaderboard-rank-${index + 1}`);
                }

                // Highlight only the authenticated user's row.
                const isCurrentUser =
                    currentUserId !== null &&
                    user.user_id != null &&
                    Number(user.user_id) === currentUserId;

                if (isCurrentUser) {
                    row.classList.add("current-user-row");
                }

                const rankCell = document.createElement("td");
                rankCell.className = "leaderboard-rank";
                rankCell.textContent = user.rank ?? index + 1;

                const userCell = document.createElement("td");
                const name = document.createElement("span");
                name.className = "leaderboard-user-name";
                name.textContent = user.name || user.username || "Unknown user";
                userCell.appendChild(name);

                if (isCurrentUser) {
                    const badge = document.createElement("span");
                    badge.className = "current-user-badge";
                    badge.textContent = "You";
                    userCell.appendChild(document.createTextNode(" "));
                    userCell.appendChild(badge);
                }

                if (user.username && user.name && user.username !== user.name) {
                    const username = document.createElement("span");
                    username.className = "leaderboard-username";
                    username.textContent = `@${user.username}`;
                    userCell.appendChild(username);
                }

                const solvedCell = document.createElement("td");
                solvedCell.textContent = Number(user.problems_solved || 0);

                const attemptsCell = document.createElement("td");
                attemptsCell.textContent = Number(user.total_attempts || 0);

                row.append(rankCell, userCell, solvedCell, attemptsCell);
                tableBody.appendChild(row);
            });
        }

        message.textContent =
            "Rankings are ordered by problems solved, then by fewer attempts.";
        message.classList.remove("leaderboard-error");
    } catch (error) {
        console.error("Failed to load leaderboard:", error);
        message.textContent =
            error.message || "Unable to connect to the server.";
        message.classList.add("leaderboard-error");

        tableBody.replaceChildren();

        const row = document.createElement("tr");
        const cell = document.createElement("td");
        cell.colSpan = 4;
        cell.className = "leaderboard-empty";
        cell.textContent = "Leaderboard data could not be loaded.";
        row.appendChild(cell);
        tableBody.appendChild(row);
    }
}

document.addEventListener("DOMContentLoaded", loadLeaderboard);
