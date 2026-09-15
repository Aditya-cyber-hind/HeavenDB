const API_BASE = 'http://localhost:8080';
const TOKEN_KEY = 'heavendb_token';
const USER_KEY  = 'heavendb_user';

let token = null;
let username = null;

// ==================== LOGIN / LOGOUT ====================

async function doLogin() {
    const user = document.getElementById('loginUser').value.trim();
    const pass = document.getElementById('loginPass').value;
    const errEl = document.getElementById('loginError');
    const btn = document.getElementById('loginBtn');
    
    errEl.textContent = '';
    btn.disabled = true;
    btn.textContent = 'Logging in…';
    
    try {
        const body = 'username=' + encodeURIComponent(user) +
                     '&password=' + encodeURIComponent(pass);
        
        const res = await fetch(API_BASE + '/login', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: body
        });
        
        if (!res.ok) {
            errEl.textContent = 'Invalid username or password';
            return;
        }
        
        const data = await res.json();
        token = data.token;
        username = user;
        sessionStorage.setItem(TOKEN_KEY, token);
        sessionStorage.setItem(USER_KEY, username);
        
        showApp();
        loadTables();
    } catch (e) {
        errEl.textContent = 'Connection error: ' + e.message;
    } finally {
        btn.disabled = false;
        btn.textContent = 'Log In';
    }
}

function doLogout() {
    token = null;
    username = null;
    sessionStorage.removeItem(TOKEN_KEY);
    sessionStorage.removeItem(USER_KEY);
    showLogin();
}

function showLogin() {
    document.getElementById('loginScreen').style.display = 'flex';
    document.getElementById('appScreen').style.display = 'none';
    document.getElementById('loginPass').value = '';
    document.getElementById('loginError').textContent = '';
}

function showApp() {
    document.getElementById('loginScreen').style.display = 'none';
    document.getElementById('appScreen').style.display = 'flex';
    document.getElementById('whoami').textContent = 'Logged in as ' + username;
}

// ==================== API ====================

async function apiQuery(sql) {
    const res = await fetch(API_BASE + '/query?sql=' + encodeURIComponent(sql), {
        headers: { 'Authorization': 'Bearer ' + token }
    });
    
    if (res.status === 401) {
        doLogout();
        throw new Error('Session expired');
    }
    
    return await res.text();
}

// ==================== TABLE LIST ====================

async function loadTables() {
    const list = document.getElementById('tableList');
    list.innerHTML = '<li class="placeholder">Loading…</li>';
    
    try {
        const text = await apiQuery('SHOW TABLES');
        const parsed = parseResult(text);
        
        if (parsed.kind !== 'table' || parsed.rows.length === 0) {
            list.innerHTML = '<li class="placeholder">No tables</li>';
            return;
        }
        
        list.innerHTML = '';
        for (const row of parsed.rows) {
            const name = row[0];
            const cols = row[1] || '';
            
            const li = document.createElement('li');
            li.innerHTML = '<span class="table-name"></span>' +
                           '<span class="col-count">' + cols + '</span>';
            li.querySelector('.table-name').textContent = name;
            li.onclick = () => {
                document.getElementById('queryInput').value = 'SELECT * FROM ' + name;
                runQuery();
            };
            list.appendChild(li);
        }
    } catch (e) {
        list.innerHTML = '<li class="placeholder">Error: ' + e.message + '</li>';
    }
}

// ==================== QUERY ====================

async function runQuery() {
    const input = document.getElementById('queryInput');
    const sql = input.value.trim();
    if (!sql) return;
    
    const statusBar = document.getElementById('statusBar');
    const resultArea = document.getElementById('resultArea');
    const runBtn = document.getElementById('runBtn');
    
    runBtn.disabled = true;
    statusBar.className = 'status-bar';
    statusBar.innerHTML = '<span class="spinner"></span>Running…';
    
    const startTime = performance.now();
    
    try {
        const text = await apiQuery(sql);
        const elapsed = (performance.now() - startTime).toFixed(0);
        
        const parsed = parseResult(text);
        renderResult(parsed);
        
        // Detect if this was a DDL/state-change statement
        if (/\b(CREATE|DROP|ALTER)\s+TABLE\b/i.test(sql)) {
            loadTables();
        }
        
        statusBar.className = 'status-bar ok';
        if (parsed.kind === 'table') {
            statusBar.textContent = parsed.rows.length + ' row' +
                                    (parsed.rows.length === 1 ? '' : 's') +
                                    ' · ' + elapsed + 'ms';
        } else {
            statusBar.textContent = 'OK · ' + elapsed + 'ms';
        }
    } catch (e) {
        statusBar.className = 'status-bar error';
        statusBar.textContent = e.message;
        resultArea.innerHTML = '<div class="result-message error">' +
                               escapeHtml(e.message) + '</div>';
    } finally {
        runBtn.disabled = false;
    }
}

