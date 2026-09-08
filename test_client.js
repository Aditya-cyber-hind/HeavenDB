const net = require('net');

const client = new net.Socket();
const PORT = 6379;
const HOST = 'localhost';

client.connect(PORT, HOST, () => {
    console.log('Connected to HeavenDB server!\n');
});

let buffer = '';

client.on('data', (data) => {
    buffer += data.toString();
    
    // Check if we have a complete response (ends with newline)
    if (buffer.includes('\n')) {
        process.stdout.write(buffer);
        buffer = '';
    }
});

client.on('close', () => {
    console.log('\nConnection closed.');
    process.exit(0);
});

client.on('error', (err) => {
    console.error('Error:', err.message);
    process.exit(1);
});

// Send a series of commands with delays
setTimeout(() => {
    console.log('Sending: SELECT * FROM users');
    client.write('SELECT * FROM users\n');
}, 1000);

setTimeout(() => {
    console.log('\nSending: BEGIN');
    client.write('BEGIN\n');
}, 3000);

setTimeout(() => {
    console.log('\nSending: INSERT INTO users VALUES (10, "NodeUser", 25)');
    client.write("INSERT INTO users VALUES (10, 'NodeUser', 25)\n");
}, 5000);

setTimeout(() => {
    console.log('\nSending: COMMIT');
    client.write('COMMIT\n');
}, 7000);

setTimeout(() => {
    console.log('\nSending: SELECT * FROM users WHERE age > 20');
    client.write('SELECT * FROM users WHERE age > 20\n');
}, 9000);

setTimeout(() => {
    console.log('\nClosing connection...');
    client.write('exit\n');
}, 12000);