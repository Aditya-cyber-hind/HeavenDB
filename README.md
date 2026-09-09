# HeavenDB

A high-performance SQL database engine written in pure C. Built from scratch with zero external dependencies.

![Version](https://img.shields.io/badge/version-2.0-blue)
![Language](https://img.shields.io/badge/language-C99-green)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)
![License](https://img.shields.io/badge/license-MIT-yellow)

## 🚀 Features

- ⚡ **In-Memory Hash Map** — 10M+ operations per second
- 🌳 **B-Tree Indexes** — O(log n) lookups on INTEGER columns
- 💾 **Group Commit Buffer** — 100x faster disk writes
- 📝 **SQL Parser** — CREATE, INSERT, SELECT, UPDATE, DELETE with WHERE
- 🔗 **INNER JOIN** — Cross-table queries with ON clause
- 🔗 **LEFT JOIN** — Returns all rows from left table with NULLs
- 👁️ **Views** — Virtual tables with saved queries
- 🔒 **Persistent Storage** — Data survives restarts
- 🔄 **ACID Transactions** — BEGIN, COMMIT, ROLLBACK
- 🌐 **TCP Server** — Multi-threaded client-server mode
- 🌐 **HTTP Server** — Built-in web dashboard
- 🖥️ **Web Dashboard** — Browser-based UI for SQL queries
- 👤 **User Authentication** — CREATE USER, LOGIN, LOGOUT
- 🔑 **Password Security** — Change password, account lockout, password validation
- 🛡️ **GRANT/REVOKE** — Permission management per user/table
- 📡 **Replication** — REPLICATE TO, SYNC
- 💻 **Interactive Shell** — Real-time SQL command execution
- 🐍 **Cross-Language** — Works with Python, Node.js, telnet, or any TCP client

## 📊 Performance

| Operation | Throughput |
|-----------|------------|
| In-Memory GET | 10,000,000 ops/sec |
| Disk SET (Group Commit) | 7,800 ops/sec |
| SQL SELECT | Instant |

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────┐
│                    HeavenDB                      │
├─────────────────────────────────────────────────┤
│  CLI Client │ TCP Server │ HTTP Server │ SQL    │
├──────────────┴────────────┴─────────────┴───────┤
│              Command Parser                     │
├─────────────────────────────────────────────────┤
│           In-Memory Hash Map (O(1))             │
├─────────────────────────────────────────────────┤
│           B-Tree Indexes (O(log n))             │
├─────────────────────────────────────────────────┤
│         Append-Only File Storage (.hdb)         │
├─────────────────────────────────────────────────┤
│           Group Commit Buffer                   │
├─────────────────────────────────────────────────┤
│           Write-Ahead Log (WAL)                 │
├─────────────────────────────────────────────────┤
│           User Authentication                   │
├─────────────────────────────────────────────────┤
│           Permission System                     │
├─────────────────────────────────────────────────┤
│           Replication                           │
└─────────────────────────────────────────────────┘
```

## 🔧 Quick Start

### Compile

```bash
gcc -Wall -Wextra -O2 -std=c99 -o heavendb.exe src\main.c src\database.c src\hashmap.c src\buffer.c src\table.c src\sql.c src\btree.c src\wal.c src\tcp_server.c src\auth.c src\auth_storage.c src\permissions.c src\replication.c src\websocket.c src\http_server.c -lws2_32
```

### Key-Value Store

```bash
heavendb set name "Aditya"
heavendb get name
heavendb delete name
heavendb size
heavendb benchmark 10000
```

### SQL Mode

```bash
heavendb sql "CREATE TABLE users (id INTEGER, name TEXT, age INTEGER)"
heavendb sql "INSERT INTO users VALUES (1, 'Aditya', 25)"
heavendb sql "SELECT * FROM users WHERE age > 18"
```

### Interactive Shell

```bash
heavendb shell
```

### Start Server (TCP + HTTP + Dashboard)

```bash
heavendb serve
```

Then open:
- **Dashboard:** http://localhost:8080
- **TCP clients:** telnet localhost 6379

### Web Dashboard

Open `http://localhost:8080` in your browser. Type SQL queries and click Run.

## 📝 SQL Commands

```sql
-- Authentication (default: admin / admin123)
LOGIN admin WITH PASSWORD 'admin123';
CHANGE PASSWORD 'NewSecure123';
CREATE USER alice WITH PASSWORD 'Alice123';
LOGOUT;

-- Permissions
GRANT SELECT ON users TO alice;
GRANT ALL ON users TO admin;
REVOKE DELETE ON users FROM alice;

-- Table Operations
CREATE TABLE users (id INTEGER, name TEXT, age INTEGER);
INSERT INTO users VALUES (1, 'Aditya', 25);
SELECT * FROM users;
SELECT * FROM users WHERE age > 18;
UPDATE users SET age = 26 WHERE id = 1;
DELETE FROM users WHERE id = 2;

-- Joins
SELECT * FROM users INNER JOIN orders ON users.id = orders.user_id;
SELECT * FROM users LEFT JOIN orders ON users.id = orders.user_id;

-- Views
CREATE VIEW adult_users AS SELECT * FROM users WHERE age >= 18;
SELECT * FROM adult_users;

-- Transactions
BEGIN;
INSERT INTO users VALUES (3, 'NewUser', 30);
COMMIT;
-- or ROLLBACK;

-- Replication
REPLICATE TO 'localhost:6379';
SYNC;
```

## 🔐 Security Features

- Password hashing (djb2)
- Account lockout after 5 failed attempts
- Password validation (min 8 chars, uppercase, lowercase, digit)
- Persistent auth storage
- Change password command
- Permission management (GRANT/REVOKE)

## 🧪 Testing

### Python Client

```python
import socket
s = socket.socket()
s.connect(('localhost', 6379))
s.send(b'SELECT * FROM users\n')
print(s.recv(4096).decode())
```

### Node.js Client

```javascript
const net = require('net');
const client = new net.Socket();
client.connect(6379, 'localhost', () => {
    client.write('SELECT * FROM users\n');
});
client.on('data', (data) => {
    console.log(data.toString());
});
```

### Benchmark

```bash
heavendb benchmark 100000
```

## 🗺️ Roadmap

- [x] Hash Map storage engine
- [x] Group Commit buffering
- [x] SQL parser (CREATE, INSERT, SELECT)
- [x] UPDATE and DELETE commands
- [x] B-Tree indexes
- [x] Persistent storage
- [x] ACID transactions
- [x] TCP server
- [x] User authentication
- [x] Password security
- [x] INNER JOIN
- [x] LEFT JOIN
- [x] Views
- [x] GRANT / REVOKE permissions
- [x] Replication
- [x] Web Dashboard
- [x] HTTP Server
- [ ] Query Optimizer
- [ ] Stored Procedures
- [ ] Full-text Search

## 👨‍💻 Author

Built by **Aditya** — a 13-year-old systems programming enthusiast.

**GitHub:** [Aditya-cyber-hind](https://github.com/Aditya-cyber-hind)

## 📄 License

MIT License — free to use, modify, and distribute.