// ==================== PARSE TEXT RESULT ====================
//
// The server prints results like:
//
//   id              name            age
//   --------------- --------------- ---------------
//   1               Aditya          25
//   2               Rahul           19
//   (2 rows)
//
// Or, for non-tabular output:
//
//   OK. Inserted 1 row
//   ERROR: Table 'users' not found
//
// parseResult() turns this into { kind: 'table', columns, rows } or
// { kind: 'message', text }.

function parseResult(text) {
    // Normalize line endings
    text = text.replace(/\r\n/g, '\n');
    
    // Split and drop trailing empty lines
    let lines = text.split('\n');
    while (lines.length > 0 && lines[lines.length - 1].trim() === '') {
        lines.pop();
    }
    while (lines.length > 0 && lines[0].trim() === '') {
        lines.shift();
    }
    
    if (lines.length === 0) {
        return { kind: 'message', text: '' };
    }
    
    // Check for tabular shape: line 0 = headers, line 1 = dashes
    if (lines.length >= 2 && isSeparatorLine(lines[1])) {
        const columns = splitColumns(lines[0]);
        const rows = [];
        
        for (let i = 2; i < lines.length; i++) {
            const trimmed = lines[i].trim();
            // Terminator: (N rows) or (N row) or (nil)
            if (/^\(\d+\s+rows?\)$/.test(trimmed) || trimmed === '(nil)') {
                break;
            }
            if (trimmed === '') continue;
            rows.push(splitCells(lines[i], columns.length));
        }
        
        return { kind: 'table', columns, rows };
    }
    
    // Otherwise it's a message
    return { kind: 'message', text: text.trim() };
}

function isSeparatorLine(line) {
    const t = line.trim();
    if (t.length === 0) return false;
    // Must contain only dashes and whitespace
    return /^-+(\s+-+)*$/.test(t);
}

function splitColumns(line) {
    // Column headers are separated by 2+ spaces
    return line.trim().split(/\s{2,}/);
}

function splitCells(line, expectedCount) {
    // Data cells are separated by 2+ spaces
    const trimmed = line.replace(/\s+$/, '');
    const cells = trimmed.split(/\s{2,}/);
    
    // Pad with empty strings if fewer cells than expected
    while (cells.length < expectedCount) {
        cells.push('');
    }
    // Truncate extras (shouldn't happen with well-formed output)
    if (cells.length > expectedCount) {
        cells.length = expectedCount;
    }
    return cells;
}

// ==================== RENDER ====================

function renderResult(parsed) {
    const area = document.getElementById('resultArea');
    area.innerHTML = '';
    
    if (parsed.kind === 'message') {
        const div = document.createElement('div');
        const isError = /^ERROR/i.test(parsed.text);
        div.className = 'result-message ' + (isError ? 'error' : 'ok');
        div.textContent = parsed.text || 'OK';
        area.appendChild(div);
        return;
    }
    
    // Table
    const table = document.createElement('table');
    table.className = 'result-table';
    
    const thead = document.createElement('thead');
    const headRow = document.createElement('tr');
    for (const col of parsed.columns) {
        const th = document.createElement('th');
        th.textContent = col;
        headRow.appendChild(th);
    }
    thead.appendChild(headRow);
    table.appendChild(thead);
    
    const tbody = document.createElement('tbody');
    for (const row of parsed.rows) {
        const tr = document.createElement('tr');
        for (let i = 0; i < parsed.columns.length; i++) {
            const td = document.createElement('td');
            const val = row[i] !== undefined ? row[i] : '';
            td.textContent = val;
            
            if (val === '') {
                td.classList.add('null');
            } else if (/^-?\d+(\.\d+)?$/.test(val)) {
                td.classList.add('number');
            } else if (val === 'TRUE' || val === 'FALSE') {
                td.classList.add('boolean');
            }
            
            tr.appendChild(td);
        }
        tbody.appendChild(tr);
    }
    table.appendChild(tbody);
    area.appendChild(table);
    
    // Row count footer
    const count = document.createElement('div');
    count.className = 'result-count';
    count.textContent = parsed.rows.length + ' row' +
                        (parsed.rows.length === 1 ? '' : 's');
    area.appendChild(count);
}

// ==================== UTIL ====================

function escapeHtml(s) {
    return String(s)
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;')
        .replace(/'/g, '&#39;');
}

// ==================== RESTORE SESSION ====================

window.addEventListener('DOMContentLoaded', () => {
    const savedToken = sessionStorage.getItem(TOKEN_KEY);
    const savedUser = sessionStorage.getItem(USER_KEY);
    
    if (savedToken && savedUser) {
        token = savedToken;
        username = savedUser;
        showApp();
        loadTables();
    } else {
        showLogin();
        // Focus the password field for quick login
        setTimeout(() => document.getElementById('loginPass').focus(), 100);
    }
});