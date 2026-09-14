<div align="center">

<img src="https://img.shields.io/badge/HeavenDB-v0.1.0--preview-0ea5e9?style=for-the-badge&labelColor=1e293b" alt="HeavenDB v0.1.0 Preview" />

# 🗄️ HeavenDB

### A SQL Database Engine Built From Scratch in Pure C

**No third-party libraries for the engine. Just the C standard library, Winsock, and BCrypt.**

[![Language](https://img.shields.io/badge/C-C99-22c55e?style=for-the-badge&logo=c&logoColor=white&labelColor=1e293b)](https://github.com/Aditya-cyber-hind/HeavenDB)
[![Platform](https://img.shields.io/badge/Windows-MinGW-64748b?style=for-the-badge&logo=windows&logoColor=white&labelColor=1e293b)](https://github.com/Aditya-cyber-hind/HeavenDB)
[![License](https://img.shields.io/badge/MIT-License-f59e0b?style=for-the-badge&labelColor=1e293b)](LICENSE)
[![Tests](https://img.shields.io/badge/tests-11%20unit%20%2B%20regression-22c55e?style=for-the-badge&labelColor=1e293b)](https://github.com/Aditya-cyber-hind/HeavenDB)
[![Security](https://img.shields.io/badge/PBKDF2-100k%20iterations-ef4444?style=for-the-badge&labelColor=1e293b)](https://github.com/Aditya-cyber-hind/HeavenDB)

**~65,000 ops/sec on a 1M-key working set** (main memory, single-threaded, hashmap layer, on a 4-core Windows laptop — your numbers will differ).

[Features](#-features) · [Architecture](#-architecture) · [Build](#-building) · [Limitations](#️-known-limitations) · [SQL Reference](#-sql-reference) · [Roadmap](#-roadmap)

</div>

---

## 📖 What Is HeavenDB?

HeavenDB is a **SQL database engine written from scratch in pure C** — no frameworks, no shortcuts. Every B-Tree rebalancing, every WAL entry, every SQL token parsed is code written by hand.

It is an **educational project** designed to demonstrate how real databases work — from raw byte storage to ACID transactions to SQL parsing to cryptographic password hashing.

> *"Anyone can `npm install sqlite3`. Very few can build the engine itself."*

### Why Build This?

Because databases are the black box of modern software. Every app uses one; few developers know how they actually work.

I wanted to understand:
- How does a `SELECT` query know where to find a row without scanning the whole file?
- What actually happens when you `COMMIT` a transaction — and why does a crash not corrupt your data?
- How does an index make lookups faster?
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
| **DDL** | `CREATE TABLE`, `DROP TABLE`, `ALTER TABLE` |
| **Views** | `CREATE VIEW`, `DROP VIEW` |
| **Indexes** | `CREATE INDEX` (INTEGER / BOOLEAN columns) |
| **Joins** | `INNER`, `LEFT`, `RIGHT`, `FULL`, `CROSS` (equality only, INTEGER columns) |
| **Subqueries** | `EXISTS`, `NOT EXISTS`, `ANY`, `ALL`, `SOME`, scalar |
| **Set ops** | `UNION`, `UNION ALL` |
| **CTEs** | `WITH ... AS (...)` (single CTE, executes inner query) |

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
- 🎲 **Random password on first run** — generated with `BCryptGenRandom` (Windows CSPRNG)
- 🔒 Account lockout after 5 failed login attempts
- ✅ Password validation (min 8 chars, uppercase, lowercase, digit)
- 👥 User management: `CREATE USER`, `LOGIN`, `LOGOUT`, `CHANGE PASSWORD`
- 🎫 Permissions: `GRANT`, `REVOKE` per user / per table — **enforced** on `SELECT`, `INSERT`, `UPDATE`, `DELETE`, `ALTER`, `TRUNCATE`, `DROP`
- 💾 Backup: `BACKUP TO 'file.hdb'`
- 🔍 Query analysis: `EXPLAIN`
- 📋 Table inspection: `SHOW TABLES`, `DESCRIBE`, `TRUNCATE`

### ⚡ Storage Engine

- 🚀 **In-memory hash map** — O(1) key-value lookups
- 🌳 **B-Tree indexes** — O(log n) lookups, duplicate-key rejection
- 📝 **Write-Ahead Log** — records INSERTs to disk during a transaction
- 💾 **Atomic saves** — temp file + rename prevents corruption on crash
- 📦 **Group Commit buffer** — batched disk writes
- 🔒 **Global write lock** — serializes all SQL execution (SRWLOCK)
- ⛓️ **FK CASCADE** — `ON DELETE CASCADE` referential integrity (INTEGER FKs only)

### 🌐 Networking

- 🔌 **TCP server** — multi-threaded client-server mode
- 🌍 **HTTP server** — built-in web dashboard
- 💬 **WebSocket** — handshake and frame parsing implemented
- 🐍 **Cross-language** — works with Python, Node.js, telnet

---

## ⚠️ Known Limitations

HeavenDB is an **educational project**, not a production database. Here's what it can't do:

### 🔒 Concurrency
- **Writers serialize.** A global `SRWLOCK` in `sql_execute` means **only one SQL statement runs at a time**, including reads. Concurrent clients block each other.
- **No transaction isolation levels.** Nested transactions are not supported beyond `SAVEPOINT`.
- **Reads can be stale.** Multi-statement transactions are not visible to other clients until commit.

### 📊 Performance Reality
- **`GROUP BY`, `ORDER BY`, and `JOIN` are O(n²)** naive algorithms. Fine for thousands of rows; not for millions.
- **B-Tree indexes support only `INTEGER` columns.** `TEXT`, `UUID`, `DATE` columns do full table scans.
- **Every `INSERT`, `UPDATE`, and `DELETE` rewrites the entire SQL database file** (`sql_save`). This is O(n) per write. A future version will use the WAL for incremental writes.
- **Benchmark numbers are measured at the hashmap layer, not the SQL layer.** The SQL layer adds parsing, planning, and projection overhead. Treat the "65,000 ops/sec" figure as a hashmap microbenchmark, not a database throughput number.

### 📝 Write-Ahead Log (WAL) and Durability
- The WAL records every `INSERT` to disk in **real-time** during a transaction.
- On startup, `wal_recover_check()` checks for an interrupted transaction and discards it (**undo-only**).
- **Durability for committed transactions comes from the atomic `.hdb` save**, not the WAL. When you `COMMIT`, the entire database is written via temp-file-plus-rename.
- The WAL does **not** replay committed operations — the main `.hdb` file is authoritative.

### 🔗 Joins
- `INNER`, `LEFT`, `RIGHT`, `FULL`, `CROSS` joins work with **simple `ON table1.col = table2.col` equality conditions only.**
- **Not supported:** Multi-column `ON` conditions, non-equality operators (`>`, `<`), joins on `TEXT` / `UUID` / `DATE` columns.

### 🔀 Query Clause Combinations
- **`WHERE` combined with `ORDER BY`, `LIMIT`, `GROUP BY`, or joins is not supported.** The dispatcher processes one clause per query. If you write `SELECT * FROM t WHERE x > 1 ORDER BY y`, the WHERE clause is silently ignored.
- **Compound `WHERE` supports at most two conditions** (`a AND b` or `a OR b`), not three or more.
- **`UNION` and `UNION ALL` are identical** — both concatenate without deduplication.

### ⛓️ FK CASCADE
- `ON DELETE CASCADE` rewrites the entire database file on each cascade delete — O(n) cost per cascade.
- Only `INTEGER` foreign keys are supported.

### 📡 Replication
- **Replication is a stub.** `REPLICATE TO` registers a replica address in memory; `SYNC` returns success but does not sync data. There is no streaming replication, no master-slave, and no conflict resolution.

### 🧪 Testing
- **11 unit tests** for `hashmap.c`.
- **`bugtest.sql`** — a small regression suite covering type handling, duplicate keys, and ordering.
- **No unit tests** for `btree.c`, `wal.c`, `buffer.c`, `auth.c`, or `sql.c`.
- **Integration tests** (`test3.sql`, `bugtest.sql`) verify that commands execute without producing `"ERROR"` in the output — they do not check query results against expected values. This means many classes of incorrect results would pass the tests silently.

### 🌍 Portability
- The networking layer uses **Winsock** and **BCrypt** (Windows-only). Linux/macOS require a POSIX port.

### 🔐 Security Caveats
- Passwords use **PBKDF2-HMAC-SHA256** with 100,000 iterations and per-user salts generated from `BCryptGenRandom`. This part is solid.
- **No constant-time password comparison** — `memcmp` on the password hash is not timing-safe.
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

### In-Memory GET (varying working set sizes)

| Working Set | Throughput | CPU Cache Level |
|:-----------:|:----------:|-----------------|
| 100 keys | **~8,300,000 ops/sec** | L1 cache |
| 1,000 keys | **~7,100,000 ops/sec** | L1/L2 cache |
| 10,000 keys | **~5,900,000 ops/sec** | L2 cache |
| 100,000 keys | **~2,200,000 ops/sec** | L3 cache |
| 1,000,000 keys | **~65,000 ops/sec** | Main memory |

### Disk Operations

| Operation | Throughput | Notes |
|-----------|:----------:|-------|
| Disk SET (Group Commit) | **~7,800 ops/sec** | Batched writes, 100 per flush |
| PBKDF2 password hash | **~50 hashes/sec** | Intentional — 100k SHA-256 iterations |

> ⚠️ **These are local machine numbers under ideal conditions.** They are **not** production benchmarks.

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
│         │  (SRWLOCK: one statement at a time) │             │
│         └────────────────┬────────────────────┘             │
│                          ▼                                  │
│   ┌──────────────────────────────────────────────────┐      │
│   │       In-Memory Hash Map  (O(1) lookups)         │      │
│   └──────────────────────────────────────────────────┘      │
│   ┌──────────────────────────────────────────────────┐      │
│   │       B-Tree Indexes  (O(log n), unique keys)    │      │
│   └──────────────────────────────────────────────────┘      │
│   ┌──────────────────────────────────────────────────┐      │
│   │  Append-Only Storage  (.hdb, atomic temp+rename) │      │
│   └──────────────────────────────────────────────────┘      │
│   ┌──────────────────────────────────────────────────┐      │
│   │       Group Commit Buffer  (batched writes)      │      │
│   └──────────────────────────────────────────────────┘      │
│   ┌──────────────────────────────────────────────────┐      │
│   │       Write-Ahead Log  (real-time disk writes)   │      │
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
    src/main.c src/database.c src/hashmap.c src/buffer.c \
    src/table.c src/sql.c src/btree.c src/wal.c \
    src/tcp_server.c src/auth.c src/auth_storage.c \
    src/permissions.c src/replication.c src/websocket.c \
    src/http_server.c -lws2_32 -lbcrypt
```

> 🚧 A full POSIX socket port is on the roadmap. Until then, use [WSL](https://learn.microsoft.com/en-us/windows/wsl/) for full cross-platform support.

---

## 🚀 Quick Start

```bash
heavendb shell             # Interactive shell
heavendb serve             # TCP + HTTP server (dashboard at :8080)
heavendb run script.sql    # Execute SQL script
heavendb benchmark 100000  # Runs 100,000 lookups at each working-set size
```

### First-Run Setup

On the very first run, HeavenDB generates a **random admin password** (via the Windows CSPRNG) and prints it once to **stderr**. It's also saved to `~/.heavendb_initial_password` as a backup.

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

**Backup file lifecycle:**
- On first run, the password is also written to `%USERPROFILE%\.heavendb_initial_password`.
- The file is created with default OS permissions — **not** the strictest available. This is a known limitation.
- **HeavenDB does not delete this file automatically.** Delete it manually after your first login:

```bash
del "%USERPROFILE%\.heavendb_initial_password"
```

### Crash Recovery

If the process is killed mid-transaction, HeavenDB recovers cleanly on next start:

```
$ heavendb shell
WAL Recovery: Found interrupted transaction (1 inserts) -- DISCARDED
Auth system loaded. 1 user(s) registered.
```

Uncommitted changes are rolled back. Committed data survives. Atomic saves prevent file corruption.

---

## 📝 SQL Reference

### Creating Tables

```sql
DROP TABLE IF EXISTS users;

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

-- Sorting (note: not combinable with WHERE)
SELECT * FROM users ORDER BY salary DESC;
SELECT * FROM users LIMIT 10 OFFSET 5;

-- Aggregation
SELECT COUNT(*) FROM users;
SELECT AVG(salary) FROM users;
SELECT age, COUNT(*) FROM users GROUP BY age HAVING COUNT(*) > 1;
SELECT GROUP_CONCAT(name) FROM users;

-- Joins (INTEGER equality only)
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

HeavenDB enforces **min 8 chars, uppercase, lowercase, digit** for any new password. Passwords are hashed with **PBKDF2-HMAC-SHA256 (100,000 iterations)** and a per-user random salt from `BCryptGenRandom`.

Permissions are enforced: a user without `SELECT` on a table cannot read it; without `DELETE`, cannot drop or truncate; etc. The `admin` user bypasses all checks.

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
> Read until you see the terminator line. **For robustness, match the full terminator pattern (a line that starts with `(` and ends with `rows)` or is exactly `(nil)`), not just the substring `rows)`.**

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

Covers: creation, set/get, updates, deletion, missing keys, 10,000-key inserts, hash collisions, empty values, 1000-char values, and edge cases.

### Regression Test — `bugtest.sql`

Covers FLOAT comparisons, BOOLEAN comparisons, `ORDER BY` on FLOAT, `DELETE` on FLOAT, `UPDATE` on BOOLEAN, `AND`/`OR` on FLOAT, and duplicate primary key rejection.

```bash
heavendb run bugtest.sql
```

### Integration Test — `test3.sql`

Broad coverage of SQL features. The runner reports errors but does not assert expected results.

```bash
heavendb run test3.sql
```

### CI / CD

Every push and pull request to `main` triggers GitHub Actions:

- Compiles HeavenDB with GCC on Windows
- Runs all 11 unit tests
- Runs the integration test
- Fails the build if any error is detected

[View CI runs →](https://github.com/Aditya-cyber-hind/HeavenDB/actions)

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
│   ├── wal.c/h             # Write-Ahead Log + undo-only crash recovery
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
├── .github/
│   └── workflows/
│       └── ci.yml          # GitHub Actions CI
│
├── bugtest.sql             # Regression tests for type handling
├── test3.sql               # Broad SQL integration test
├── README.md
└── LICENSE
```

> 📝 **Note:** Files marked `(Windows-only)` are excluded from a Linux/macOS build. See the [Building](#-building) section.

---

## 🗺️ Roadmap

<details open>
<summary><b>✅ Completed</b></summary>

- [x] Hash Map storage engine
- [x] Group Commit buffering
- [x] SQL parser & tokenizer
- [x] B-Tree indexes with proper rebalancing
- [x] B-Tree duplicate key rejection
- [x] Persistent storage with **atomic saves** (temp file + rename)
- [x] **WAL real-time writes + undo-only crash recovery**
- [x] In-session transactions with SAVEPOINT
- [x] TCP + HTTP servers with web dashboard
- [x] **PBKDF2-HMAC-SHA256** password hashing
- [x] **BCryptGenRandom** for salts, passwords, and UUIDs
- [x] **Random password on first run** (no default credentials)
- [x] **Global SQL lock** (SRWLOCK) for safe concurrent clients
- [x] User authentication with account lockout
- [x] `GRANT` / `REVOKE` permissions — **enforced** on all handlers
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
- [x] **Regression test for type handling and constraints**
- [x] **GitHub Actions CI**

</details>

<details>
<summary><b>🚧 In Progress (next up)</b></summary>

- [ ] Query clause dispatcher rewrite (support `WHERE` + `ORDER BY`, `WHERE` + `LIMIT`, `WHERE` + `GROUP BY` combinations)
- [ ] Compound `WHERE` with 3+ conditions (`a AND b AND c`)
- [ ] Incremental `sql_save` (WAL-backed; stop rewriting the whole DB per write)
- [ ] `btree_delete` for proper index maintenance
- [ ] Unit tests for `btree.c`, `wal.c`, `buffer.c`, `sql.c`
- [ ] POSIX socket port (Linux/macOS networking)
- [ ] Constant-time password hash comparison

</details>

<details>
<summary><b>🔮 Planned</b></summary>

- [ ] Window functions (`ROW_NUMBER`, `RANK`)
- [ ] Triggers
- [ ] Stored procedures
- [ ] Full-text search
- [ ] B-Tree support for TEXT / UUID / DATE columns
- [ ] Incremental FK CASCADE (no full-file rewrite)
- [ ] Real replication (streaming master-slave)
- [ ] Query optimizer
- [ ] Cost-based planner
- [ ] Columnar storage engine

</details>

---

## 🧑💻 Author

**Aditya** — systems programmer from India. Built HeavenDB at 13.

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