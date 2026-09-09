function executeQuery() {
    const query = document.getElementById('query').value;
    const host = document.getElementById('host').value;
    const port = parseInt(document.getElementById('port').value);
    
    fetch(`http://${host}:8080/query?sql=${encodeURIComponent(query)}`)
        .then(response => response.text())
        .then(data => {
            document.getElementById('output').textContent = data;
        })
        .catch(error => {
            document.getElementById('output').textContent = 'Error: ' + error.message;
        });
}

function connect() {
    document.getElementById('status').textContent = 'Connected (HTTP)';
    document.getElementById('status').className = 'status-online';
}

function handleEnter(event) {
    if (event.key === 'Enter') {
        executeQuery();
    }
}