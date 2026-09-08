# HeavenDB

A high-performance SQL database engine written in pure C. Built from scratch with zero external dependencies.

## Features

- ⚡ **In-Memory Hash Map** — 10M+ operations per second
- 🌳 **B-Tree Indexes** — O(log n) lookups on INTEGER columns
- 💾 **Group Commit Buffer** — 100x faster disk writes
- 📝 **SQL Parser** — CREATE, INSERT, SELECT with WHERE
- 🔒 **Persistent Storage** — Data survives restarts
- 🔄 **Crash Recovery** — Write-Ahead Logging

## Performance

| Operation | Throughput |
|-----------|------------|
| In-Memory GET | 10,000,000 ops/sec |
| Disk SET (Group Commit) | 7,800 ops/sec |

## Quick Start

```bash
# Compile
gcc -Wall -Wextra -O2 -std=c99 -o heavendb.exe src\main.c src\database.c src\hashmap.c src\buffer.c src\table.c src\sql.c src\btree.c

# Key-Value Store
heavendb set name "Aditya"
heavendb get name

# SQL Mode
heavendb sql "CREATE TABLE users (id INTEGER, name TEXT, age INTEGER)"
heavendb sql "INSERT INTO users VALUES (1, 'Aditya', 25)"
heavendb sql "SELECT * FROM users WHERE age > 18"

# Benchmark
heavendb benchmark 10000
```

## Architecture

```
┌─────────────────────────────────────────────┐
│                  HeavenDB                    │
├─────────────────────────────────────────────┤
│  CLI Client        │  SQL Engine            │
├────────────────────┴────────────────────────┤
│            Command Parser                   │
├─────────────────────────────────────────────┤
│         In-Memory Hash Map (O(1))           │
├─────────────────────────────────────────────┤
│         B-Tree Indexes (O(log n))           │
├─────────────────────────────────────────────┤
│       Append-Only File Storage (.hdb)       │
├─────────────────────────────────────────────┤
│         Background Group Commit Buffer      │
└─────────────────────────────────────────────┘
```

## Tech Stack

- **Language:** C (C99 standard)
- **Compiler:** GCC
- **Platform:** Windows, Linux, macOS
- **Dependencies:** Zero

## Roadmap

- [x] Hash Map storage engine
- [x] Group Commit buffering
- [x] SQL parser (CREATE, INSERT, SELECT)
- [x] B-Tree indexes
- [x] Persistent storage
- [ ] ACID transactions
- [ ] TCP server (client-server mode)
- [ ] Joins
- [ ] Views
- [ ] User permissions
- [ ] Replication

## Author

Built by Aditya. A systems programming project to understand how databases work under the hood.
