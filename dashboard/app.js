let currentToken = null;
let currentUser = null;

function showMain() {
    document.getElementById('loginBox').style.display = 'none';
    document.getElementById('mainBox').style.display = 'block';
    document.getElementById('whoami').textContent = 'Logged in as ' + currentUser;
}

function showLogin() {
    document.getElementById('loginBox').style.display = 'flex';
    document.getElementById('mainBox').style.display = 'none';
}

async function doLogin() {
    const user = document.getElementById('loginUser').value;
    const pass = document.getElementById('loginPass').value;
    const status = document.getElementById('loginStatus');
    
    status.textContent = 'Logging in...';
    
    try {
        const res = await fetch('http://localhost:8080/login', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: `username=${encodeURIComponent(user)}&password=${encodeURIComponent(pass)}`
        });
        
        if (!res.ok) {
            status.textContent = 'Login failed';
            return;
        }
        
        const data = await res.json();
        currentToken = data.token;
        currentUser = user;
        sessionStorage.setItem('token', currentToken);
        sessionStorage.setItem('user', currentUser);
        status.textContent = '';
        showMain();
    } catch (e) {
        status.textContent = 'Error: ' + e.message;
    }
}

function doLogout() {
    currentToken = null;
    currentUser = null;
    sessionStorage.removeItem('token');
    sessionStorage.removeItem('user');
    showLogin();
}

async function executeQuery() {
    const query = document.getElementById('query').value;
    const output = document.getElementById('output');
    
    if (!currentToken) {
        output.textContent = 'Not logged in';
        return;
    }
    
    output.textContent = 'Running...';
    
    try {
        const res = await fetch(`http://localhost:8080/query?sql=${encodeURIComponent(query)}`, {
            headers: { 'Authorization': 'Bearer ' + currentToken }
        });
        
        if (res.status === 401) {
            output.textContent = 'Session expired. Please log in again.';
            doLogout();
            return;
        }
        
        const data = await res.text();
        output.textContent = data;
    } catch (e) {
        output.textContent = 'Error: ' + e.message;
    }
}

function handleEnter(event) {
    if (event.key === 'Enter') {
        executeQuery();
    }
}

// Restore session on page load
window.addEventListener('DOMContentLoaded', () => {
    const saved = sessionStorage.getItem('token');
    const user = sessionStorage.getItem('user');
    if (saved && user) {
        currentToken = saved;
        currentUser = user;
        showMain();
    } else {
        showLogin();
    }
});