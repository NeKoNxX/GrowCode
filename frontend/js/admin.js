(() => {
    "use strict";

    document.addEventListener("DOMContentLoaded", () => {
        const $ = (id) => document.getElementById(id);
        const state = { problems: [], selectedProblemId: null, testCases: [] };
        const adminMessage = $("admin-message");
        const problemList = $("problem-list");
        const testCaseList = $("test-case-list");
        const problemIdInput = $("problem-id");

        async function api(url, options = {}) {
            const response = await fetch(url, {
                credentials: "include",
                ...options,
                headers: { ...(options.headers || {}) }
            });
            let payload = null;
            const contentType = response.headers.get("content-type") || "";
            if (contentType.includes("application/json")) {
                try { payload = await response.json(); } catch (_) { payload = null; }
            } else {
                try { payload = await response.text(); } catch (_) { payload = null; }
            }
            if (!response.ok) {
                const message = payload && typeof payload === "object" && payload.error
                    ? payload.error
                    : (typeof payload === "string" && payload.trim() ? payload : `Request failed (${response.status})`);
                throw new Error(message);
            }
            return payload;
        }

        function notify(message, type = "success") {
            const region = $("toast-region");
            const toast = document.createElement("div");
            toast.className = `toast ${type}`;
            toast.textContent = message;
            region.appendChild(toast);
            window.setTimeout(() => toast.remove(), 4200);
        }

        function setFeedback(id, message, type = "") {
            const el = $(id);
            if (!el) return;
            el.textContent = message;
            el.dataset.state = type;
        }

        function setAccess(stateName, message) {
            $("admin-status").dataset.state = stateName;
            adminMessage.textContent = message;
        }

        function makeButton(label, className, onClick) {
            const button = document.createElement("button");
            button.type = "button";
            button.className = `btn ${className || "btn-secondary"}`;
            button.textContent = label;
            button.addEventListener("click", onClick);
            return button;
        }

        function updateStats() {
            $("stat-problems").textContent = String(state.problems.length);
            $("stat-easy").textContent = String(state.problems.filter(p => String(p.difficulty).toUpperCase() === "EASY").length);
            $("stat-medium").textContent = String(state.problems.filter(p => String(p.difficulty).toUpperCase() === "MEDIUM").length);
            $("stat-hard").textContent = String(state.problems.filter(p => String(p.difficulty).toUpperCase() === "HARD").length);
        }

        function renderProblems() {
            const query = $("problem-search").value.trim().toLowerCase();
            const difficulty = $("difficulty-filter").value;
            const sort = $("sort-problems").value;
            let filtered = state.problems.filter(p => {
                const matchQuery = !query || [p.problem_id, p.title, p.description, p.recommended_algorithm, p.constraints]
                    .some(value => String(value ?? "").toLowerCase().includes(query));
                const matchDifficulty = difficulty === "ALL" || String(p.difficulty).toUpperCase() === difficulty;
                return matchQuery && matchDifficulty;
            });
            if (sort === "id-desc") filtered.sort((a, b) => Number(b.problem_id) - Number(a.problem_id));
            else if (sort === "title") filtered.sort((a, b) => String(a.title).localeCompare(String(b.title)));
            else filtered.sort((a, b) => Number(a.problem_id) - Number(b.problem_id));

            $("problem-count").textContent = `${filtered.length} of ${state.problems.length} problems`;
            problemList.replaceChildren();
            if (!filtered.length) {
                const empty = document.createElement("div");
                empty.className = "empty-state";
                empty.textContent = state.problems.length ? "No problems match your search/filter." : "No problems yet. Create your first problem above.";
                problemList.appendChild(empty);
                return;
            }

            filtered.forEach(problem => {
                const card = document.createElement("article");
                card.className = "problem-card";
                const head = document.createElement("div");
                head.className = "problem-card-header";
                const titleWrap = document.createElement("div");
                const id = document.createElement("span");
                id.className = "problem-id";
                id.textContent = `PROBLEM #${problem.problem_id}`;
                const title = document.createElement("h3");
                title.textContent = problem.title || "Untitled problem";
                titleWrap.append(id, title);
                const diff = document.createElement("span");
                const difficultyName = String(problem.difficulty || "EASY").toLowerCase();
                diff.className = `difficulty-pill ${difficultyName}`;
                diff.textContent = difficultyName.toUpperCase();
                head.append(titleWrap, diff);

                const description = document.createElement("p");
                description.className = "problem-card-description";
                description.textContent = problem.description || "No description provided.";

                const chips = document.createElement("div");
                chips.className = "problem-card-meta";
                const time = document.createElement("span"); time.className = "meta-chip"; time.textContent = `Time ${problem.expected_time_complexity || "—"}`;
                const space = document.createElement("span"); space.className = "meta-chip"; space.textContent = `Space ${problem.expected_space_complexity || "—"}`;
                const algo = document.createElement("span"); algo.className = "meta-chip"; algo.textContent = problem.recommended_algorithm || "Algorithm not set";
                chips.append(time, space, algo);

                const actions = document.createElement("div");
                actions.className = "problem-card-actions";
                actions.append(
                    makeButton("Manage tests", "btn-primary", async () => selectProblem(problem)),
                    makeButton("Delete", "btn-danger", async (event) => deleteProblem(problem))
                );
                card.append(head, description, chips, actions);
                problemList.appendChild(card);
            });
        }

        async function loadProblems() {
            problemList.innerHTML = '<div class="empty-state">Loading problem library…</div>';
            try {
                const data = await api("/api/admin/problems", { cache: "no-store" });
                if (!Array.isArray(data)) throw new Error("Unexpected problem list response.");
                state.problems = data;
                updateStats();
                renderProblems();
                if (state.selectedProblemId && !state.problems.some(p => Number(p.problem_id) === Number(state.selectedProblemId))) {
                    clearSelectedProblem();
                }
            } catch (error) {
                problemList.innerHTML = "";
                const box = document.createElement("div"); box.className = "empty-state"; box.textContent = error.message || "Unable to load problems.";
                problemList.appendChild(box);
                setAccess("error", "Could not load problem library.");
                notify(error.message || "Problem load failed", "error");
            }
        }

        async function selectProblem(problem) {
            state.selectedProblemId = Number(problem.problem_id);
            problemIdInput.value = String(problem.problem_id);
            $("selected-problem-label").textContent = `#${problem.problem_id} · ${problem.title}`;
            setFeedback("test-case-message", "");
            await loadTestCases(problem.problem_id);
            $("test-case-panel").scrollIntoView({ behavior: "smooth", block: "start" });
        }

        function clearSelectedProblem() {
            state.selectedProblemId = null;
            problemIdInput.value = "";
            $("selected-problem-label").textContent = "None selected";
            testCaseList.innerHTML = '<div class="empty-state">Select a problem to load its test cases.</div>';
        }

        async function loadTestCases(problemId) {
            testCaseList.innerHTML = '<div class="empty-state">Loading test cases…</div>';
            try {
                const data = await api(`/api/admin/test-cases/${encodeURIComponent(problemId)}`, { cache: "no-store" });
                if (!Array.isArray(data)) throw new Error("Unexpected test-case response.");
                state.testCases = data;
                testCaseList.replaceChildren();
                if (!data.length) {
                    const empty = document.createElement("div"); empty.className = "empty-state"; empty.textContent = "No test cases have been added to this problem yet.";
                    testCaseList.appendChild(empty); return;
                }
                data.forEach((testCase, index) => {
                    const card = document.createElement("article"); card.className = "test-case-card";
                    const heading = document.createElement("h3"); heading.textContent = `Test case ${index + 1} · ID ${testCase.test_case_id}`;
                    const tag = document.createElement("span"); tag.className = `case-type ${testCase.is_sample ? "sample" : "hidden"}`; tag.textContent = testCase.is_sample ? "SAMPLE / PUBLIC" : "HIDDEN / JUDGE";
                    const inputTitle = document.createElement("p"); inputTitle.textContent = "Input";
                    const input = document.createElement("pre"); input.textContent = testCase.input ?? "(empty input)";
                    const outputTitle = document.createElement("p"); outputTitle.textContent = "Expected output";
                    const output = document.createElement("pre"); output.textContent = testCase.expected_output ?? "";
                    const remove = makeButton("Delete test case", "btn-danger", async () => deleteTestCase(testCase, problemId));
                    card.append(heading, tag, inputTitle, input, outputTitle, output, remove);
                    testCaseList.appendChild(card);
                });
            } catch (error) {
                testCaseList.replaceChildren(); const box = document.createElement("div"); box.className = "empty-state"; box.textContent = error.message || "Unable to load test cases."; testCaseList.appendChild(box);
                setFeedback("test-case-message", error.message || "Could not load test cases.", "error");
            }
        }

        async function deleteProblem(problem) {
            if (!window.confirm(`Delete problem #${problem.problem_id} “${problem.title}”? Related test cases may be deleted by database cascade. This cannot be undone.`)) return;
            try {
                await api(`/api/admin/problems/${problem.problem_id}`, { method: "DELETE" });
                if (Number(state.selectedProblemId) === Number(problem.problem_id)) clearSelectedProblem();
                notify(`Deleted problem #${problem.problem_id}.`);
                await loadProblems();
                await loadAuditLogs();
            } catch (error) { notify(error.message || "Problem deletion failed.", "error"); }
        }

        async function deleteTestCase(testCase, problemId) {
            if (!window.confirm(`Delete test case #${testCase.test_case_id}? This cannot be undone.`)) return;
            try {
                await api(`/api/admin/test-cases/${testCase.test_case_id}`, { method: "DELETE" });
                notify(`Deleted test case #${testCase.test_case_id}.`);
                await loadTestCases(problemId);
            } catch (error) { notify(error.message || "Test-case deletion failed.", "error"); }
        }

        $("problem-form").addEventListener("submit", async event => {
            event.preventDefault();
            const submit = event.submitter || $("problem-form").querySelector('[type="submit"]');
            submit.disabled = true;
            setFeedback("problem-message", "Creating problem…");
            const form = $("problem-form");
            const body = new URLSearchParams(new FormData(form));
            try {
                const result = await api("/api/admin/problems", {
                    method: "POST",
                    headers: { "Content-Type": "application/x-www-form-urlencoded;charset=UTF-8" },
                    body: body.toString()
                });
                setFeedback("problem-message", `Created successfully · Problem ID ${result.problem_id}`, "success");
                notify(`Problem #${result.problem_id} created.`);
                form.reset();
                await loadProblems();
                await loadAuditLogs();
                const created = state.problems.find(p => Number(p.problem_id) === Number(result.problem_id));
                if (created) await selectProblem(created);
            } catch (error) {
                setFeedback("problem-message", error.message || "Could not create problem.", "error");
                notify(error.message || "Problem creation failed.", "error");
            } finally { submit.disabled = false; }
        });

        $("test-case-form").addEventListener("submit", async event => {
            event.preventDefault();
            const submit = event.submitter || $("test-case-form").querySelector('[type="submit"]');
            submit.disabled = true;
            const problemId = Number(problemIdInput.value);
            if (!Number.isInteger(problemId) || problemId < 1) {
                setFeedback("test-case-message", "Choose a valid problem first.", "error"); submit.disabled = false; return;
            }
            setFeedback("test-case-message", "Adding test case…");
            const body = new URLSearchParams(new FormData($("test-case-form")));
            try {
                const result = await api("/api/admin/test-cases", {
                    method: "POST",
                    headers: { "Content-Type": "application/x-www-form-urlencoded;charset=UTF-8" },
                    body: body.toString()
                });
                setFeedback("test-case-message", `Added test case · ID ${result.test_case_id}`, "success");
                notify(`Test case #${result.test_case_id} added.`);
                $("input").value = ""; $("expected-output").value = "";
                await loadTestCases(problemId);
            } catch (error) {
                setFeedback("test-case-message", error.message || "Could not add test case.", "error");
                notify(error.message || "Test-case creation failed.", "error");
            } finally { submit.disabled = false; }
        });

        $("problem-search").addEventListener("input", renderProblems);
        $("difficulty-filter").addEventListener("change", renderProblems);
        $("sort-problems").addEventListener("change", renderProblems);
        $("refresh-problems").addEventListener("click", loadProblems);
        $("clear-selected-problem").addEventListener("click", clearSelectedProblem);
        document.querySelectorAll("[data-toggle]").forEach(button => button.addEventListener("click", () => {
            const form = $(button.dataset.toggle);
            if (!form) return;
            form.hidden = !form.hidden;
            button.textContent = form.hidden ? "Expand" : "Collapse";
        }));


        // ---------- Additional admin tools ----------
        let selectedAdminUserId = null;
        let selectedAdminSubmissionId = null;
        let selectedAdminLanguageId = null;

        function cell(row, value) {
            const td = document.createElement("td");
            td.textContent = value == null ? "—" : String(value);
            row.appendChild(td);
            return td;
        }

        function table(headers, rows) {
            const wrap = document.createElement("div");
            wrap.className = "admin-table-wrap";
            const tbl = document.createElement("table");
            tbl.className = "admin-table";
            const thead = document.createElement("thead");
            const tr = document.createElement("tr");
            headers.forEach(h => { const th=document.createElement("th"); th.textContent=h; tr.appendChild(th); });
            thead.appendChild(tr); tbl.appendChild(thead);
            const tbody = document.createElement("tbody");
            rows.forEach(row => tbody.appendChild(row));
            tbl.appendChild(tbody); wrap.appendChild(tbl);
            return wrap;
        }

        async function loadAdminUsers() {
            const list = $("admin-users-list");
            const search = $("admin-user-search").value.trim();
            list.replaceChildren();
            const loading = document.createElement("div"); loading.className="empty-state"; loading.textContent="Loading users…"; list.appendChild(loading);
            try {
                const users = await api(`/api/admin/users?search=${encodeURIComponent(search)}&limit=100&offset=0`, {cache:"no-store"});
                if (!Array.isArray(users)) throw new Error("Unexpected users response.");
                if (!users.length) { list.innerHTML='<div class="empty-state">No matching users.</div>'; return; }
                const rows = users.map(user => {
                    const tr=document.createElement("tr");
                    cell(tr,user.user_id); cell(tr,user.name); cell(tr,user.username); cell(tr,user.email);
                    cell(tr,user.role); cell(tr,user.is_active ? "Active":"Disabled"); cell(tr,user.submission_count);
                    const actions=document.createElement("td"); actions.className="table-actions";
                    const submissions=makeButton("Submissions","btn-secondary",()=>loadUserSubmissions(user));
                    const toggle=makeButton(user.is_active ? "Disable":"Enable",user.is_active?"btn-warning":"btn-primary",()=>toggleUser(user));
                    const del=makeButton("Delete","btn-danger",()=>deleteAdminUser(user));
                    actions.append(submissions,toggle,del); tr.appendChild(actions); return tr;
                });
                list.replaceChildren(table(["ID","Name","Username","Email","Role","Status","Submissions","Actions"],rows));
                setFeedback("admin-user-message",`${users.length} user(s) loaded.`);
            } catch(err) {
                list.replaceChildren(); const box=document.createElement("div"); box.className="empty-state error"; box.textContent=err.message; list.appendChild(box);
                setFeedback("admin-user-message",err.message,"error");
            }
        }

        async function toggleUser(user) {
            const nextActive = !Boolean(user.is_active);
            if (!confirm(`${nextActive ? "Enable" : "Disable"} account ${user.username}?`)) return;
            try {
                await api(`/api/admin/users/${user.user_id}/status`, {
                    method:"PUT", headers:{"Content-Type":"application/json"},
                    body:JSON.stringify({active:nextActive})
                });
                notify(`Account ${user.username} ${nextActive?"enabled":"disabled"}.`);
                await loadAdminUsers();
            } catch(err) { notify(err.message,"error"); }
        }

        async function deleteAdminUser(user) {
            if (!confirm(`Permanently delete user ${user.username} (ID ${user.user_id})? This may be blocked by related database records and cannot be undone.`)) return;
            try {
                await api(`/api/admin/users/${user.user_id}`,{method:"DELETE"});
                notify(`Deleted user ${user.username}.`);
                if (selectedAdminUserId === Number(user.user_id)) {
                    selectedAdminUserId=null;
                    $("admin-submissions-user-label").textContent="Select a user from User Management.";
                    $("admin-submissions-list").innerHTML='<div class="empty-state">No user selected.</div>';
                }
                await loadAdminUsers();
            } catch(err) { notify(err.message,"error"); }
        }

        async function loadUserSubmissions(user) {
            selectedAdminUserId=Number(user.user_id);
            $("admin-submissions-user-label").textContent=`Submissions for ${user.username} (user #${user.user_id})`;
            const list=$("admin-submissions-list");
            list.innerHTML='<div class="empty-state">Loading submissions…</div>';
            $("user-submissions-panel").scrollIntoView({behavior:"smooth",block:"start"});
            try {
                const data=await api(`/api/admin/users/${user.user_id}/submissions?limit=100&offset=0`,{cache:"no-store"});
                if (!Array.isArray(data)) throw new Error("Unexpected submissions response.");
                if (!data.length) { list.innerHTML='<div class="empty-state">No submissions for this user.</div>'; return; }
                const rows=data.map(s=>{
                    const tr=document.createElement("tr");
                    cell(tr,s.submission_id); cell(tr,s.problem_title); cell(tr,s.language); cell(tr,s.status); cell(tr,s.submitted_at);
                    const td=document.createElement("td");
                    td.appendChild(makeButton("View code","btn-primary",()=>loadSubmission(s.submission_id)));
                    tr.appendChild(td); return tr;
                });
                list.replaceChildren(table(["Submission","Problem","Language","Verdict","Submitted","Action"],rows));
            } catch(err) { list.innerHTML=""; const box=document.createElement("div");box.className="empty-state error";box.textContent=err.message;list.appendChild(box); }
        }

        async function loadSubmission(submissionId) {
            try {
                const s=await api(`/api/admin/submissions/${submissionId}`,{cache:"no-store"});
                selectedAdminSubmissionId=Number(s.submission_id);
                selectedAdminLanguageId=Number(s.language_id);
                $("admin-source-info").textContent=`Submission #${s.submission_id} · ${s.username} · ${s.problem_title} · ${s.language} · ${s.status} · ${s.submitted_at}`;
                $("admin-source-code").value=s.code || "";
                $("admin-similarity-btn").disabled=false;
                $("admin-ai-review-btn").disabled=false;
                $("admin-analysis-result").textContent="Choose Similarity or AI Review to analyze this submission.";
                $("code-review-panel").scrollIntoView({behavior:"smooth",block:"start"});
            } catch(err) { notify(err.message || "Could not load submission.","error"); }
        }

        $("admin-user-search-btn").addEventListener("click",loadAdminUsers);
        $("admin-user-refresh-btn").addEventListener("click",loadAdminUsers);
        $("admin-user-search").addEventListener("keydown",e=>{if(e.key==="Enter"){e.preventDefault();loadAdminUsers();}});
        // Render analysis results as readable UI instead of raw JSON.
        function resultContainer(title, subtitle = "") {
            const root = $("admin-analysis-result");
            root.replaceChildren();
            root.classList.add("analysis-result-ui");
            const header = document.createElement("div");
            header.className = "analysis-result-header";
            const h = document.createElement("h3"); h.textContent = title;
            header.appendChild(h);
            if (subtitle) { const p=document.createElement("p"); p.textContent=subtitle; header.appendChild(p); }
            root.appendChild(header);
            return root;
        }

        function addAnalysisNotice(root, text, kind = "info") {
            const notice=document.createElement("div"); notice.className=`analysis-notice ${kind}`; notice.textContent=text; root.appendChild(notice);
        }

        function addMetric(root, label, value, hint = "") {
            const card=document.createElement("article"); card.className="analysis-metric-card";
            const name=document.createElement("span"); name.className="analysis-metric-label"; name.textContent=label;
            const metric=document.createElement("strong"); metric.className="analysis-metric-value"; metric.textContent=value == null || value === "" ? "—" : String(value);
            card.append(name,metric);
            if(hint){const small=document.createElement("small");small.textContent=hint;card.appendChild(small);}
            root.appendChild(card);
        }

        function renderSimilarityResult(result) {
            const matches=Array.isArray(result?.matches)?result.matches:[];
            const root=resultContainer("Similarity analysis", `Submission #${result?.submission_id ?? selectedAdminSubmissionId}`);
            const metrics=document.createElement("div");metrics.className="analysis-metrics-grid";
            const maxSimilarity=matches.reduce((max,m)=>Math.max(max,Number(m.similarity_percent)||0),0);
            addMetric(metrics,"Potential matches",matches.length,"Similar submissions found");
            addMetric(metrics,"Highest similarity",`${maxSimilarity}%`,matches.length?"Highest reported match":"No matches found");
            root.appendChild(metrics);

            if(!matches.length){addAnalysisNotice(root,"No similar submissions were found.","success");}
            else {
                const section=document.createElement("section");section.className="analysis-section";
                const title=document.createElement("h4");title.textContent="Matching submissions";section.appendChild(title);
                const tableWrap=document.createElement("div");tableWrap.className="analysis-table-wrap";
                const table=document.createElement("table");table.className="analysis-table";
                const thead=document.createElement("thead"),headRow=document.createElement("tr");
                ["Similarity","Submission ID","User ID","Assessment","Compare lines"].forEach(label=>{const th=document.createElement("th");th.textContent=label;headRow.appendChild(th);});
                thead.appendChild(headRow);table.appendChild(thead);
                const tbody=document.createElement("tbody");
                matches.slice().sort((a,b)=>(Number(b.similarity_percent)||0)-(Number(a.similarity_percent)||0)).forEach(match=>{
                    const tr=document.createElement("tr");
                    const pct=Number(match.similarity_percent)||0;
                    const tdPct=document.createElement("td");
                    const pill=document.createElement("span");pill.className=`similarity-pill ${pct>=80?"high":pct>=50?"medium":"low"}`;pill.textContent=`${pct}%`;tdPct.appendChild(pill);tr.appendChild(tdPct);
                    [match.submission_id,match.user_id,pct>=80?"High similarity":pct>=50?"Moderate similarity":"Lower similarity"].forEach(value=>{const td=document.createElement("td");td.textContent=value==null?"—":String(value);tr.appendChild(td);});
                    const actionTd=document.createElement("td");
                    const compareBtn=document.createElement("button");compareBtn.type="button";compareBtn.className="btn btn-secondary similarity-compare-btn";compareBtn.textContent="Highlight lines";
                    compareBtn.addEventListener("click",()=>showSimilarLines(match));
                    actionTd.appendChild(compareBtn);tr.appendChild(actionTd);
                    tbody.appendChild(tr);
                });
                table.appendChild(tbody);tableWrap.appendChild(table);section.appendChild(tableWrap);root.appendChild(section);
            }
            addAnalysisNotice(root,result?.warning||"Similarity is a heuristic signal, not proof of plagiarism.","warning");
        }

        
        
        function normalizeMeaningfulLine(line) {
            let normalized = line.trim();

            if (!normalized) return null;

            // Remove preprocessor directives.
            if (/^\s*#/.test(normalized)) return null;

            // Remove namespace boilerplate.
            normalized = normalized.replace(
                /\busing\s+namespace\s+std\s*;/g,
                ""
            );

            // Remove main() declaration without discarding its body.
            normalized = normalized.replace(
                /\b(?:int|signed\s+int|signed)\s+main\s*\([^)]*\)\s*\{/g,
                ""
            );

            // Normalize std::cout and other standard namespace qualifiers.
            normalized = normalized.replace(/\bstd\s*::\s*/g, "");

            // Remove common return statements.
            normalized = normalized.replace(
                /\breturn\s+0\s*;/g,
                ""
            );

            // Remove standalone structural braces.
            normalized = normalized.replace(/[{}]/g, "");

            // Remove unnecessary whitespace.
            let result = "";
            let inString = false;
            let inChar = false;
            let escaped = false;

            for (const ch of normalized) {
                if (escaped) {
                    result += ch;
                    escaped = false;
                    continue;
                }

                if ((inString || inChar) && ch === "\\") {
                    result += ch;
                    escaped = true;
                    continue;
                }

                if (!inChar && ch === '"') {
                    inString = !inString;
                    result += ch;
                    continue;
                }

                if (!inString && ch === "'") {
                    inChar = !inChar;
                    result += ch;
                    continue;
                }

                if (!inString && !inChar && /\s/.test(ch)) {
                    continue;
                }

                result += ch;
            }

            return result || null;
        }



        async function showSimilarLines(match) {
            const currentCode = $("admin-source-code").value || "";
            const currentId = Number(selectedAdminSubmissionId);
            const matchedId = Number(match?.submission_id);

            if (!currentCode.trim() || !currentId || !matchedId) {
                notify("Select a submission and a valid match first.", "error");
                return;
            }

            const root = $("admin-analysis-result");
            const previous = root.querySelector(".similarity-compare-view");

            if (previous) previous.remove();

            const view = document.createElement("section");
            view.className = "similarity-compare-view analysis-section";

            const heading = document.createElement("h4");
            heading.textContent =
                `Matching lines: submission #${currentId} vs #${matchedId}`;

            view.appendChild(heading);

            const loading = document.createElement("p");
            loading.className = "muted";
            loading.textContent = "Loading matched submission source...";
            view.appendChild(loading);

            root.appendChild(view);

            try {
                const other = await api(
                    `/api/admin/submissions/${matchedId}`,
                    { cache: "no-store" }
                );

                const sourceCode = String(other.code || "");

                const sourceLines = currentCode.replace(/\r/g, "").split("\n");
                const otherLines = sourceCode.replace(/\r/g, "").split("\n");

                // Normalize only meaningful lines.
                const normalizedSource = sourceLines.map(normalizeMeaningfulLine);
                const normalizedOther = otherLines.map(normalizeMeaningfulLine);

                const otherLineSet = new Set(
                    normalizedOther.filter(Boolean)
                );

                const currentLineSet = new Set(
                    normalizedSource.filter(Boolean)
                );

                const sourceShared = new Set();
                const otherShared = new Set();

                // Find matching meaningful lines in selected submission.
                normalizedSource.forEach((line, i) => {
                    if (line && otherLineSet.has(line)) {
                        sourceShared.add(i);
                    }
                });

                // Find matching meaningful lines in matched submission.
                normalizedOther.forEach((line, i) => {
                    if (line && currentLineSet.has(line)) {
                        otherShared.add(i);
                    }
                });

                view.replaceChildren();

                const top = document.createElement("div");
                top.className = "similarity-compare-heading";

                const title = document.createElement("h4");
                title.textContent =
                    `Shared meaningful lines · submission #${currentId} and #${matchedId}`;

                const count = document.createElement("span");
                count.className = "similarity-shared-count";
                count.textContent =
                    `${sourceShared.size} meaningful matching line(s)`;

                top.append(title, count);
                view.appendChild(top);

                const legend = document.createElement("p");
                legend.className = "similarity-highlight-legend";
                legend.textContent =
                    "Yellow lines indicate matching meaningful source statements. " +
                    "Whitespace differences are ignored; common boilerplate is excluded.";

                view.appendChild(legend);

                const columns = document.createElement("div");
                columns.className = "similarity-code-columns";

                const makeCodePane = (paneTitle, lines, shared) => {
                    const pane = document.createElement("div");
                    pane.className = "similarity-code-pane";

                    const paneHead = document.createElement("div");
                    paneHead.className = "similarity-code-pane-title";
                    paneHead.textContent = paneTitle;

                    pane.appendChild(paneHead);

                    const pre = document.createElement("pre");
                    pre.className = "similarity-code-lines";

                    lines.forEach((line, i) => {
                        const row = document.createElement("div");

                        row.className =
                            "similarity-code-line" +
                            (shared.has(i) ? " is-similar" : "");

                        const number = document.createElement("span");
                        number.className = "similarity-line-number";
                        number.textContent = String(i + 1);

                        const content = document.createElement("span");
                        content.className = "similarity-line-content";
                        content.textContent = line || " ";

                        row.append(number, content);
                        pre.appendChild(row);
                    });

                    pane.appendChild(pre);
                    return pane;
                };

                columns.append(
                    makeCodePane(
                        `Selected submission #${currentId}`,
                        sourceLines,
                        sourceShared
                    ),
                    makeCodePane(
                        `Matched submission #${matchedId}`,
                        otherLines,
                        otherShared
                    )
                );

                view.appendChild(columns);

                if (!sourceShared.size) {
                    addAnalysisNotice(
                        view,
                        "No identical meaningful source lines were found. " +
                        "The similarity percentage may reflect token or structural similarity.",
                        "info"
                    );
                }

            } catch (err) {
                view.replaceChildren();

                const error = document.createElement("div");
                error.className = "analysis-notice error";
                error.textContent =
                    `Could not load comparison source: ${err.message || "Request failed"}`;

                view.appendChild(error);
            }
        }


        function renderAiReviewResult(result) {
            const root=resultContainer("AI code review", `Submission #${selectedAdminSubmissionId}`);
            const confidence=result?.confidence;
            const status=document.createElement("div");status.className="analysis-review-status";
            const label=document.createElement("span");label.textContent="Review confidence";
            const badge=document.createElement("strong");badge.className="analysis-confidence";badge.textContent=confidence?String(confidence):"Not specified";
            status.append(label,badge);root.appendChild(status);

            const metrics=document.createElement("div");metrics.className="analysis-metrics-grid";
            addMetric(metrics,"Time complexity",result?.time_complexity||result?.time_complexity_estimate||"Not provided");
            addMetric(metrics,"Space complexity",result?.space_complexity||result?.space_complexity_estimate||"Not provided");
            root.appendChild(metrics);

            const explanation=result?.explanation||result?.summary||result?.review||result?.analysis;
            if(explanation){const section=document.createElement("section");section.className="analysis-section";const h=document.createElement("h4");h.textContent="Review summary";const p=document.createElement("p");p.className="analysis-explanation";p.textContent=String(explanation);section.append(h,p);root.appendChild(section);}
            const extraFields=[
                ["Correctness",result?.correctness||result?.correctness_notes],
                ["Potential issues",result?.potential_issues||result?.issues||result?.bugs],
                ["Recommendations",result?.recommendations||result?.suggestions],
                ["Complexity explanation",result?.complexity_explanation]
            ];
            extraFields.forEach(([heading,value])=>{
                if(value==null||value===""||(Array.isArray(value)&&!value.length))return;
                const section=document.createElement("section");section.className="analysis-section";const h=document.createElement("h4");h.textContent=heading;section.appendChild(h);
                if(Array.isArray(value)){const ul=document.createElement("ul");value.forEach(item=>{const li=document.createElement("li");li.textContent=typeof item==="string"?item:JSON.stringify(item);ul.appendChild(li);});section.appendChild(ul);}
                else {const p=document.createElement("p");p.textContent=typeof value==="string"?value:JSON.stringify(value);section.appendChild(p);}
                root.appendChild(section);
            });
            if(!explanation&&!extraFields.some(([,v])=>v!=null&&v!=="")) addAnalysisNotice(root,"Review completed, but the response did not include a textual explanation.","info");
            addAnalysisNotice(root,"AI-generated feedback is advisory. Validate complexity and suggestions manually.","warning");
        }

        function renderAnalysisError(message) {
            const root=resultContainer("Analysis could not be completed");
            addAnalysisNotice(root,message,"error");
        }

        $("admin-similarity-btn").addEventListener("click",async()=>{
            if(!selectedAdminSubmissionId)return;
            const btn=$("admin-similarity-btn");btn.disabled=true;
            resultContainer("Similarity analysis","Comparing source code with recent submissions…");
            try {
                const result=await api("/api/admin/similarity",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({submission_id:selectedAdminSubmissionId})});
                renderSimilarityResult(result);
                notify("Similarity analysis completed.");
            } catch(err) {renderAnalysisError(err.message);notify(err.message,"error");}
            finally{btn.disabled=false;}
        });
        $("admin-ai-review-btn").addEventListener("click",async()=>{
            if(!selectedAdminSubmissionId)return;
            const btn=$("admin-ai-review-btn");btn.disabled=true;
            resultContainer("AI code review","Reviewing the selected source code…");
            try {
                const result=await api("/api/admin/ai-review",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({code:$("admin-source-code").value,language_id:selectedAdminLanguageId})});
                renderAiReviewResult(result);
                notify("AI code review completed.");
            } catch(err) {renderAnalysisError(err.message);notify(err.message,"error");}
            finally{btn.disabled=false;}
        });

        async function loadAuditLogs() {
            const list=$("admin-audit-list");
            list.innerHTML='<div class="empty-state">Loading audit logs…</div>';
            try {
                const logs=await api("/api/admin/audit",{cache:"no-store"});
                if(!Array.isArray(logs))throw new Error("Unexpected audit log response.");
                if(!logs.length){list.innerHTML='<div class="empty-state">No audit events found.</div>';return;}
                const rows=logs.map(log=>{
                    const tr=document.createElement("tr");
                    cell(tr,log.audit_id);cell(tr,log.action);cell(tr,log.admin_user_id);cell(tr,log.target_user_id);cell(tr,log.ip_address);cell(tr,log.created_at);
                    const td=document.createElement("td");const details=document.createElement("details");const summary=document.createElement("summary");summary.textContent="Details";const pre=document.createElement("pre");pre.textContent=typeof log.details==="string"?log.details:JSON.stringify(log.details??{},null,2);details.append(summary,pre);td.appendChild(details);tr.appendChild(td);
                    return tr;
                });
                list.replaceChildren(table(["ID","Action","Admin ID","Target user","IP","Date","Details"],rows));
            }catch(err){list.innerHTML="";const box=document.createElement("div");box.className="empty-state error";box.textContent=err.message;list.appendChild(box);}
        }
        $("admin-audit-refresh").addEventListener("click",loadAuditLogs);

        
        (async () => {
            try {
                await api("/api/admin/check", { cache: "no-store" });

                setAccess("ok", "Administrator access verified.");

                await loadProblems();
                await loadAuditLogs();

                // Automatically load the full user list on dashboard startup.
                await loadAdminUsers();

            } catch (error) {
                setAccess("error", error.message || "Admin access required.");

                document.querySelectorAll(".admin-panel, .admin-stats")
                    .forEach(el => el.hidden = true);

                notify(
                    "You need an active administrator session to access this page.",
                    "error"
                );
            }
        })();

    });
})();
