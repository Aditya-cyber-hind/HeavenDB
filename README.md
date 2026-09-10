# HeavenDB

A high-performance SQL database engine written in pure C. Built from scratch with zero external dependencies.

![Version](https://img.shields.io/badge/version-4.0-blue)
![Language](https://img.shields.io/badge/language-C99-green)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)
![License](https://img.shields.io/badge/license-MIT-yellow)
![Lines of Code](https://img.shields.io/badge/lines%20of%20code-6000%2B-orange)

## What is HeavenDB?

HeavenDB is a complete SQL database engine written from scratch in pure C. No external libraries, no dependencies, no frameworks. Just raw C code compiled with GCC.

Built by a 13-year-old developer to understand how databases work under the hood.

## Features

### SQL Engine
- **Full CRUD** — CREATE, INSERT, SELECT, UPDATE, DELETE
- **Joins** — INNER JOIN, LEFT JOIN with ON clause
- **Views** — Virtual tables with saved queries
- **Constraints** — PRIMARY KEY, UNIQUE, NOT NULL, AUTO_INCREMENT, FOREIGN KEY
- **Aggregate Functions** — COUNT, SUM, AVG, MIN, MAX
- **Grouping** — GROUP BY with counts
- **Sorting** — ORDER BY ASC/DESC
- **Pagination** — LIMIT and OFFSET
- **Filtering** — WHERE with AND/OR, =, !=, <>, <, >, <=, >=
- **Pattern Matching** — LIKE with % wildcards
- **Range Queries** — BETWEEN
- **List Matching** — IN operator
- **NULL Checks** — IS NULL, IS NOT NULL
- **Distinct Values** — DISTINCT
- **Transactions** — BEGIN, COMMIT, ROLLBACK with Write-Ahead Log
- **Query Analysis** — EXPLAIN to see query plans

### Data Types
- **INTEGER** — 32-bit signed integers
- **FLOAT** — 64-bit floating point
- **TEXT** — Variable-length strings
- **UUID** — 128-bit unique identifiers (auto-generated)
- **JSON** — Structured JSON documents

### Storage Engine
- **In-Memory Hash Map** — 10M+ operations per second
- **B-Tree Indexes** — O(log n) lookups on INTEGER columns
- **Group Commit Buffer** — 100x faster disk writes
- **Append-Only File Storage** — Crash-safe persistence
- **Write-Ahead Log** — ACID transaction support
- **Custom Binary Format** — `.hdb` files

### Networking
- **TCP Server** — Multi-threaded client-server mode
- **HTTP Server** — Built-in web dashboard
- **WebSocket Support** — Real-time browser connections
- **Cross-Language** — Works with Python, Node.js, telnet, or any TCP client

### Security
- **User Authentication** — CREATE USER, LOGIN, LOGOUT
- **Password Hashing** — djb2 hashing algorithm
- **Password Validation** — Minimum 8 chars, upper, lower, digit required
- **Account Lockout** — Auto-lock after 5 failed attempts
- **Password Change** — CHANGE PASSWORD command
- **Persistent Auth** — Users saved to disk
- **Permissions** — GRANT and REVOKE per user/table

### Administration
- **BACKUP** — Save database to file
- **REPLICATE TO** — Configure replica servers
- **SYNC** — Push data to replicas
- **EXPLAIN** — Analyze query execution plans

## Performance

| Operation | Throughput |
|-----------|------------|
| In-Memory GET | 10,000,000 ops/sec |
| Disk SET (Group Commit) | 7,800 ops/sec |
| SQL SELECT | Instant |
| B-Tree Lookup | O(log n) |
| Hash Map Lookup | O(1) |

## Architecture

```
┌─────────────────────────────────────────────────┐
│                    HeavenDB                      │
├─────────────────────────────────────────────────┤
│  CLI Client │ TCP Server │ HTTP Server │ WebSocket │
├──────────────┴────────────┴─────────────┴───────┤
│              Command Parser / SQL Tokenizer     │
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
│      User Auth │ Permissions │ Replication      │
└─────────────────────────────────────────────────┘
```

## Quick Start

### Compile

```bash
gcc -Wall -Wextra -O2 -std=c99 -o heavendb.exe src\main.c src\database.c src\hashmap.c src\buffer.c src\table.c src\sql.c src\btree.c src\wal.c src\tcp_server.c src\auth.c src\auth_storage.c src\permissions.c src\replication.c src\websocket.c src\http_server.c -lws2_32
```

### Interactive Shell

```bash
heavendb shell
```

### Start Server (TCP + HTTP + Dashboard)

```bash
heavendb serve
```

Then open **http://localhost:8080** for the web dashboard.

### Key-Value Store

```bash
heavendb set name "Aditya"
heavendb get name
heavendb delete name
heavendb size
```

### Benchmark

```bash
heavendb benchmark 100000
```

## SQL Examples

### Authentication

```sql
LOGIN admin WITH PASSWORD 'admin123';
CREATE USER alice WITH PASSWORD 'Alice123';
CHANGE PASSWORD 'NewSecure123';
LOGOUT;
```

### Permissions

```sql
GRANT SELECT ON users TO alice;
GRANT ALL ON users TO admin;
REVOKE DELETE ON users FROM alice;
```

### Table Management

```sql
CREATE TABLE users (
    id UUID,
    name TEXT NOT NULL,
    age INTEGER,
    data JSON
);

CREATE TABLE orders (
    id INTEGER PRIMARY KEY AUTO_INCREMENT,
    user_id INTEGER REFERENCES users(id),
    total INTEGER
);

ALTER TABLE users ADD COLUMN email TEXT;
DROP TABLE orders;
```

### Data Operations

```sql
INSERT INTO users VALUES (UUID(), 'Aditya', 25, '{"city":"Mumbai","hobbies":["coding","gaming"]}');
INSERT INTO users VALUES (UUID(), 'Rahul', 19, '{"city":"Delhi"}');
INSERT INTO users VALUES (UUID(), 'Priya', 30, '{"city":"Bangalore"}');

SELECT * FROM users;
SELECT name, age FROM users;
SELECT * FROM users WHERE age > 20;
SELECT * FROM users WHERE age > 20 AND age < 30;
SELECT * FROM users WHERE name = 'Aditya' OR name = 'Priya';
SELECT * FROM users WHERE age IN (25, 30);
SELECT * FROM users WHERE name LIKE 'Adi%';
SELECT * FROM users WHERE age BETWEEN 20 AND 30;
SELECT * FROM users WHERE email IS NULL;
```

### Aggregations

```sql
SELECT COUNT(*) FROM users;
SELECT SUM(age) FROM users;
SELECT AVG(age) FROM users;
SELECT MIN(age) FROM users;
SELECT MAX(age) FROM users;
SELECT age, COUNT(*) FROM users GROUP BY age;
SELECT DISTINCT age FROM users;
```

### Sorting and Pagination

```sql
SELECT * FROM users ORDER BY age ASC;
SELECT * FROM users ORDER BY age DESC;
SELECT * FROM users LIMIT 2;
SELECT * FROM users LIMIT 2 OFFSET 1;
```

### Joins

```sql
SELECT * FROM users INNER JOIN orders ON users.id = orders.user_id;
SELECT * FROM users LEFT JOIN orders ON users.id = orders.user_id;
```

### Views

```sql
CREATE VIEW adult_users AS SELECT * FROM users WHERE age >= 18;
SELECT * FROM adult_users;
```

### Updates and Deletes

```sql
UPDATE users SET age = 26 WHERE id = 1;
DELETE FROM users WHERE id = 2;
```

### Transactions

```sql
BEGIN;
INSERT INTO users VALUES (UUID(), 'TestUser', 50, '{}');
COMMIT;
-- or ROLLBACK;
```

### Administration

```sql
BACKUP TO 'mybackup.hdb';
REPLICATE TO 'localhost:6379';
SYNC;
EXPLAIN SELECT * FROM users WHERE age > 18;
```

## Cross-Language Clients

### Python

```python
import socket

s = socket.socket()
s.connect(('localhost', 6379))
s.send(b'SELECT * FROM users\n')
print(s.recv(4096).decode())
s.close()
```

### Node.js

```javascript
const net = require('net');
const client = new net.Socket();

client.connect(6379, 'localhost', () => {
    client.write('SELECT * FROM users\n');
});

client.on('data', (data) => {
    console.log(data.toString());
    client.destroy();
});
```

## Web Dashboard

Open **http://localhost:8080** after running `heavendb serve`.

The dashboard connects to HeavenDB via HTTP and lets you run SQL queries directly in the browser.

## Tech Stack

- **Language:** C (C99 standard)
- **Compiler:** GCC
- **Platform:** Windows, Linux, macOS
- **Dependencies:** Zero
- **Build System:** Manual GCC commands

## Roadmap

- [x] Hash Map storage engine
- [x] Group Commit buffering
- [x] SQL parser (CREATE, INSERT, SELECT, UPDATE, DELETE)
- [x] B-Tree indexes
- [x] Persistent storage
- [x] ACID transactions with WAL
- [x] TCP server
- [x] User authentication
- [x] Password security with lockout
- [x] INNER JOIN
- [x] LEFT JOIN
- [x] Views
- [x] GRANT / REVOKE permissions
- [x] Replication
- [x] Web Dashboard
- [x] HTTP Server
- [x] COUNT, SUM, AVG, MIN, MAX
- [x] ORDER BY, GROUP BY
- [x] LIMIT, OFFSET, DISTINCT
- [x] LIKE, BETWEEN, IN, IS NULL
- [x] AND / OR conditions
- [x] <=, >=, !=, <> operators
- [x] PRIMARY KEY, UNIQUE, NOT NULL, AUTO_INCREMENT
- [x] FOREIGN KEY (parsed)
- [x] BACKUP command
- [x] EXPLAIN command
- [x] UUID data type with auto-generation
- [x] JSON data type
- [ ] CTEs (Common Table Expressions)
- [ ] Subqueries
- [ ] UNION
- [ ] HAVING clause
- [ ] CREATE INDEX (manual)
- [ ] TRUNCATE TABLE
- [ ] SHOW TABLES / DESCRIBE
- [ ] Full-text search

## File Structure

```
HeavenDB/
├── src/
│   ├── main.c              # CLI entry point
│   ├── database.c/h        # Key-value storage engine
│   ├── hashmap.c/h         # Hash map implementation
│   ├── btree.c/h           # B-Tree implementation
│   ├── buffer.c/h          # Group commit buffer
│   ├── wal.c/h             # Write-Ahead Log
│   ├── table.c/h           # Table structure
│   ├── sql.c/h             # SQL parser and executor
│   ├── auth.c/h            # Authentication
│   ├── auth_storage.c/h    # Persistent user storage
│   ├── permissions.c/h     # GRANT/REVOKE system
│   ├── replication.c/h     # Replication system
│   ├── tcp_server.c/h      # TCP server
│   ├── http_server.c/h     # HTTP server
│   └── websocket.c/h       # WebSocket support
├── dashboard/
│   ├── index.html          # Web dashboard UI
│   ├── style.css           # Dashboard styles
│   └── app.js              # Dashboard logic
├── README.md
└── Makefile
```

## Author

Built by **Aditya** — a 13-year-old systems programming enthusiast.

**GitHub:** [Aditya-cyber-hind](https://github.com/Aditya-cyber-hind)

## License

MIT License — free to use, modify, and distribute.
