<div align="center">

# 🗄️ HeavenDB

**A SQL database engine built from scratch in pure C**

*B-Tree indexes, Write-Ahead Log, PBKDF2 auth, TCP/HTTP servers — no frameworks, no shortcuts*

[![C99](https://img.shields.io/badge/C-C99-22c55e?style=flat-square&logo=c&logoColor=white)](https://github.com/Aditya-cyber-hind/HeavenDB)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20MinGW-64748b?style=flat-square&logo=windows&logoColor=white)](https://github.com/Aditya-cyber-hind/HeavenDB)
[![License](https://img.shields.io/badge/License-MIT-f59e0b?style=flat-square)](LICENSE)
[![CI](https://img.shields.io/badge/CI-passing-22c55e?style=flat-square)](https://github.com/Aditya-cyber-hind/HeavenDB/actions)
[![Version](https://img.shields.io/badge/Version-v0.1.0--preview-0ea5e9?style=flat-square)](https://github.com/Aditya-cyber-hind/HeavenDB/releases)
[![LOC](https://img.shields.io/badge/LOC-~8000-blue?style=flat-square)]()
[![Tests](https://img.shields.io/badge/tests-11%20unit%20%2B%20regression-22c55e?style=flat-square)]()

[Features](#-features) · [Quick Start](#-quick-start) · [SQL Reference](#-sql-reference) · [HTTP API](#-http-api) · [Design Notes](#-design-notes) · [Limitations](#-known-limitations) · [FAQ](#-faq) · [Roadmap](#-roadmap)

</div>

---

## Table of Contents

- [What is HeavenDB?](#what-is-heavendb)
- [Why Build This?](#why-build-this)
- [Features](#-features)
- [Quick Start](#-quick-start)
- [Architecture](#-architecture)
- [SQL Reference](#-sql-reference)
- [HTTP API](#-http-api)
- [Cross-Language Clients](#-cross-language-clients)
- [Command-Line Reference](#-command-line-reference)
- [Configuration](#-configuration)
- [Web Dashboard](#-web-dashboard)
- [Testing](#-testing)
- [Design Notes](#-design-notes)
- [Performance](#-performance)
- [Known Limitations](#-known-limitations)
- [Project Structure](#-project-structure)
- [FAQ](#-faq)
- [Roadmap](#-roadmap)
- [Contributing](#-contributing)
- [License](#-license)

---

## What is HeavenDB?

HeavenDB is a **SQL database engine written from scratch in pure C**. Every B-Tree rebalancing, every WAL entry, every SQL token parsed is code written by hand — no third-party libraries, no frameworks, no shortcuts.

It's an **educational project** designed to demonstrate how real databases work: from raw byte storage to ACID transactions to SQL parsing to cryptographic password hashing.

> *"Anyone can `npm install sqlite3`. Very few can build the engine itself."*

The engine is roughly **8,000 lines of C** across 16 source files. It implements:

- A hand-written **tokenizer and recursive-descent parser** for SQL
- A **B-Tree** with insertion, rebalancing, range scans, and unique-key enforcement
- A **Write-Ahead Log** with two-pass undo-only crash recovery
- **Atomic file replacement** (temp + rename) for durability
- **SHA-256, HMAC-SHA256, and PBKDF2** implemented from the RFCs
- **Windows CSPRNG** (`BCryptGenRandom`) for salts, passwords, UUIDs, and session tokens
- **TCP, HTTP, and WebSocket** servers
- A **web dashboard** with login and real-time query execution

---

## Why Build This?

Because databases are the black box of modern software. Every app uses one; few developers know how they actually work.

I wanted to understand:

- How does a `SELECT` query know where to find a row without scanning the whole file?
- What actually happens when you `COMMIT` a transaction — and why does a crash not corrupt your data?
- How does an index make lookups faster?
- What does a SQL parser actually do?
- How does a database prevent two users from overwriting each other's writes?
- What does password hashing actually look like at the byte level?

So I read SQLite's internals, studied PostgreSQL's WAL design, and built HeavenDB to answer those questions by writing the code myself.

**This isn't a competitor to SQLite.** It's what I built to learn how SQLite works.

---

## ✨ Features

### SQL Engine

| Category | Support |
|----------|---------|
| **DML** | `SELECT`, `INSERT`, `UPDATE`, `DELETE` |
| **DDL** | `CREATE TABLE`, `DROP TABLE`, `ALTER TABLE` |
| **Views** | `CREATE VIEW`, `DROP VIEW` |
| **Indexes** | `CREATE INDEX` on INTEGER / BOOLEAN columns |
| **Joins** | `INNER`, `LEFT`, `RIGHT`, `FULL`, `CROSS` (equality only, INTEGER) |
| **Subqueries** | `EXISTS`, `NOT EXISTS`, `ANY`, `ALL`, `SOME`, scalar |
| **Set operations** | `UNION`, `UNION ALL` |
| **CTEs** | `WITH ... AS (...)` (single CTE) |
| **Transactions** | `BEGIN`, `COMMIT`, `ROLLBACK`, `SAVEPOINT`, `RELEASE`, `ROLLBACK TO` |

### Data Processing

| Category | Support |
|----------|---------|
| **Aggregates** | `COUNT`, `SUM`, `AVG`, `MIN`, `MAX`, `GROUP_CONCAT` on INTEGER / FLOAT / BOOLEAN |
| **Grouping** | `GROUP BY` on all column types, `HAVING` |
| **Sorting** | `ORDER BY` (ASC/DESC), `LIMIT`, `OFFSET`, `DISTINCT` |
| **Filtering** | `WHERE` with `AND` / `OR` precedence, parentheses, all comparison operators |
| **Patterns** | `LIKE`, `BETWEEN`, `IN`, `IS NULL` |
| **Conditional** | `CASE WHEN ... THEN ... ELSE ... END` |

### Data Types

| Type | Storage | Comparison |
|------|---------|-----------|
| `INTEGER` | 4-byte int | Numeric |
| `FLOAT` | 8-byte double | Numeric |
| `BOOLEAN` | 4-byte int (0/1) | Numeric |
| `TEXT` | Variable-length string | Lexicographic |
| `UUID` | 36-char string | Lexicographic |
| `JSON` | Variable-length string | Lexicographic |
| `DATE` | `YYYY-MM-DD` string | Lexicographic |
| `TIMESTAMP` | `YYYY-MM-DD HH:MM:SS` string | Lexicographic |

### Built-in Functions

| String | Math | JSON | Date / Time |
|---|---|---|---|
| `UPPER` | `ABS` | `json_extract` | `NOW` |
| `LOWER` | `ROUND` | `json_set` | `CURRENT_DATE` |
| `LENGTH` | `FLOOR` | | `YEAR` |
| `TRIM` | `CEIL` | | `MONTH` |
| `SUBSTR` | `MOD` | | `DAY` |
| `CONCAT` | | | |

### Security

- 🔐 **PBKDF2-HMAC-SHA256** password hashing with **100,000 iterations** and per-user random salts
- 🎲 **BCryptGenRandom** for salts, passwords, UUIDs, and session tokens
- 🔒 Account lockout after 5 failed login attempts
- ✅ Password validation (min 8 chars, uppercase, lowercase, digit)
- 👥 User management: `CREATE USER`, `LOGIN`, `LOGOUT`, `CHANGE PASSWORD`
- 🎫 Permissions: `GRANT`, `REVOKE` per user / per table — **enforced on all handlers**
- 🔑 **HTTP session-token authentication** — 30-minute expiry, 256-bit tokens
- 🛡️ **Path traversal protection** on the HTTP file server

### Storage Engine

- 🚀 **In-memory hash map** — O(1) key-value lookups
- 🌳 **B-Tree indexes** — O(log n) lookups, duplicate rejection
- 📝 **Write-Ahead Log** — real-time INSERT logging during transactions
- 💾 **Atomic saves** — temp file + rename prevents corruption on crash
- 📦 **Group Commit buffer** — batched disk writes
- 🔒 **Global write lock** — serializes all SQL execution (`SRWLOCK`)
- ⛓️ **Foreign keys** with `ON DELETE CASCADE`
- 🔄 **Undo-only crash recovery** — uncommitted transactions discarded on restart

### Networking

- 🔌 **TCP server** on port 6379 — multi-threaded, telnet-compatible
- 🌍 **HTTP server** on port 8080 — REST-style query endpoint
- 💬 **WebSocket** — handshake and frame parsing implemented
- 🌐 **Web dashboard** — login, table list, query execution, HTML table rendering

---

## Quick Start

### 1. Build

**Windows (MinGW / MSYS2):**

```bash
# Clone
git clone https://github.com/Aditya-cyber-hind/HeavenDB.git
cd HeavenDB

# Build — uses the provided batch files
build.bat          # dev build, fast compile (-O0)
build-release.bat  # release build, fast binary (-O2)
```

**Or compile manually:**

```bash
gcc -Wall -Wextra -Wno-switch -O2 -std=c99 -o heavendb.exe src\*.c -lws2_32 -lbcrypt
```

### 2. Run

```bash
heavendb shell             # Interactive SQL shell
heavendb serve             # TCP + HTTP server (dashboard at :8080)
heavendb run script.sql    # Execute a SQL file
heavendb benchmark 100000  # Run performance benchmarks
```

### 3. Query

```sql
CREATE TABLE users (
    id INTEGER PRIMARY KEY AUTO_INCREMENT,
    name TEXT NOT NULL,
    age INTEGER,
    salary FLOAT,
    active BOOLEAN
);

INSERT INTO users VALUES (NULL, 'Aditya', 25, 85000.50, TRUE);
INSERT INTO users VALUES (NULL, 'Rahul',  19, 65000.75, FALSE);

SELECT * FROM users WHERE age > 20 AND active = TRUE ORDER BY salary DESC LIMIT 10;
```

### 4. First-run setup

On the very first run, HeavenDB generates a **random admin password** using the Windows CSPRNG and prints it once to stderr:

```
+==========================================================+
|           HeavenDB -- FIRST RUN SETUP                    |
+==========================================================+
|  Username: admin                                         |
|  Password: Tj45bZi128yB%6A1R9BrnWm                       |
|                                                          |
|  !!! COPY THIS PASSWORD NOW -- it will NOT be shown !!!  |
|      again. Change it immediately after first login:     |
|                                                          |
|      CHANGE PASSWORD 'YourNewSecure456'                  |
+==========================================================+
```

Then change it in your first session:

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
- The file is created with default OS permissions — **not** the strictest available.
- **HeavenDB does not delete this file automatically.** Delete it manually after your first login:

```bash
del "%USERPROFILE%\.heavendb_initial_password"
```

---

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                         HEAVENDB                            │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│    ┌──────────┐      ┌──────────┐      ┌──────────┐         │
│    │   CLI    │      │   TCP    │      │   HTTP   │         │
│    │  Client  │      │  Server  │      │  Server  │         │
│    └────┬─────┘      └────┬─────┘      └────┬─────┘         │
│         │                 │                  │              │
│         └─────────────────┼──────────────────┘              │
│                           ▼                                 │
│         ┌─────────────────────────────────────┐             │
│         │  SQL Tokenizer → Parser → Executor  │             │
│         │  (SRWLOCK: one statement at a time) │             │
│         │  WHERE: predicate tree with AND/OR  │             │
│         └────────────────┬────────────────────┘             │
│                          ▼                                  │
│   ┌──────────────────────────────────────────────────┐      │
│   │       In-Memory Hash Map  (O(1) lookups)         │      │
│   ├──────────────────────────────────────────────────┤      │
│   │  B-Tree Indexes  (O(log n), PK/UNIQUE only)      │      │
│   ├──────────────────────────────────────────────────┤      │
│   │  Atomic Storage  (.hdb, temp+rename)             │      │
│   ├──────────────────────────────────────────────────┤      │
│   │  Write-Ahead Log  (undo-only recovery)           │      │
│   └──────────────────────────────────────────────────┘      │
│                                                             │
│   ┌───────────┐  ┌─────────────┐  ┌──────────────┐          │
│   │ PBKDF2    │  │ Permissions │  │ Replication  │          │
│   │  Auth     │  │  GRANT/REV  │  │  (stubbed)   │          │
│   └───────────┘  └─────────────┘  └──────────────┘          │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### Request flow

A query from the CLI shell passes through:

1. **Read line** from stdin
2. **Tokenize** — split into words, handle quoted strings, operators
3. **Dispatch** — look at the first token, route to the right handler
4. **Parse** — each handler parses its own clause (WHERE, ORDER BY, etc.)
5. **Execute** — scan/filter/sort rows in memory
6. **Serialize** — write the table to `heaven_sql.hdb` (atomic rename)
7. **Print** — column headers, rows, and a `(N rows)` terminator

A query from the HTTP server adds two more layers:

- **Session validation** — check the `Authorization: Bearer` token
- **Permission check** — verify the current user has rights on the table

### Storage layout

HeavenDB keeps three on-disk files:

| File | Purpose | Format |
|------|---------|--------|
| `heaven_sql.hdb` | SQL tables (snapshot) | Binary with magic header |
| `heaven_users.hdb` | Users and password hashes | Binary with magic header |
| `heaven.wal` | Write-ahead log (during transactions) | Append-only records |

Both `.hdb` files use the same pattern: write to `.tmp`, `fclose`, `remove` the old file, `rename` the new one. If the process dies at any point, either the old file or the new file exists — never a half-written one.

---

## SQL Reference

### Data Definition

```sql
-- Create a table with constraints
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

-- Foreign key with cascade delete
CREATE TABLE orders (
    id INTEGER PRIMARY KEY AUTO_INCREMENT,
    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    total FLOAT,
    status TEXT
);

-- Drop a table
DROP TABLE users;
DROP TABLE IF EXISTS users;   -- no error if it doesn't exist

-- Alter a table
ALTER TABLE users ADD COLUMN phone TEXT;
ALTER TABLE users DROP COLUMN phone;

-- Truncate (delete all rows, keep schema)
TRUNCATE TABLE users;

-- Indexes
CREATE INDEX idx_user_age ON users(age);
```

### Data Manipulation

```sql
-- Insert
INSERT INTO users VALUES (NULL, 'Aditya', 'a@x.com', 25, 85000.50, TRUE, '{"role":"engineer"}', NOW());

-- Update
UPDATE users SET salary = 90000 WHERE id = 1;
UPDATE users SET active = FALSE WHERE age > 60;

-- Delete
DELETE FROM users WHERE id = 1;
```

### SELECT — Filtering

```sql
-- Simple
SELECT * FROM users;
SELECT name, age FROM users;

-- Comparisons
SELECT * FROM users WHERE age > 25;
SELECT * FROM users WHERE age >= 25 AND age <= 35;
SELECT * FROM users WHERE salary != 50000;

-- Compound conditions (AND has higher precedence than OR)
SELECT * FROM users WHERE age > 20 AND age < 30 AND active = TRUE;
SELECT * FROM users WHERE active = TRUE OR (city = 'Mumbai' AND age < 25);

-- Parentheses override precedence
SELECT * FROM users WHERE (active = TRUE OR city = 'Mumbai') AND age < 40;

-- BETWEEN
SELECT * FROM users WHERE age BETWEEN 20 AND 30;

-- LIKE
SELECT * FROM users WHERE name LIKE 'A%';
SELECT * FROM users WHERE email LIKE '%@gmail.com';

-- IN
SELECT * FROM users WHERE id IN (1, 2, 3);
SELECT * FROM orders WHERE status IN ('completed', 'pending');

-- IS NULL
SELECT * FROM users WHERE phone IS NULL;
SELECT * FROM users WHERE phone IS NOT NULL;

-- CASE WHEN
SELECT name, CASE WHEN salary > 80000 THEN 'High' ELSE 'Normal' END FROM users;
```

### SELECT — Sorting and Limiting

```sql
-- Sorting
SELECT * FROM users ORDER BY age ASC;
SELECT * FROM users ORDER BY salary DESC;

-- Limiting
SELECT * FROM users LIMIT 10;
SELECT * FROM users LIMIT 10 OFFSET 20;

-- All together: filter, sort, then limit
SELECT * FROM users WHERE active = TRUE ORDER BY salary DESC LIMIT 5;
```

### SELECT — Aggregates

```sql
-- Counting
SELECT COUNT(*) FROM users;

-- Numeric aggregates
SELECT SUM(salary) FROM users;
SELECT AVG(salary) FROM users;
SELECT MIN(salary) FROM users;
SELECT MAX(salary) FROM users;

-- Grouping
SELECT city, COUNT(*) FROM users GROUP BY city;
SELECT age, AVG(salary) FROM users GROUP BY age HAVING COUNT(*) > 1;

-- Sorting groups
SELECT city, COUNT(*) FROM users GROUP BY city ORDER BY city ASC;
SELECT city, COUNT(*) FROM users GROUP BY city ORDER BY city DESC;

-- Group concatenation
SELECT GROUP_CONCAT(name) FROM users;
SELECT city, GROUP_CONCAT(name) FROM users GROUP BY city;
```

### SELECT — Joins

```sql
-- Inner join
SELECT * FROM users
INNER JOIN orders ON users.id = orders.user_id;

-- Left, right, full
SELECT * FROM users LEFT  JOIN orders ON users.id = orders.user_id;
SELECT * FROM users RIGHT JOIN orders ON users.id = orders.user_id;
SELECT * FROM users FULL  JOIN orders ON users.id = orders.user_id;

-- Cross join
SELECT * FROM users CROSS JOIN departments;
```

> **Note:** Joins only support equality on INTEGER columns. `WHERE` clauses on joins are not yet applied.

### SELECT — Subqueries

```sql
-- Scalar subquery in WHERE
SELECT * FROM users WHERE age > (SELECT AVG(age) FROM users);

-- EXISTS
SELECT * FROM users
WHERE EXISTS (SELECT 1 FROM orders WHERE orders.user_id = users.id);

-- NOT EXISTS
SELECT * FROM users
WHERE NOT EXISTS (SELECT 1 FROM orders WHERE orders.user_id = users.id);

-- ANY / ALL
SELECT * FROM users WHERE salary > ANY (SELECT salary FROM employees);
SELECT * FROM users WHERE salary > ALL (SELECT salary FROM employees);
```

### SELECT — Set Operations

```sql
SELECT name FROM users UNION     SELECT name FROM employees;
SELECT name FROM users UNION ALL SELECT name FROM employees;
```

### Built-in Function Examples

```sql
-- String
SELECT UPPER(name) FROM users;
SELECT LOWER(email) FROM users;
SELECT LENGTH(name) FROM users;
SELECT TRIM('   hello   ');
SELECT SUBSTR('HeavenDB', 1, 6);
SELECT CONCAT(name, ' <', email, '>') FROM users;

-- Math
SELECT ABS(-42);
SELECT ROUND(3.14159, 2);
SELECT FLOOR(99.9);
SELECT CEIL(0.01);
SELECT MOD(100, 7);

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

### Security & Administration

```sql
-- Users
CREATE USER alice WITH PASSWORD 'AlicePass789';
LOGIN admin WITH PASSWORD 'AdminPass123';
LOGOUT;
CHANGE PASSWORD 'NewSecure456';

-- Permissions
GRANT  SELECT ON users TO alice;
GRANT  ALL ON users TO alice;
REVOKE DELETE ON users FROM alice;

-- Inspection
SHOW TABLES;
DESCRIBE users;
EXPLAIN SELECT * FROM users WHERE salary > 70000;

-- Backup
BACKUP TO 'backup.hdb';
```

### Views

```sql
CREATE VIEW active_users AS SELECT * FROM users WHERE active = TRUE;
SELECT * FROM active_users;
DROP VIEW active_users;
```

---

## HTTP API

The HTTP server runs on port 8080 by default. Every request except `POST /login` requires a session token.

### `POST /login`

Authenticates a user and returns a session token.

**Request:**
```http
POST /login HTTP/1.1
Content-Type: application/x-www-form-urlencoded

username=admin&password=YourPassword
```

**Response (200):**
```json
{"token":"a1b2c3d4e5f6..."}
```

**Response (401):**
```
Invalid credentials
```

**Session lifetime:** 30 minutes from creation. Tokens are 256-bit random values, hex-encoded.

### `GET /query`

Executes a SQL statement.

**Request:**
```http
GET /query?sql=SHOW%20TABLES HTTP/1.1
Authorization: Bearer <token>
```

**Response (200):** the query result as plain text, same format as the CLI.

**Response (401):**
```
Missing or invalid Authorization header
```
or
```
Invalid or expired token
```

### `GET /`

Serves the web dashboard (`dashboard/index.html`).

### `GET /<file>`

Serves static files from the `dashboard/` folder. Path traversal is blocked.

---

## Cross-Language Clients

### Protocol

**TCP (port 6379):** Send a SQL statement followed by `\n`. Read until you see `(N rows)` or `(nil)`.

**HTTP (port 8080):** Authenticate via `POST /login`, then send `GET /query?sql=<urlencoded>` with `Authorization: Bearer <token>`.

### Python (TCP)

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

### Node.js (TCP)

```javascript
const net = require('net');
const client = new net.Socket();

client.connect(6379, 'localhost', () => {
    client.write('SELECT * FROM users\n');
});

client.on('data', (data) => console.log(data.toString()));
```

### curl (HTTP)

```bash
# Log in and capture the token
TOKEN=$(curl -s -X POST \
    --data-urlencode "username=admin" \
    --data-urlencode "password=YourPassword" \
    http://localhost:8080/login | jq -r .token)

# Run a query
curl -H "Authorization: Bearer $TOKEN" \
     "http://localhost:8080/query?sql=SHOW%20TABLES"
```

### telnet

```bash
telnet localhost 6379
```

---

## Command-Line Reference

| Command | Description |
|---------|-------------|
| `heavendb shell` | Interactive SQL shell |
| `heavendb serve [port]` | Start TCP + HTTP server (default TCP: 6379, HTTP: 8080) |
| `heavendb run <file.sql>` | Execute a SQL script file |
| `heavendb sql "<query>"` | Execute a single SQL statement |
| `heavendb set <key> <value>` | Store a key-value pair in the KV store |
| `heavendb get <key>` | Retrieve a value from the KV store |
| `heavendb delete <key>` | Delete a key |
| `heavendb size` | Show the number of keys in the KV store |
| `heavendb flush` | Force KV writes to disk |
| `heavendb benchmark <n>` | Run benchmarks |
| `heavendb help` | Show usage |

---

## Configuration

### Environment variables

| Variable | Purpose |
|---|---|
| `HEAVENDB_INITIAL_PASSWORD` | Set the admin password on first run (instead of random) |

### Files created at runtime

| File | Purpose | Gitignored |
|---|---|---|
| `heaven.hdb` | KV store data | ✅ |
| `heaven_sql.hdb` | SQL tables | ✅ |
| `heaven_users.hdb` | User accounts and hashes | ✅ |
| `heaven.wal` | Write-ahead log (only during transactions) | ✅ |
| `%USERPROFILE%\.heavendb_initial_password` | First-run password backup | ❌ (outside repo) |

### Ports

| Port | Protocol | Purpose |
|---|---|---|
| 6379 | TCP | SQL over raw TCP (redis-compatible port number) |
| 8080 | HTTP | REST API + web dashboard |

---

## Web Dashboard

The dashboard is served at `http://localhost:8080` when you run `heavendb serve`.

**Features:**
- Login form (uses the same credentials as the SQL layer)
- Table list in the sidebar — click a table to auto-run `SELECT *`
- Query input with monospace font and Enter-to-run
- Real HTML tables for `SELECT` results
- Green/red status messages
- Session persists in `sessionStorage` across refreshes
- Logout button

**Layout:**
- Top bar: brand, "Logged in as admin", logout
- Left sidebar: table list with refresh button
- Main area: query bar, status line, results panel

**Files:**

```
dashboard/
├── index.html    # Layout
├── style.css     # Dark theme
└── app.js        # Login, query, table rendering
```

---

## Testing

### Unit tests — hashmap

**11 unit tests** covering creation, set/get, updates, deletion, missing keys, 10,000-key inserts, hash collisions, empty values, 1000-char values, and edge cases.

```bash
gcc -Wall -Wextra -O2 -std=c99 -o tests\test_hashmap.exe tests\test_hashmap.c src\hashmap.c
tests\test_hashmap.exe
```

Expected output:
```
════════════════════════════════════════
  Passed: 11
  Failed: 0
════════════════════════════════════════
```

### Regression tests — `bugtest.sql`

Covers FLOAT comparisons, BOOLEAN comparisons, `ORDER BY` on FLOAT, `DELETE` on FLOAT, `UPDATE` on BOOLEAN, compound `WHERE` with 3+ conditions, duplicate primary key rejection, and `WHERE + ORDER BY + LIMIT` combinations.

```bash
heavendb run bugtest.sql
```

### CI / CD

Every push and pull request to `main` triggers GitHub Actions:

- Compiles HeavenDB with GCC on Windows
- Runs all 11 hashmap unit tests
- Runs the integration test (`test3.sql`)
- Fails the build if any `ERROR` appears in the output

[View CI runs →](https://github.com/Aditya-cyber-hind/HeavenDB/actions)

---

## Design Notes

### Why a hash map and a B-Tree?

The hash map handles point lookups (`WHERE key = X`) in O(1). The B-Tree handles range queries and ordered iteration in O(log n). Real databases use both for different access patterns. HeavenDB builds both to demonstrate the tradeoffs.

### Why atomic saves instead of incremental updates?

Incremental updates require careful handling of partial writes at every byte boundary. Atomic replacement — write to `.tmp`, rename over the original — guarantees that the file on disk is always either the old state or the new state, never a mixture.

This is what SQLite does with `PRAGMA journal_mode=WAL` and `PRAGMA synchronous=FULL`. It's the simplest correct approach for an educational project.

### Why a Write-Ahead Log if saves are atomic?

The WAL records each INSERT **during** a transaction, before the in-memory state changes. If the process crashes mid-transaction, the WAL contains the uncommitted operations, and on restart they're discarded. The `.hdb` snapshot is only written on `COMMIT` — that's the durable point.

This is the standard "undo-only" WAL design. It's what SQLite used before the `WAL` journal mode was introduced.

### Why PBKDF2?

PBKDF2 is the oldest and simplest of the password hashing functions in common use. It's built on HMAC, which is built on a hash function — in this case SHA-256. Implementing it from scratch means implementing SHA-256, HMAC, and PBKDF2, all of which are standard and well-specified.

Modern systems should use Argon2id or bcrypt. PBKDF2 is used here because it's the easiest to implement correctly from the RFCs.

### Why a global SQL lock?

`sql_execute` mutates global state — the `tables[]` array, the current logged-in user, the active WAL. Allowing two threads to run `sql_execute` concurrently would corrupt this state. A global `SRWLOCK` serializes all SQL execution, which is correct but not fast.

A real database would use per-table locks or MVCC. HeavenDB takes the simple approach: one statement at a time.

### Why does the CLI accept one statement per line?

The `run` command reads a `.sql` file line by line, treating each line as a complete statement. This is simpler than parsing multi-line statements, which would require a proper SQL grammar with semicolon detection. As a result, statements in a `.sql` file must be on a single line.

### Why does `ORDER BY` not use the B-Tree?

Because the query planner doesn't exist yet. `ORDER BY` does an in-memory insertion sort. This is O(n²), which is fine for thousands of rows and unacceptable for millions. Wiring `ORDER BY` to walk the B-Tree in order is a future improvement.

---

## Performance

Measured on a standard development laptop (Windows, 4-core CPU, 8GB RAM).

> **Measurement note:** These numbers are measured at the **hash map layer** (`hashmap_get()` directly). SQL `SELECT` adds parsing, planning, and projection overhead on top. Expect significantly lower throughput through the SQL layer.

### In-Memory GET (varying working set sizes)

| Working Set | Throughput | CPU Cache Level |
|:-----------:|:----------:|:---------------:|
| 100 keys | ~8,300,000 ops/sec | L1 cache |
| 1,000 keys | ~7,100,000 ops/sec | L1/L2 cache |
| 10,000 keys | ~5,900,000 ops/sec | L2 cache |
| 100,000 keys | ~2,200,000 ops/sec | L3 cache |
| 1,000,000 keys | ~65,000 ops/sec | Main memory |

The **128× drop** from 100 to 1M keys is the cost of memory hierarchy. At 100 keys, everything fits in L1. At 1M keys, every lookup is a main-memory access.

### Disk Operations

| Operation | Throughput | Notes |
|-----------|:----------:|-------|
| Disk SET (Group Commit) | ~7,800 ops/sec | Batched writes, 100 per flush |
| PBKDF2 password hash | ~50 hashes/sec | Intentional — 100k SHA-256 iterations |

### SQL Layer

| Operation | Complexity | Notes |
|-----------|:----------:|-------|
| `SELECT` (no WHERE) | O(n) | Full table scan |
| `SELECT ... WHERE pk = X` | O(n) | Full scan (index not wired to queries) |
| `SELECT ... WHERE <cond>` | O(n) | Full scan + predicate eval |
| `ORDER BY` | O(n²) | Insertion sort in memory |
| `GROUP BY` | O(n²) | Nested loop grouping |
| `INSERT` | O(n) | Full file rewrite (`sql_save`) |
| `UPDATE` | O(n) | Full scan + full file rewrite |
| `DELETE` | O(n) | Full scan + shift + full file rewrite |

---

## Known Limitations

HeavenDB is an **educational project**, not a production database. Here's what it can't do:

### Concurrency

- **Writers serialize.** A global `SRWLOCK` means only one SQL statement runs at a time, including reads.
- **No transaction isolation levels.** Nested transactions are not supported beyond `SAVEPOINT`.
- **Reads can be stale.** Multi-statement transactions are not visible to other clients until commit.

### Performance

- **B-Tree indexes support only `INTEGER` / `BOOLEAN` columns.** TEXT, UUID, DATE do full table scans.
- **Every write rewrites the entire SQL file** (`sql_save`). O(n) per write.
- **`GROUP BY`, `ORDER BY`, and `JOIN` are O(n²)** naive algorithms. Fine for thousands of rows.
- **Benchmarks are at the hashmap layer**, not the SQL layer.

### Joins

- Equality only.
- INTEGER columns only.
- **`WHERE` clauses on joins are silently dropped.**
- Multi-column `ON` conditions not supported.

### Query Clause Combinations

- `WHERE + ORDER BY + LIMIT + GROUP BY` all work.
- **`WHERE + JOIN` does not work.**
- `ORDER BY` after `GROUP BY` works only on the group column itself.
- **`UNION` and `UNION ALL` are identical.**

### Replication

- **Stub.** `REPLICATE TO` registers a replica address in memory; `SYNC` returns success but doesn't sync data.

### Testing

- 11 unit tests for `hashmap.c`.
- No unit tests for `btree.c`, `wal.c`, `buffer.c`, `auth.c`, or `sql.c`.
- Integration tests check for absence of `"ERROR"` — not for correct output.

### Portability

- Winsock and BCrypt — **Windows-only**. Linux/macOS need a POSIX port.

### Security

- PBKDF2 and BCryptGenRandom are solid.
- **No constant-time password comparison.**
- **No prepared statements.**
- **No TLS.**
- **No encryption at rest.**

### Not Supported

Window functions · Triggers · Stored procedures · Full-text search · User-defined functions · Subqueries in `FROM` · Recursive CTEs · Streaming results

---

## Project Structure

```
HeavenDB/
├── src/
│   ├── main.c              # CLI entry point
│   ├── database.c/h        # Key-value storage engine
│   ├── hashmap.c/h         # Hash map
│   ├── btree.c/h           # B-Tree indexes
│   ├── buffer.c/h          # Group commit buffer
│   ├── wal.c/h             # Write-Ahead Log
│   ├── table.c/h           # Table structure & constraints
│   ├── sql.c/h             # SQL parser & executor
│   ├── auth.c/h            # PBKDF2 authentication
│   ├── auth_storage.c/h    # Persistent user storage
│   ├── session.c/h         # HTTP session tokens
│   ├── permissions.c/h     # GRANT / REVOKE
│   ├── replication.c/h     # Replication (stubbed)
│   ├── tcp_server.c/h      # TCP server
│   ├── http_server.c/h     # HTTP server
│   └── websocket.c/h       # WebSocket support
│
├── dashboard/
│   ├── index.html          # Dashboard layout
│   ├── style.css           # Dark theme
│   └── app.js              # Login, query, table rendering
│
├── tests/
│   └── test_hashmap.c      # 11 unit tests for the hash map
│
├── .github/
│   └── workflows/
│       └── ci.yml          # GitHub Actions CI
│
├── build.bat               # Dev build (-O0, fast compile)
├── build-release.bat       # Release build (-O2, fast binary)
├── bugtest.sql             # Regression tests
├── test3.sql               # Broad SQL integration test
├── README.md
└── LICENSE
```

---

## FAQ

### Is HeavenDB production-ready?

**No.** It's an educational project. Use SQLite for anything real.

### Is HeavenDB faster than SQLite?

**No.** SQLite is heavily optimized after 25 years of development. HeavenDB is a learning project.

### Why is `WHERE` on a JOIN not supported?

Because the WHERE clause in a JOIN references columns from **two** tables (`users.age`, `orders.status`). The current predicate parser only knows about one table. Fixing this requires extending `Predicate` to carry table information.

### Why does every `INSERT` take so long?

Because `handle_insert` calls `sql_save()` at the end, which writes the entire database file. For a 10 MB database, that's 10 MB of disk writes per insert.

The fix is to route DML through the WAL and only snapshot periodically. That's a future change.

### Why is `ORDER BY` so slow?

It's an insertion sort, which is O(n²). For a 10,000-row table, that's 50 million comparisons. A proper sort would use `qsort` (O(n log n)) or a B-Tree walk.

### Why does the CLI accept only one statement per line?

The `run` command reads the file line by line. Multi-line statements would require a semicolon-aware lexer. It's a convenience tradeoff.

### Can I use this on Linux?

Not yet. The networking layer uses Winsock and the crypto uses BCrypt. A POSIX port would replace these with `sys/socket.h` and `getrandom()` / `/dev/urandom`.

### How do I reset the admin password?

Delete `heaven_users.hdb` and restart. A new random password will be generated and printed to stderr.

### Can I use it as a library?

Not currently. The code assumes a `main()` and global state. Refactoring it into a library would require a context struct to replace the globals.

### Why 100,000 PBKDF2 iterations?

Because that's what OWASP recommended in 2017. Modern guidance is higher (600,000+). The number is a constant in `auth.h` — `PBKDF2_ITERATIONS`.

---

## Roadmap

<details open>
<summary><b>✅ Completed</b></summary>

- Hash map storage engine
- Group Commit buffering
- SQL parser and tokenizer
- B-Tree indexes with rebalancing
- B-Tree duplicate-key rejection (PK/UNIQUE only)
- Atomic saves (temp file + rename)
- WAL real-time writes + undo-only crash recovery
- Transactions with SAVEPOINT
- TCP and HTTP servers with web dashboard
- HTTP session-token authentication
- PBKDF2-HMAC-SHA256 password hashing
- BCryptGenRandom for salts, passwords, UUIDs, session tokens
- Random password on first run
- Global SQL lock (`SRWLOCK`)
- User authentication with account lockout
- `GRANT` / `REVOKE` permissions — enforced on all handlers
- All JOIN types (simple equality)
- Views
- Aggregates (INTEGER, FLOAT, BOOLEAN), GROUP BY, HAVING
- GROUP BY on all column types
- ORDER BY after GROUP BY
- Compound WHERE with 3+ conditions and AND/OR precedence
- WHERE + ORDER BY + LIMIT combinations
- Subqueries, EXISTS, ANY / ALL
- CASE WHEN
- 8 data types
- String, Math, JSON, and Date functions
- Primary key, unique, not null, auto-increment, FK CASCADE
- SAVEPOINT / RELEASE / ROLLBACK TO
- BACKUP, EXPLAIN, DROP TABLE IF EXISTS
- 11 unit tests for hashmap.c
- Regression test for type handling and clause combinations
- GitHub Actions CI
- build.bat / build-release.bat

</details>

<details>
<summary><b>🚧 In Progress</b></summary>

- **WHERE clause support in JOINs** (qualified and unqualified column names)
- Incremental `sql_save` (WAL-backed; no full-file rewrite per write)
- `btree_delete` for index maintenance
- Unit tests for `btree.c`, `wal.c`, `buffer.c`, `sql.c`
- POSIX socket port
- Constant-time password hash comparison

</details>

<details>
<summary><b>🔮 Planned</b></summary>

- Window functions (`ROW_NUMBER`, `RANK`)
- Triggers
- Stored procedures
- Full-text search
- B-Tree support for TEXT / UUID / DATE
- Incremental FK CASCADE
- Real streaming replication (master-slave)
- Query optimizer
- Cost-based planner
- Columnar storage engine

</details>

---

## Contributing

HeavenDB is a learning project, but contributions are welcome.

**How to contribute:**

1. Fork the repo
2. Create a feature branch (`git checkout -b feature/your-feature`)
3. Write code, add tests
4. Run the existing tests (`tests\test_hashmap.exe` and `heavendb run bugtest.sql`)
5. Commit with a clear message
6. Push to your fork
7. Open a Pull Request

**What I'm looking for:**

- Bug fixes (especially with a test that reproduces the bug)
- Documentation improvements
- Performance improvements with benchmarks
- POSIX port

**What I'm not looking for (yet):**

- New SQL features before the existing ones are solid
- Large refactors without discussion
- Dependencies on third-party libraries

---

## License

MIT License — free to use, modify, and distribute.

See [LICENSE](LICENSE) for full text.

---

<div align="center">

**Built from scratch. Built with obsession. Built in pure C.**

[Report a bug](https://github.com/Aditya-cyber-hind/HeavenDB/issues) · [Request a feature](https://github.com/Aditya-cyber-hind/HeavenDB/issues) · [View CI](https://github.com/Aditya-cyber-hind/HeavenDB/actions)

⭐ If HeavenDB impressed you, star the repository

</div>
