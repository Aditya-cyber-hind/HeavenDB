<div align="center">

<img src="https://img.shields.io/badge/HeavenDB-v0.1.0--preview-0ea5e9?style=for-the-badge&labelColor=1e293b" alt="HeavenDB v0.1.0 Preview" />

# 🗄️ HeavenDB

### A SQL Database Engine Built From Scratch in Pure C

**No third-party libraries. Just the C standard library, Winsock, and 5 days of obsession.**

[![Language](https://img.shields.io/badge/C-C99-22c55e?style=for-the-badge&logo=c&logoColor=white&labelColor=1e293b)](https://github.com/Aditya-cyber-hind/HeavenDB)
[![Platform](https://img.shields.io/badge/Windows-MinGW-64748b?style=for-the-badge&logo=windows&logoColor=white&labelColor=1e293b)](https://github.com/Aditya-cyber-hind/HeavenDB)
[![License](https://img.shields.io/badge/MIT-License-f59e0b?style=for-the-badge&labelColor=1e293b)](LICENSE)
[![Tests](https://img.shields.io/badge/tests-11%20unit%20%2B%20282%20integration-22c55e?style=for-the-badge&labelColor=1e293b)](https://github.com/Aditya-cyber-hind/HeavenDB)
[![Security](https://img.shields.io/badge/PBKDF2-100k%20iterations-ef4444?style=for-the-badge&labelColor=1e293b)](https://github.com/Aditya-cyber-hind/HeavenDB)

**~65,000 ops/sec on a 1M-key working set** (main memory, single-threaded, hashmap layer).

[Features](#-features) · [Architecture](#-architecture) · [Build](#-building) · [Limitations](#️-known-limitations) · [SQL Reference](#-sql-reference) · [Roadmap](#-roadmap)

</div>

---

## 📖 What Is HeavenDB?

HeavenDB is a **SQL database engine written from scratch in pure C** — no external libraries, no frameworks, no shortcuts. Every B-Tree rebalancing, every WAL entry, every SQL token parsed is code written from the ground up.

It is an **educational project** designed to demonstrate how real databases work — from raw byte storage to ACID transactions to SQL parsing to cryptographic password hashing.

> *"Anyone can `npm install sqlite3`. Very few can build the engine itself."*

### Why Build This?

Because databases are the black box of modern software. Every app uses one; few developers know how they actually work.

I wanted to understand:
- How does a `SELECT` query know where to find a row without scanning the whole file?
- What actually happens when you `COMMIT` a transaction — and why does a crash not corrupt your data?
- How does an index make lookups 1000x faster?
- What does a SQL parser actually do?

So I read the SQLite internals, studied PostgreSQL's WAL design, and built HeavenDB to answer those questions by writing the code myself.

This isn't a competitor to SQLite. It's what I built to learn how SQLite works.

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
| **Joins** | `INNER`, `LEFT`, `RIGHT`, `FULL`, `CROSS` (equality only) |
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
- 🎲 **Random password on first run** — no default credentials to leak
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
- 📝 **Write-Ahead Log** — real-time disk writes for crash recovery
- 💾 **Atomic saves** — temp file + rename prevents corruption on crash
- 📦 **Group Commit buffer** — batched disk writes
- 🗄️ **Append-only `.hdb` format** — persisted state
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
- **Writers serialize.** A global write lock means **only one writer can execute at a time.** Concurrent `INSERT`/`UPDATE`/`DELETE` operations will block. There is no MVCC.
- **No transaction isolation levels.** Nested transactions are not supported beyond `SAVEPOINT`.
- **Reads can be stale.** A reader thread can see partial state during a concurrent write.

### 📊 Performance Reality
- **`GROUP BY`, `ORDER BY`, and `JOIN` are O(n²)** naive algorithms. Fine for thousands of rows; not for millions.
- **B-Tree indexes support only `INTEGER` columns.** `TEXT`, `UUID`, `DATE` columns do full table scans.

### 📝 Write-Ahead Log (WAL)
- The WAL records every `INSERT` to disk in **real-time** during a transaction.
- On startup, `wal_recover()` checks for an interrupted transaction and discards it.
- **Uncommitted changes are rolled back.** Committed changes are persisted via the atomic save.
- WAL does **not** yet support full replay of committed operations — the main `.hdb` file is authoritative.

### 🔗 Joins
- `INNER`, `LEFT`, `RIGHT`, `FULL`, `CROSS` joins work with **simple `ON table1.col = table2.col` equality conditions only.**
- **Not supported:** Multi-column `ON` conditions, non-equality operators (`>`, `<`), joins on `TEXT` / `UUID` / `DATE` columns.

### ⛓️ FK CASCADE
- `ON DELETE CASCADE` rewrites the entire database file on each cascade delete — O(n) cost per cascade.
- Only `INTEGER` foreign keys are supported.

### 📡 Replication
- "Replication" in HeavenDB means **manual file copy**. The `REPLICATE TO` and `SYNC` commands are **stubs** that return success but do not actually sync data.
- **No streaming replication. No master-slave. No conflict resolution.**

### 🧪 Testing & CI
- **11 unit tests** for `hashmap.c` (all passing).
- **282-command integration test** with 0 errors.
- **No unit tests** yet for `btree.c`, `wal.c`, or `buffer.c`.
- **No GitHub Actions CI** running on every commit.

### 🌍 Portability
- The networking layer uses **Winsock** (Windows-only). Linux/macOS require a POSIX port.

### 🔐 Security Caveats
- Passwords use **PBKDF2-HMAC-SHA256** — this part is solid.
- **No timing-attack protection** on password comparison.
- **No prepared statements or parameter binding.** Clients must escape input themselves. This is standard for raw SQL protocols (MySQL, PostgreSQL, SQLite all behave the same way), but it means HeavenDB cannot protect you from injection the way a parameterized driver would.
- **No TLS/SSL** on the network layer. Traffic is plaintext.

### 🚫 Not Supported
- Window functions (`ROW_NUMBER`, `RANK`)
- Triggers
- Stored procedures
- Full-text search
- User-defined functions
- Subqueries in `FROM` clauses
- Recursive CTEs
- Streaming results

---

## 📊 Performance

**Honest benchmarks** on a standard development laptop (Windows, 4-core CPU, 8GB RAM). All numbers are reproducible with `heavendb benchmark 100000`.

> ⚠️ **Measurement note:** These numbers are measured at the **hash map layer** (`hashmap_get()` directly). SQL `SELECT` adds parsing, planning, and projection overhead — expect significantly lower throughput through the SQL layer.
>
> **The only number that reflects real workload performance is the 1M-key row: ~65,000 ops/sec.**

### In-Memory GET (varying working set sizes)

| Working Set | Throughput | CPU Cache Level |
|:-----------:|:----------:|-----------------|
| 100 keys | **~8,300,000 ops/sec** | L1 cache |
| 1,000 keys | **~7,100,000 ops/sec** | L1/L2 cache |
| 10,000 keys | **~5,900,000 ops/sec** | L2 cache |
| 100,000 keys | **~2,200,000 ops/sec** | L3 cache |
| 1,000,000 keys | **~65,000 ops/sec** | Main memory |

Performance drops **~128x** as the working set grows from 100 keys to 1M keys. The "10M ops/sec" figure only applies when the entire dataset fits in L1 cache.

### Disk Operations

| Operation | Throughput | Notes |
|-----------|:----------:|-------|
| Disk SET (Group Commit) | **~7,800 ops/sec** | Batched writes, 100 per flush |
| PBKDF2 password hash | **~50 hashes/sec** | Intentional — 100k SHA-256 iterations |

> ⚠️ **These are local machine numbers under ideal conditions.** They are **not** production benchmarks. Real throughput depends on working set size, disk speed, and concurrency.

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
│   │       Atomic Saves  (temp file + rename)         │      │
│   └──────────────────────────────────────────────────┘      │
│   ┌──────────────────────────────────────────────────┐      │
│   │       Group Commit Buffer  (batched writes)      │      │
│   └──────────────────────────────────────────────────┘      │
│   ┌──────────────────────────────────────────────────┐      │
│   │       Write-Ahead Log  (real-time disk writes)   │      │
│   └──────────────────────────────────────────────────┘      │
│   ┌──────────────────────────────────────────────────┐      │
│   │       Global Write Lock  (writers serialize)     │      │
│   └──────────────────────────────────────────────────┘      │
│                                                             │
│   ┌───────────┐  ┌─────────────┐  ┌──────────────┐         │
│   │ PBKDF2    │  │ Permissions │  │ Replication  │         │
│   │  Auth     │  │  GRANT/REV  │  │  (stubbed)   │         │
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
heavendb shell             # Interactive shell
heavendb serve             # TCP + HTTP server (dashboard at :8080)
heavendb run script.sql    # Execute SQL script
heavendb run test3.sql     # 282-command integration test
heavendb benchmark 100000  # Benchmark at multiple working-set sizes
```

### First-Run Setup

On the very first run, HeavenDB generates a **random admin password** and prints it once to **stderr** (so pipes don't swallow it). It's also saved to `~/.heavendb_initial_password` as a backup.

```
+==========================================================+
|           HeavenDB -- FIRST RUN SETUP                    |
+==========================================================+
|  Username: admin                                         |
|  Password: Tj45bZi128yB%6A1R9BrnWm                       |
|                                                          |
|  !!! COPY THIS PASSWORD NOW -- it will NOT be shown !!!  |
+==========================================================+
```

Copy it, then change it in your first session:

```sql
LOGIN admin WITH PASSWORD 'Tj45bZi128yB%6A1R9BrnWm';
CHANGE PASSWORD 'YourNewSecure456';
```

You can also set `HEAVENDB_INITIAL_PASSWORD` before first run to use your own password:

```bash
set HEAVENDB_INITIAL_PASSWORD=MyPassword123
heavendb shell
```

### Crash Recovery

If the process is killed mid-transaction, HeavenDB recovers cleanly on next start:

```
$ heavendb shell
WAL Recovery: Found interrupted transaction (1 inserts) — DISCARDED
Auth system loaded. 1 user(s) registered.
```

Uncommitted changes are rolled back. Committed data survives. Atomic saves prevent file corruption.

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
-- First run generates a random admin password (see console output).
LOGIN admin WITH PASSWORD '<the-random-password>';
CHANGE PASSWORD 'YourNewSecure456';

CREATE USER alice WITH PASSWORD 'AlicePass789';
GRANT  SELECT ON users TO alice;
REVOKE DELETE ON users FROM alice;
LOGOUT;
```

HeavenDB enforces **min 8 chars, uppercase, lowercase, digit** for any new password. Passwords are hashed with **PBKDF2-HMAC-SHA256 (100,000 iterations)** and a per-user random salt.

### Administration

```sql
SHOW TABLES;
DESCRIBE users;
EXPLAIN SELECT * FROM users WHERE salary > 70000;
BACKUP TO 'backup.hdb';
TRUNCATE TABLE users;
DROP TABLE IF EXISTS users;
```

---

## 🌍 Cross-Language Clients

> **Protocol:** Send `SELECT ...\n`. Server responds with:
> 1. Welcome banner (first connection only)
> 2. `heavendb> <your command>` (echo)
> 3. Result table
> 4. `(N rows)` or `(nil)` terminator
>
> Read until you see the terminator line. These examples are minimal — add reconnection, timeouts, and proper framing for production.

### Python

```python
import socket

s = socket.socket()
s.connect(('localhost', 6379))

print(s.recv(4096).decode())           # welcome banner
s.send(b'SELECT * FROM users\n')

response = b''
while True:
    chunk = s.recv(4096)
    if not chunk: break
    response += chunk
    # Response ends with "(N rows)" or "(nil)"
    if b'(nil)' in chunk or b'rows)' in chunk:
        break

print(response.decode())
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

### Unit Tests — `hashmap.c`

**11 unit tests** for the hash map:

```bash
gcc -Wall -Wextra -O2 -std=c99 -o tests\test_hashmap.exe tests\test_hashmap.c src\hashmap.c
tests\test_hashmap.exe
```

Result:

```
========================================
  Passed: 11
  Failed: 0
========================================
```

Covers: creation, set/get, updates, deletion, missing keys, 10,000-key inserts, hash collisions, empty values, 1000-char values, and edge cases.

### Integration Test

**282 SQL commands** across 9 tables, using `DROP TABLE IF EXISTS` for clean runs:

```bash
heavendb run test3.sql
```

Result:

```
========================================
Script complete!
  Commands executed: 282
  Errors: 0
========================================
```

### Crash Recovery Test

Kill the process mid-transaction and restart — the database recovers cleanly:

```bash
heavendb shell
BEGIN
INSERT INTO users VALUES (NULL, 'TempUser')
# Press Ctrl+C — process killed mid-transaction
```

Restart and check:
```bash
heavendb shell
# Output: WAL Recovery: Found interrupted transaction (1 inserts) — DISCARDED
SELECT * FROM users
# TempUser is NOT present — transaction was discarded
```

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
│   ├── wal.c/h             # Write-Ahead Log + crash recovery
│   ├── table.c/h           # Table structure & constraints
│   ├── sql.c/h             # SQL parser & executor
│   ├── auth.c/h            # PBKDF2 authentication
│   ├── auth_storage.c/h    # Persistent user storage
│   ├── permissions.c/h     # GRANT / REVOKE system
│   ├── replication.c/h     # Replication (stubbed)
│   ├── tcp_server.c/h      # TCP server (Windows-only)
│   ├── http_server.c/h     # HTTP server (Windows-only)
│   └── websocket.c/h       # WebSocket support (Windows-only)
│
├── dashboard/
│   ├── index.html          # Web dashboard UI
│   ├── style.css           # Dashboard styles
│   └── app.js              # Dashboard logic
│
├── tests/
│   └── test_hashmap.c      # 11 unit tests for the hash map
│
├── test3.sql               # 282-command integration test
├── README.md
└── Makefile
```

> 📝 **Note:** Files marked `(Windows-only)` are excluded from the Linux/macOS build. See the [Building](#-building) section for platform-specific compile commands.

---

## 🗺️ Roadmap

<details open>
<summary><b>✅ Completed</b></summary>

- [x] Hash Map storage engine
- [x] Group Commit buffering
- [x] SQL parser & tokenizer
- [x] B-Tree indexes with proper rebalancing
- [x] Persistent storage with **atomic saves** (temp file + rename)
- [x] **WAL real-time writes + crash recovery**
- [x] In-session transactions with SAVEPOINT
- [x] TCP + HTTP servers with web dashboard
- [x] **PBKDF2-HMAC-SHA256** password hashing
- [x] **Random password on first run** (no default credentials)
- [x] **Global write lock** for single-process safety
- [x] User authentication with account lockout
- [x] `GRANT` / `REVOKE` permissions
- [x] All `JOIN` types (simple equality)
- [x] Views
- [x] Aggregates, `GROUP BY`, `HAVING`
- [x] Subqueries, `EXISTS`, `ANY` / `ALL`
- [x] `CASE WHEN`
- [x] All 8 data types
- [x] String, Math, JSON, and Date functions
- [x] Primary key, unique, not null, auto-increment, FK CASCADE
- [x] `SAVEPOINT` / `RELEASE` / `ROLLBACK TO`
- [x] `BACKUP`, `EXPLAIN`, `DROP TABLE IF EXISTS`
- [x] **Unit tests for hashmap.c** (11 passing)
- [x] **Honest multi-scale benchmark** (100 to 1M keys)
- [x] **Clean integration test run** (282 commands, 0 errors)

</details>

<details>
<summary><b>🚧 In Progress (next up)</b></summary>

- [ ] **GitHub Actions CI**
- [ ] Unit tests for btree.c, wal.c, buffer.c
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
- [ ] Real replication (streaming master-slave)
- [ ] Query optimizer
- [ ] Cost-based planner
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
