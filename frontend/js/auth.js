document.addEventListener("DOMContentLoaded", async function () {
    const authLink = document.getElementById("auth-link");
    const registerLink = document.getElementById("register-link");
    const adminLink = document.getElementById("admin-link");

    // Pages without the shared authentication navigation can safely skip it.
    if (!authLink) {
        return;
    }

    // Start with a logged-out state until the server confirms the session.
    function showLoggedOutState() {
        authLink.textContent = "Login";
        authLink.href = "/login.html";
        authLink.removeAttribute("aria-disabled");

        if (registerLink) {
            registerLink.hidden = false;
        }

        if (adminLink) {
            adminLink.hidden = true;
        }
    }

    function showLoggedInState(role) {
        authLink.textContent = "Logout";
        authLink.href = "#logout";

        if (registerLink) {
            registerLink.hidden = true;
        }

        if (adminLink) {
            adminLink.hidden = role !== "ADMIN";
        }
    }

    try {
        const response = await fetch("/api/session", {
            method: "GET",
            credentials: "include",
            cache: "no-store"
        });

        if (!response.ok) {
            showLoggedOutState();
            return;
        }

        const session = await response.json();

        if (session.logged_in) {
            showLoggedInState(session.role);
        } else {
            showLoggedOutState();
        }
    } catch (error) {
        // If the server cannot be reached, do not assume the user is logged in.
        showLoggedOutState();
        console.error("Unable to check login session:", error);
    }

    // Invalidate the session on the server when Logout is clicked.
    authLink.addEventListener("click", async function (event) {
        if (authLink.textContent.trim() !== "Logout") {
            return;
        }

        event.preventDefault();

        authLink.textContent = "Logging out...";
        authLink.setAttribute("aria-disabled", "true");

        try {
            const response = await fetch("/api/logout", {
                method: "POST",
                credentials: "include",
                cache: "no-store"
            });

            if (!response.ok) {
                throw new Error("Logout request failed");
            }

            // Redirect only after the server confirms logout.
            window.location.replace("/");
        } catch (error) {
            console.error("Logout failed:", error);
            authLink.textContent = "Logout";
            authLink.href = "#logout";
            authLink.removeAttribute("aria-disabled");
            alert("Unable to log out. Please try again.");
        }
    });
});
