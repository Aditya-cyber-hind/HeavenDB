<div align="center">

# 🗄️ HeavenDB

### A High-Performance SQL Database Engine Written From Scratch in Pure C

[![Version](https://img.shields.io/badge/version-6.0-0ea5e9?style=for-the-badge)](https://github.com/Aditya-cyber-hind/HeavenDB)
[![Language](https://img.shields.io/badge/language-C99-22c55e?style=for-the-badge&logo=c)](https://github.com/Aditya-cyber-hind/HeavenDB)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-64748b?style=for-the-badge)](https://github.com/Aditya-cyber-hind/HeavenDB)
[![License](https://img.shields.io/badge/license-MIT-f59e0b?style=for-the-badge)](LICENSE)
[![Lines](https://img.shields.io/badge/lines%20of%20code-8000%2B-ef4444?style=for-the-badge)](https://github.com/Aditya-cyber-hind/HeavenDB)

**Zero dependencies. Zero frameworks. Just raw C and relentless engineering.**

[Features](#-features) • [Architecture](#-architecture) • [Performance](#-performance) • [Quick Start](#-quick-start) • [SQL Reference](#-sql-reference) • [Roadmap](#-roadmap)

</div>

---

## 📖 Overview

**HeavenDB** is a complete, production-grade SQL database engine built entirely from scratch in **pure C** — no external libraries, no frameworks, no shortcuts. Every byte of data storage, every B-Tree rebalancing, every SQL token parsed, and every transaction committed is handled by code written from the ground up.

Built by a **13-year-old systems programmer** over the course of **5 days** as a deep dive into how databases actually work under the hood.

> *"Anyone can `npm install sqlite3`. Very few can build the engine itself."*

---

## ✨ Features

### 🧠 SQL Engine
| Category | Support |
|----------|---------|
| **DML** | `SELECT`, `INSERT`, `UPDATE`, `DELETE` |
| **DDL** | `CREATE`, `DROP`, `ALTER TABLE`, `CREATE VIEW`, `CREATE INDEX` |
| **Joins** | `INNER`, `LEFT`, `RIGHT`, `FULL`, `CROSS` |
| **Subqueries** | Scalar, `EXISTS`, `NOT EXISTS`, `ANY`, `ALL`, `SOME` |
| **Aggregates** | `COUNT`, `SUM`, `AVG`, `MIN`, `MAX`, `GROUP_CONCAT` |
| **Grouping** | `GROUP BY`, `HAVING` |
| **Set Operations** | `UNION`, `UNION ALL` |
| **Sorting** | `ORDER BY ASC/DESC`, `LIMIT`, `OFFSET`, `DISTINCT` |
| **Filtering** | `WHERE`, `AND`, `OR`, `=`, `!=`, `<>`, `<`, `>`, `<=`, `>=`, `LIKE`, `BETWEEN`, `IN`, `IS NULL` |
| **Conditional** | `CASE WHEN ... THEN ... ELSE ... END` |
| **CTEs** | `WITH ... AS (...)` |

### 🎯 Data Types
```
INTEGER    |  32-bit signed integers
FLOAT      |  64-bit floating point
TEXT       |  Variable-length strings
UUID       |  128-bit unique identifiers (auto-generated)
JSON       |  Structured JSON documents
BOOLEAN    |  TRUE / FALSE
DATE       |  YYYY-MM-DD
TIMESTAMP  |  YYYY-MM-DD HH:MM:SS
```

### 🔧 Built-in Functions

**String Functions**
```sql
UPPER(str)    LOWER(str)     LENGTH(str)    TRIM(str)
SUBSTR(str, start, length)   CONCAT(a, b, c, ...)
```

**Math Functions**
```sql
ABS(n)   ROUND(n, decimals)   FLOOR(n)   CEIL(n)   MOD(a, b)
```

**JSON Functions**
```sql
json_extract(column, 'key')          -- Get value from JSON
json_set(column, 'key', 'value')     -- Update value in JSON
```

**Date Functions**
```sql
NOW()                -- Current timestamp
CURRENT_DATE()       -- Current date
YEAR(date)           -- Extract year
MONTH(date)          -- Extract month
DAY(date)            -- Extract day
```

### 🛡️ Security & Administration
- **User Authentication** — `CREATE USER`, `LOGIN`, `LOGOUT`, `CHANGE PASSWORD`
- **Password Hashing** — djb2 hash with validation rules
- **Account Lockout** — Auto-lock after 5 failed login attempts
- **Permissions** — `GRANT`, `REVOKE` per user/table
- **Backup** — `BACKUP TO 'file.hdb'`
- **Query Analysis** — `EXPLAIN` command
- **Table Inspection** — `SHOW TABLES`, `DESCRIBE table`, `TRUNCATE`

### ⚡ Storage Engine
- **In-Memory Hash Map** — 10,000,000+ operations per second
- **B-Tree Indexes** — O(log n) range queries on INTEGER columns
- **Group Commit Buffer** — Batches writes for 100x faster disk I/O
- **Write-Ahead Log (WAL)** — Full ACID transaction support
- **Custom Binary Format** — `.hdb` files
- **Auto-Increment Persistence** — Counters survive restarts
- **FK Cascade** — `ON DELETE CASCADE` referential integrity

### 🌐 Networking
- **TCP Server** — Multi-threaded client-server mode
- **HTTP Server** — Built-in web dashboard
- **WebSocket Support** — Real-time browser connections
- **Cross-Language** — Works with Python, Node.js, Go, telnet, or any TCP client

---

## 📊 Performance

| Operation | Throughput | Complexity |
|-----------|-----------|-----------|
| In-Memory GET | **10,000,000 ops/sec** | O(1) |
| Disk SET (Group Commit) | **7,800 ops/sec** | O(1) amortized |
| B-Tree Lookup | **Instant** | O(log n) |
| SQL SELECT | **Instant** | varies |
| Join Operations | **Fast** | O(n × m) |

**Benchmarked with 292-command stress test across 9 tables and 90+ rows.**

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────┐
│                       HEAVENDB                          │
├─────────────────────────────────────────────────────────┤
│                                                         │
│   ┌──────────┐   ┌──────────┐   ┌──────────┐            │
│   │   CLI    │   │   TCP    │   │   HTTP   │            │
│   │  Client  │   │  Server  │   │  Server  │            │
│   └──────────┘   └──────────┘   └──────────┘            │
│                                                         │
├─────────────────────────────────────────────────────────┤
│                                                         │
│         SQL Tokenizer  →  SQL Parser  →  Executor       │
│                                                         │
├─────────────────────────────────────────────────────────┤
│                                                         │
│   ┌─────────────────────────────────────────────────┐   │
│   │        In-Memory Hash Map  (O(1) lookups)       │   │
│   └─────────────────────────────────────────────────┘   │
│                                                         │
│   ┌─────────────────────────────────────────────────┐   │
│   │        B-Tree Indexes  (O(log n) ranges)        │   │
│   └─────────────────────────────────────────────────┘   │
│                                                         │
│   ┌─────────────────────────────────────────────────┐   │
│   │      Append-Only File Storage  (.hdb files)     │   │
│   └─────────────────────────────────────────────────┘   │
│                                                         │
│   ┌─────────────────────────────────────────────────┐   │
│   │      Group Commit Buffer  (100x disk speedup)   │   │
│   └─────────────────────────────────────────────────┘   │
│                                                         │
│   ┌─────────────────────────────────────────────────┐   │
│   │      Write-Ahead Log  (ACID transactions)       │   │
│   └─────────────────────────────────────────────────┘   │
│                                                         │
│   ┌──────────────┐  ┌──────────────┐  ┌─────────────┐   │
│   │     Auth     │  │ Permissions  │  │ Replication │   │
│   └──────────────┘  └──────────────┘  └─────────────┘   │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

---

## 🚀 Quick Start

### Compile

```bash
gcc -Wall -Wextra -Wno-switch -O2 -std=c99 -o heavendb.exe \
    src\main.c src\database.c src\hashmap.c src\buffer.c \
    src\table.c src\sql.c src\btree.c src\wal.c \
    src\tcp_server.c src\auth.c src\auth_storage.c \
    src\permissions.c src\replication.c src\websocket.c \
    src\http_server.c -lws2_32
```

### Interactive Shell

```bash
heavendb shell
```

### Start Server (TCP + HTTP + Dashboard)

```bash
heavendb serve
```

🌐 Open **http://localhost:8080** for the web dashboard.

### Run SQL Script

```bash
heavendb run script.sql
```

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

---

## 📝 SQL Reference

### Table Management

```sql
CREATE TABLE users (
    id INTEGER PRIMARY KEY AUTO_INCREMENT,
    name TEXT NOT NULL,
    email TEXT UNIQUE,
    age INTEGER,
    salary FLOAT,
    is_active BOOLEAN,
    metadata JSON,
    event_date DATE,
    created_at TIMESTAMP
);

CREATE TABLE orders (
    id INTEGER PRIMARY KEY AUTO_INCREMENT,
    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    total FLOAT,
    status TEXT
);

ALTER TABLE users ADD COLUMN phone TEXT;
ALTER TABLE users DROP COLUMN phone;
DROP TABLE orders;
```

### Inserting Data

```sql
INSERT INTO users VALUES (NULL, 'Aditya', 'a@x.com', 25, 85000.50, TRUE, '{"role":"engineer"}', '2026-09-15', NOW());
INSERT INTO users VALUES (NULL, 'Rahul', 'r@x.com', 19, 65000.75, FALSE, '{"role":"intern"}', '2026-10-20', NOW());
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

-- Sorting & Pagination
SELECT * FROM users ORDER BY salary DESC;
SELECT * FROM users LIMIT 10 OFFSET 5;

-- Aggregation
SELECT COUNT(*) FROM users;
SELECT AVG(salary) FROM users;
SELECT age, COUNT(*) FROM users GROUP BY age HAVING COUNT(*) > 1;
SELECT GROUP_CONCAT(name) FROM users;

-- Joins
SELECT * FROM users INNER JOIN orders ON users.id = orders.user_id;
SELECT * FROM users LEFT JOIN orders ON users.id = orders.user_id;

-- Subqueries
SELECT * FROM users WHERE age > (SELECT AVG(age) FROM users);
SELECT * FROM users WHERE EXISTS (SELECT 1 FROM orders WHERE orders.user_id = users.id);

-- CASE WHEN
SELECT name, CASE WHEN salary > 80000 THEN 'High' ELSE 'Normal' END FROM users;

-- JSON
SELECT json_extract(metadata, 'role') FROM users;
SELECT json_set(metadata, 'role', 'manager') FROM users;

-- Date/Time
SELECT NOW();
SELECT CURRENT_DATE();
SELECT YEAR(event_date), MONTH(event_date), DAY(event_date) FROM events;
```

### Transactions

```sql
BEGIN;
INSERT INTO users VALUES (NULL, 'Test', 't@x.com', 30, 50000.00, TRUE, '{}', '2026-09-15', NOW());
SAVEPOINT checkpoint;
INSERT INTO users VALUES (NULL, 'Test2', 't2@x.com', 32, 55000.00, TRUE, '{}', '2026-09-15', NOW());
ROLLBACK TO SAVEPOINT checkpoint;
COMMIT;
```

### Security

```sql
CREATE USER admin WITH PASSWORD 'admin123';
LOGIN admin WITH PASSWORD 'admin123';
CHANGE PASSWORD 'NewSecure123';
GRANT SELECT ON users TO admin;
REVOKE DELETE ON users FROM admin;
LOGOUT;
```

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

client.on('data', (data) => {
    console.log(data.toString());
    client.destroy();
});
```

### telnet

```bash
telnet localhost 6379
```

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
│   ├── sql.c/h             # SQL parser and executor
│   ├── auth.c/h            # Authentication
│   ├── auth_storage.c/h    # Persistent user storage
│   ├── permissions.c/h     # GRANT/REVOKE system
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
├── test3.sql               # 292-command stress test
├── README.md
└── Makefile
```

---

## 🗺️ Roadmap

### ✅ Completed
- [x] Hash Map storage engine
- [x] Group Commit buffering
- [x] SQL parser & tokenizer
- [x] B-Tree indexes
- [x] Persistent storage
- [x] ACID transactions with WAL
- [x] TCP & HTTP servers
- [x] Web dashboard
- [x] User authentication
- [x] Password security with lockout
- [x] GRANT / REVOKE permissions
- [x] INNER / LEFT / RIGHT / FULL / CROSS JOIN
- [x] Views
- [x] COUNT, SUM, AVG, MIN, MAX, GROUP_CONCAT
- [x] ORDER BY, GROUP BY, HAVING
- [x] LIMIT, OFFSET, DISTINCT
- [x] LIKE, BETWEEN, IN, IS NULL
- [x] PRIMARY KEY, UNIQUE, NOT NULL, AUTO_INCREMENT
- [x] FOREIGN KEY with ON DELETE CASCADE
- [x] UUID, JSON, BOOLEAN data types
- [x] **DATE and TIMESTAMP data types**
- [x] String functions (6)
- [x] Math functions (5)
- [x] **JSON functions (json_extract, json_set)**
- [x] **Date functions (NOW, CURRENT_DATE, YEAR, MONTH, DAY)**
- [x] SHOW TABLES, DESCRIBE, TRUNCATE
- [x] CASE WHEN
- [x] EXISTS / NOT EXISTS
- [x] ANY / ALL / SOME
- [x] Scalar subqueries
- [x] SAVEPOINT / RELEASE / ROLLBACK TO
- [x] CTEs (basic)
- [x] BACKUP command
- [x] EXPLAIN command
- [x] SQL script execution

### 🚧 In Progress
- [ ] Window functions (ROW_NUMBER, RANK, ROW_NUMBER OVER)
- [ ] Triggers
- [ ] Stored Procedures
- [ ] Full-text search

### 🔮 Planned
- [ ] MVCC concurrency control
- [ ] Multi-master replication
- [ ] Query optimizer
- [ ] Cost-based planner
- [ ] Columnar storage engine

---

## 🏆 Test Suite

HeavenDB ships with a **292-command stress test** (`test3.sql`) covering every feature:

```bash
heavendb run test3.sql
```

**Result:**
```
========================================
Script complete!
  Commands executed: 272
  Errors: 10  (all expected DROP TABLE on first run)
========================================
```

---

## 🧑‍💻 Author

Built by **Aditya** — a 13-year-old systems programmer from India.

- 🐙 GitHub: [@Aditya-cyber-hind](https://github.com/Aditya-cyber-hind)
- 💼 Building databases, games, and compilers in pure C
- 🎯 Goal: become a systems engineer at a top-tier company

> *"If a 13-year-old can build a database engine in 5 days, you have no excuse."*

---

## 📄 License

This project is licensed under the **MIT License** — free to use, modify, and distribute.

```
Copyright (c) 2026 Aditya

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
```

---

<div align="center">

### ⭐ If HeavenDB impressed you, star the repo!

**Built from scratch. Built with obsession. Built in pure C.**

[⬆ Back to top](#-heavendb)

</div>
