<div align="center">

<img src="https://img.shields.io/badge/HeavenDB-v6.0-0ea5e9?style=for-the-badge&labelColor=1e293b" alt="HeavenDB v6.0" />

# 🗄️ HeavenDB

### A SQL Database Engine Built From Scratch in Pure C

**Zero dependencies. Zero frameworks. Just raw C, B-Trees, WAL, and 5 days of obsession.**

[![Language](https://img.shields.io/badge/C-C99-22c55e?style=for-the-badge&logo=c&logoColor=white&labelColor=1e293b)](https://github.com/Aditya-cyber-hind/HeavenDB)
[![Platform](https://img.shields.io/badge/Windows-MinGW-64748b?style=for-the-badge&logo=windows&logoColor=white&labelColor=1e293b)](https://github.com/Aditya-cyber-hind/HeavenDB)
[![License](https://img.shields.io/badge/MIT-License-f59e0b?style=for-the-badge&labelColor=1e293b)](LICENSE)
[![Tests](https://img.shields.io/badge/272%20commands-passing-22c55e?style=for-the-badge&labelColor=1e293b)](https://github.com/Aditya-cyber-hind/HeavenDB)
[![Security](https://img.shields.io/badge/PBKDF2-100k%20iterations-ef4444?style=for-the-badge&labelColor=1e293b)](https://github.com/Aditya-cyber-hind/HeavenDB)

[**Features**](#-features) · [**Architecture**](#-architecture) · [**Build**](#-building) · [**Limitations**](#️-known-limitations) · [**SQL Reference**](#-sql-reference) · [**Roadmap**](#-roadmap)

</div>

---

## 📖 What Is HeavenDB?

HeavenDB is a **SQL database engine written from scratch in pure C** — no external libraries, no frameworks, no shortcuts. Every B-Tree rebalancing, every WAL entry, every SQL token parsed is code written from the ground up.

It is an **educational project** designed to demonstrate how real databases work — from raw byte storage to ACID transactions to SQL parsing to cryptographic password hashing.

> *"Anyone can `npm install sqlite3`. Very few can build the engine itself."*

**Built by a 13-year-old systems programmer in 5 days.**

---

## ✨ Features

<table>
<tr>
<td width="50%" valign="top">

### 🧠 SQL Engine

| Category | Support |
|----------|---------|
| **DML** | `SELECT`, `INSERT`, `UPDATE`, `DELETE` |
| **DDL** | `CREATE`, `DROP`, `ALTER TABLE` |
| **Views** | `CREATE VIEW`, `DROP VIEW` |
| **Indexes** | `CREATE INDEX` |
| **Joins** | `INNER`, `LEFT`, `RIGHT`, `FULL`, `CROSS` |
| **Subqueries** | `EXISTS`, `NOT EXISTS`, `ANY`, `ALL`, `SOME` |
| **Set ops** | `UNION`, `UNION ALL` |
| **CTEs** | `WITH ... AS (...)` |

</td>
<td width="50%" valign="top">

### 📊 Data Processing

| Category | Support |
|----------|---------|
| **Aggregates** | `COUNT`, `SUM`, `AVG`, `MIN`, `MAX`, `GROUP_CONCAT` |
| **Grouping** | `GROUP BY`, `HAVING` |
| **Sorting** | `ORDER BY`, `LIMIT`, `OFFSET`, `DISTINCT` |
| **Filtering** | `WHERE`, `AND`, `OR`, all comparison ops |
| **Patterns** | `LIKE`, `BETWEEN`, `IN`, `IS NULL` |
| **Conditional** | `CASE WHEN ... THEN ... ELSE ... END` |

</td>
</tr>
</table>

### 🎯 Data Types

<table>
<tr>
<td align="center"><code>INTEGER</code></td>
<td align="center"><code>FLOAT</code></td>
<td align="center"><code>TEXT</code></td>
<td align="center"><code>BOOLEAN</code></td>
</tr>
<tr>
<td align="center"><code>UUID</code></td>
<td align="center"><code>JSON</code></td>
<td align="center"><code>DATE</code></td>
<td align="center"><code>TIMESTAMP</code></td>
</tr>
</table>

### 🔧 Built-in Functions

<table>
<tr>
<td width="33%" valign="top">

**String**
```sql
UPPER(str)
LOWER(str)
LENGTH(str)
TRIM(str)
SUBSTR(str, start, len)
CONCAT(a, b, c, ...)
```

</td>
<td width="33%" valign="top">

**Math**
```sql
ABS(n)
ROUND(n, decimals)
FLOOR(n)
CEIL(n)
MOD(a, b)
```

</td>
<td width="33%" valign="top">

**JSON / Date**
```sql
json_extract(col, 'key')
json_set(col, 'key', val)
NOW()
CURRENT_DATE()
YEAR(date)
MONTH(date)
DAY(date)
```

</td>
</tr>
</table>

### 🛡️ Security & Administration

- 🔐 **PBKDF2-HMAC-SHA256** password hashing with **100,000 iterations** and per-user salts
- 🔒 Account lockout after 5 failed login attempts
- ✅ Password validation (min 8 chars, uppercase, lowercase, digit)
- 👥 User management: `CREATE USER`, `LOGIN`, `LOGOUT`, `CHANGE PASSWORD`
- 🎫 Permissions: `GRANT`, `REVOKE` per user / per table
- 💾 Backup: `BACKUP TO 'file.hdb'`
- 🔍 Query analysis: `EXPLAIN`
- 📋 Table inspection: `SHOW TABLES`, `DESCRIBE`, `TRUNCATE`

### ⚡ Storage Engine

- 🚀 **In-memory hash map** — O(1) key-value lookups
- 🌳 **B-Tree indexes** — O(log n) range queries on `INTEGER` columns
- 📝 **Write-Ahead Log** — Full ACID transaction support
- 📦 **Group Commit buffer** — batched disk writes
- 🗄️ **Append-only `.hdb` format** — crash-safe persistence
- 🔒 **Global write lock** — single-process safety
- ⛓️ **FK CASCADE** — `ON DELETE CASCADE` referential integrity

### 🌐 Networking

- 🔌 **TCP server** — multi-threaded client-server mode
- 🌍 **HTTP server** — built-in web dashboard
- 💬 **WebSocket** — real-time browser connections
- 🐍 **Cross-language** — works with Python, Node.js, Go, telnet

---

## ⚠️ Known Limitations

HeavenDB is an **educational project**, not a production database. Here's what it can't do:

### 🔒 Concurrency
- **Writers serialize.** A global write lock means **only one writer can execute at a time.** Concurrent `INSERT`/`UPDATE`/`DELETE` operations will block. There is no MVCC (Multi-Version Concurrency Control).
- **No transaction isolation levels.** Nested transactions are not supported beyond `SAVEPOINT`.
- **Reads can be stale.** A reader thread can see partial state during a concurrent write.

### 📊 Performance Reality
- The **"10M ops/sec"** figure is measured on a **1,000-key working set** — small enough to fit entirely in **L1 CPU cache**. Real workloads with millions of keys will be **orders of magnitude slower**.
- **SQL queries do linear scans** unless they use an indexed `INTEGER` column.
- **`GROUP BY`, `ORDER BY`, and `JOIN` are O(n²)** naive algorithms. Fine for thousands of rows; not for millions.
- **The database file is rewritten on every write** via `sql_save()`. Real databases use incremental WAL replay.

### 🌳 Indexing
- B-Tree indexes support **only `INTEGER` columns**.
- `TEXT`, `UUID`, `DATE`, `TIMESTAMP`, `JSON`, and `BOOLEAN` columns do full table scans.

### ⛓️ FK CASCADE
- `ON DELETE CASCADE` **rewrites the entire database file** on each cascade delete — O(n) cost per cascade.
- Only `INTEGER` foreign keys are supported.
- No `ON UPDATE CASCADE`.

### 🧪 Testing & CI
- Only **one integration test** file (`test3.sql`, 272 commands).
- **No unit tests** for `hashmap.c`, `btree.c`, `wal.c`, or `buffer.c`.
- **No GitHub Actions CI** running on every commit.
- **No fuzz testing** on the SQL parser.

### 🌍 Portability
- The networking layer uses **Winsock** (Windows-only). Linux/macOS require a POSIX port (on the roadmap).
- Compile flags (`-lws2_32`, backslash paths) are Windows-specific.

### 🔐 Security Caveats
- Passwords use **PBKDF2-HMAC-SHA256** — this part is solid.
- **No timing-attack protection** on password comparison.
- **No SQL injection protection** — the parser is homegrown.
- **No TLS/SSL** on the network layer. Traffic is plaintext.
- The default `admin` user password is documented below. **Change it immediately.**

### 🚫 Not Supported
- Window functions (`ROW_NUMBER`, `RANK`)
- Triggers
- Stored procedures
- Full-text search
- User-defined functions
- Subqueries in `FROM` clauses
- `RIGHT JOIN` / `FULL JOIN` with complex conditions
- Recursive CTEs
- Streaming results

---

## 📊 Performance

**Honest benchmarks** on a standard development laptop (Windows, 4-core CPU, 8GB RAM):

| Operation | Throughput | What It Actually Measures |
|-----------|:----------:|---------------------------|
| In-memory GET (1k keys) | ~10,000,000 ops/sec | **L1 cache throughput** — not realistic for large datasets |
| In-memory GET (1M keys) | *(not yet benchmarked)* | Would be significantly slower — depends on cache misses |
| Disk SET (Group Commit) | ~7,800 ops/sec | Batched writes, 100 per flush |
| PBKDF2 password hash | ~50 hashes/sec | Intentional — 100k SHA-256 iterations |
| SQL SELECT | Linear scan | ~1M rows/sec on integer comparisons |

> ⚠️ **These numbers reflect local hardware under ideal conditions.** They are **not** indicative of production throughput. Real performance depends on working set size, disk speed, and concurrency.
>
> Run `heavendb benchmark 100000` on your own machine to verify. **Do not cite these numbers in production planning.**

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        HEAVENDB                             │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│    ┌──────────┐      ┌──────────┐      ┌──────────┐        │
│    │   CLI    │      │   TCP    │      │   HTTP   │        │
│    │  Client  │      │  Server  │      │  Server  │        │
│    └────┬─────┘      └────┬─────┘      └────┬─────┘        │
│         │                 │                  │              │
│         └─────────────────┼──────────────────┘              │
│                           ▼                                 │
│         ┌─────────────────────────────────────┐             │
│         │  SQL Tokenizer → Parser → Executor  │             │
│         └────────────────┬────────────────────┘             │
│                          ▼                                  │
│   ┌──────────────────────────────────────────────────┐      │
│   │       In-Memory Hash Map  (O(1) lookups)         │      │
│   └──────────────────────────────────────────────────┘      │
│   ┌──────────────────────────────────────────────────┐      │
│   │       B-Tree Indexes  (O(log n) ranges)          │      │
│   └──────────────────────────────────────────────────┘      │
│   ┌──────────────────────────────────────────────────┐      │
│   │       Append-Only Storage  (.hdb files)          │      │
│   └──────────────────────────────────────────────────┘      │
│   ┌──────────────────────────────────────────────────┐      │
│   │       Group Commit Buffer  (batched writes)      │      │
│   └──────────────────────────────────────────────────┘      │
│   ┌──────────────────────────────────────────────────┐      │
│   │       Write-Ahead Log  (ACID transactions)       │      │
│   └──────────────────────────────────────────────────┘      │
│   ┌──────────────────────────────────────────────────┐      │
│   │       Global Write Lock  (writers serialize)     │      │
│   └──────────────────────────────────────────────────┘      │
│                                                             │
│   ┌───────────┐  ┌─────────────┐  ┌──────────────┐         │
│   │ PBKDF2    │  │ Permissions │  │ Replication  │         │
│   │  Auth     │  │  GRANT/REV  │  │  (basic)     │         │
│   └───────────┘  └─────────────┘  └──────────────┘         │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## 🔨 Building

### Windows (MinGW / MSYS2)

```bash
gcc -Wall -Wextra -Wno-switch -O2 -std=c99 -o heavendb.exe \
    src\main.c src\database.c src\hashmap.c src\buffer.c \
    src\table.c src\sql.c src\btree.c src\wal.c \
    src\tcp_server.c src\auth.c src\auth_storage.c \
    src\permissions.c src\replication.c src\websocket.c \
    src\http_server.c -lws2_32
```

### Linux / macOS (Core Engine Only)

The networking layer uses Winsock. For Linux/macOS, build the core engine without networking:

```bash
gcc -Wall -Wextra -Wno-switch -O2 -std=c99 -o heavendb \
    src/main.c src/database.c src/hashmap.c src/buffer.c \
    src/table.c src/sql.c src/btree.c src/wal.c \
    src/auth.c src/auth_storage.c src/permissions.c src/replication.c
```

> 🚧 A full POSIX socket port is on the roadmap. Until then, use [WSL](https://learn.microsoft.com/en-us/windows/wsl/) for full cross-platform support.

---

## 🚀 Quick Start

```bash
# Interactive shell
heavendb shell

# Start TCP + HTTP server (Windows only)
heavendb serve
# → Open http://localhost:8080 for the web dashboard

# Run a SQL script
heavendb run script.sql

# Run the stress test (272 commands)
heavendb run test3.sql

# Benchmark
heavendb benchmark 100000
```

---

## 📝 SQL Reference

### Creating Tables

```sql
CREATE TABLE users (
    id INTEGER PRIMARY KEY AUTO_INCREMENT,
    name TEXT NOT NULL,
    email TEXT UNIQUE,
    age INTEGER,
    salary FLOAT,
    active BOOLEAN,
    metadata JSON,
    created_at TIMESTAMP
);

CREATE TABLE orders (
    id INTEGER PRIMARY KEY AUTO_INCREMENT,
    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    total FLOAT,
    status TEXT
);
```

### Inserting Data

```sql
INSERT INTO users VALUES (NULL, 'Aditya', 'a@x.com', 25, 85000.50, TRUE, '{"role":"engineer"}', NOW());
INSERT INTO users VALUES (NULL, 'Rahul',  'r@x.com', 19, 65000.75, FALSE, '{"role":"intern"}',   NOW());
```

### Querying

```sql
-- Basic
SELECT * FROM users;
SELECT name, age FROM users;

-- Filtering
SELECT * FROM users WHERE age > 25;
SELECT * FROM users WHERE age BETWEEN 20 AND 30;
SELECT * FROM users WHERE name LIKE 'A%';
SELECT * FROM users WHERE id IN (1, 2, 3);

-- Sorting
SELECT * FROM users ORDER BY salary DESC;
SELECT * FROM users LIMIT 10 OFFSET 5;

-- Aggregation
SELECT COUNT(*) FROM users;
SELECT AVG(salary) FROM users;
SELECT age, COUNT(*) FROM users GROUP BY age HAVING COUNT(*) > 1;
SELECT GROUP_CONCAT(name) FROM users;

-- Joins
SELECT * FROM users INNER JOIN orders ON users.id = orders.user_id;
SELECT * FROM users LEFT  JOIN orders ON users.id = orders.user_id;

-- Subqueries
SELECT * FROM users WHERE age > (SELECT AVG(age) FROM users);
SELECT * FROM users WHERE EXISTS (SELECT 1 FROM orders WHERE orders.user_id = users.id);

-- CASE WHEN
SELECT name, CASE WHEN salary > 80000 THEN 'High' ELSE 'Normal' END FROM users;

-- JSON
SELECT json_extract(metadata, 'role') FROM users;
SELECT json_set(metadata, 'role', 'manager') FROM users;

-- Date / Time
SELECT NOW();
SELECT CURRENT_DATE();
SELECT YEAR(created_at), MONTH(created_at), DAY(created_at) FROM users;
```

### Transactions

```sql
BEGIN;
INSERT INTO users VALUES (NULL, 'Test', 't@x.com', 30, 50000.00, TRUE, '{}', NOW());
SAVEPOINT checkpoint;
INSERT INTO users VALUES (NULL, 'Test2', 't2@x.com', 32, 55000.00, TRUE, '{}', NOW());
ROLLBACK TO SAVEPOINT checkpoint;
COMMIT;
```

### Security

```sql
-- Default admin password: Admin1234 (CHANGE THIS IMMEDIATELY AFTER FIRST LOGIN)
LOGIN admin WITH PASSWORD 'Admin1234';
CHANGE PASSWORD 'YourNewSecure456';

CREATE USER alice WITH PASSWORD 'AlicePass789';
GRANT  SELECT ON users TO alice;
REVOKE DELETE ON users FROM alice;
LOGOUT;
```

> ⚠️ **The default password `Admin1234` is publicly known.** Change it in your first session. HeavenDB enforces min 8 chars, uppercase, lowercase, digit for any new password.

### Administration

```sql
SHOW TABLES;
DESCRIBE users;
EXPLAIN SELECT * FROM users WHERE salary > 70000;
BACKUP TO 'backup.hdb';
TRUNCATE TABLE users;
```

---

## 🌍 Cross-Language Clients

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

client.on('data', (data) => console.log(data.toString()));
```

### telnet

```bash
telnet localhost 6379
```

---

## 🧪 Testing

### Integration Test

HeavenDB ships with **one integration test** covering 272 SQL commands across 9 tables:

```bash
heavendb run test3.sql
```

Result:

```
========================================
Script complete!
  Commands executed: 272
  Errors: 10  (all expected DROP TABLE on first run)
========================================
```

### Unit Tests

> 🚧 **Not yet implemented.** Unit tests for `hashmap.c`, `btree.c`, `wal.c`, and `buffer.c` are on the roadmap.

### CI / CD

> 🚧 **Not yet implemented.** GitHub Actions CI is on the roadmap.

---

## 📁 Project Structure

```
HeavenDB/
├── src/
│   ├── main.c              # CLI entry point
│   ├── database.c/h        # Key-value storage engine
│   ├── hashmap.c/h         # Hash map implementation
│   ├── btree.c/h           # B-Tree implementation
│   ├── buffer.c/h          # Group commit buffer
│   ├── wal.c/h             # Write-Ahead Log
│   ├── table.c/h           # Table structure & constraints
│   ├── sql.c/h             # SQL parser & executor
│   ├── auth.c/h            # PBKDF2 authentication
│   ├── auth_storage.c/h    # Persistent user storage
│   ├── permissions.c/h     # GRANT / REVOKE system
│   ├── replication.c/h     # Replication
│   ├── tcp_server.c/h      # TCP server
│   ├── http_server.c/h     # HTTP server
│   └── websocket.c/h       # WebSocket support
│
├── dashboard/
│   ├── index.html          # Web dashboard UI
│   ├── style.css           # Dashboard styles
│   └── app.js              # Dashboard logic
│
├── test3.sql               # 272-command integration test
├── README.md
└── Makefile
```

---

## 🗺️ Roadmap

<details open>
<summary><b>✅ Completed</b></summary>

- [x] Hash Map storage engine
- [x] Group Commit buffering
- [x] SQL parser & tokenizer
- [x] B-Tree indexes with proper rebalancing
- [x] Persistent storage
- [x] ACID transactions with WAL
- [x] TCP + HTTP servers with web dashboard
- [x] **PBKDF2-HMAC-SHA256** password hashing
- [x] **Global write lock** for single-process safety
- [x] User authentication with account lockout
- [x] `GRANT` / `REVOKE` permissions
- [x] All `JOIN` types
- [x] Views
- [x] Aggregates, `GROUP BY`, `HAVING`
- [x] Subqueries, `EXISTS`, `ANY` / `ALL`
- [x] `CASE WHEN`
- [x] All 8 data types
- [x] String, Math, JSON, and Date functions
- [x] Primary key, unique, not null, auto-increment, FK CASCADE
- [x] `SAVEPOINT` / `RELEASE` / `ROLLBACK TO`
- [x] `BACKUP` and `EXPLAIN`

</details>

<details>
<summary><b>🚧 In Progress</b></summary>

- [ ] Unit tests for hashmap, btree, wal, buffer
- [ ] GitHub Actions CI
- [ ] POSIX socket port (Linux/macOS networking)
- [ ] Multi-process MVCC concurrency
- [ ] Window functions (`ROW_NUMBER`, `RANK`)
- [ ] Triggers
- [ ] Stored procedures

</details>

<details>
<summary><b>🔮 Planned</b></summary>

- [ ] Full-text search
- [ ] B-Tree support for TEXT / UUID / DATE columns
- [ ] Incremental FK CASCADE (no full-file rewrite)
- [ ] Query optimizer
- [ ] Cost-based planner
- [ ] Real replication
- [ ] Columnar storage engine

</details>

---

## 🧑‍💻 Author

**Aditya** — 13-year-old systems programmer from India.

[![GitHub](https://img.shields.io/badge/GitHub-Aditya--cyber--hind-181717?style=for-the-badge&logo=github)](https://github.com/Aditya-cyber-hind)

---

## 📄 License

MIT License — free to use, modify, and distribute.

See [LICENSE](LICENSE) for full text.

---

<div align="center">

### ⭐ If HeavenDB impressed you, star the repository

**Built from scratch. Built with obsession. Built in pure C.**

[⬆ Back to top](#️-heavendb)

</div>
