# HeavenDB

A high-performance SQL database engine written in pure C. Built from scratch with zero external dependencies.

## Features

- ⚡ **In-Memory Hash Map** — 10M+ operations per second
- 🌳 **B-Tree Indexes** — O(log n) lookups on INTEGER columns
- 💾 **Group Commit Buffer** — 100x faster disk writes
- 📝 **SQL Parser** — CREATE, INSERT, SELECT, UPDATE, DELETE with WHERE
- 🔒 **Persistent Storage** — Data survives restarts
- 🔄 **ACID Transactions** — BEGIN, COMMIT, ROLLBACK
- 🌐 **TCP Server** — Client-server mode with multi-threaded connections
- 👤 **User Authentication** — CREATE USER, LOGIN, LOGOUT
- 💻 **Interactive Shell** — Type SQL commands in real-time
- 🔐 **Password Hashing** — djb2 hash for password storage

## Performance

| Operation | Throughput |
|-----------|------------|
| In-Memory GET | 10,000,000 ops/sec |
| Disk SET (Group Commit) | 7,800 ops/sec |

## Quick Start

```bash
# Compile
gcc -Wall -Wextra -O2 -std=c99 -o heavendb.exe src\main.c src\database.c src\hashmap.c src\buffer.c src\table.c src\sql.c src\btree.c src\wal.c src\tcp_server.c src\auth.c -lws2_32

# Key-Value Store
heavendb set name "Aditya"
heavendb get name

# SQL Mode
heavendb sql "CREATE TABLE users (id INTEGER, name TEXT, age INTEGER)"
heavendb sql "INSERT INTO users VALUES (1, 'Aditya', 25)"
heavendb sql "SELECT * FROM users WHERE age > 18"

# Interactive Shell
heavendb shell

# TCP Server
heavendb serve
```

## SQL Commands

```sql
-- Authentication (default: admin / admin123)
LOGIN admin WITH PASSWORD 'admin123';
CREATE USER aditya WITH PASSWORD 'secret123';
LOGOUT;

-- Table Operations
CREATE TABLE users (id INTEGER, name TEXT, age INTEGER);
INSERT INTO users VALUES (1, 'Aditya', 25);
SELECT * FROM users;
SELECT * FROM users WHERE age > 18;
UPDATE users SET age = 26 WHERE id = 1;
DELETE FROM users WHERE id = 2;

-- Transactions
BEGIN;
INSERT INTO users VALUES (3, 'NewUser', 30);
COMMIT;
-- or ROLLBACK;

-- Key-Value Store
heavendb set name "Aditya"
heavendb get name
heavendb delete name
heavendb size
heavendb benchmark 10000
```

## Architecture

```
┌─────────────────────────────────────────────┐
│                  HeavenDB                    │
├─────────────────────────────────────────────┤
│  CLI Client  │  TCP Server  │  SQL Engine  │
├──────────────┴──────────────┴───────────────┤
│            Command Parser                   │
├─────────────────────────────────────────────┤
│         In-Memory Hash Map (O(1))           │
├─────────────────────────────────────────────┤
│         B-Tree Indexes (O(log n))           │
├─────────────────────────────────────────────┤
│       Append-Only File Storage (.hdb)       │
├─────────────────────────────────────────────┤
│         Group Commit Buffer                 │
├─────────────────────────────────────────────┤
│         Write-Ahead Log (WAL)               │
├─────────────────────────────────────────────┤
│         User Authentication                 │
└─────────────────────────────────────────────┘
```

## Tech Stack

- **Language:** C (C99 standard)
- **Compiler:** GCC
- **Platform:** Windows, Linux, macOS
- **Dependencies:** Zero
- **Client Libraries:** Node.js, Python, or any TCP client

## Roadmap

- [x] Hash Map storage engine
- [x] Group Commit buffering
- [x] SQL parser (CREATE, INSERT, SELECT)
- [x] UPDATE and DELETE commands
- [x] B-Tree indexes
- [x] Persistent storage
- [x] ACID transactions (BEGIN, COMMIT, ROLLBACK)
- [x] TCP server (client-server mode)
- [x] User authentication (CREATE USER, LOGIN, LOGOUT)
- [ ] Joins
- [ ] Views
- [ ] GRANT / REVOKE permissions
- [ ] Replication

## Author

Built by Aditya. A systems programming project to understand how databases work under the hood.

**GitHub:** [Aditya-cyber-hind](https://github.com/Aditya-cyber-hind)
