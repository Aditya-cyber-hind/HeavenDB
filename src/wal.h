#ifndef HEAVENDB_WAL_H
#define HEAVENDB_WAL_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#define WAL_FILE "heaven.wal"
#define WAL_MAGIC 0x57414C31

#define WAL_BEGIN 1
#define WAL_INSERT 2
#define WAL_COMMIT 3
#define WAL_ROLLBACK 4

typedef struct WALEntry {
    uint32_t type;
    char *table_name;
    void **values;
    int column_count;
    struct WALEntry *next;
} WALEntry;

typedef struct Savepoint {
    char name[64];
    int entry_count;
    struct Savepoint *next;
} Savepoint;

typedef struct {
    WALEntry *head;
    WALEntry *tail;
    int count;
    int in_transaction;
    Savepoint *savepoints;
    FILE *log_fp;  // Persistent file handle for real-time WAL writes
} WAL;

WAL *wal_create(void);
int wal_begin(WAL *wal);
int wal_log_insert(WAL *wal, const char *table_name, void **values, int column_count);
int wal_commit(WAL *wal);
int wal_rollback(WAL *wal);
void wal_destroy(WAL *wal);

int wal_savepoint(WAL *wal, const char *name);
int wal_release_savepoint(WAL *wal, const char *name);
int wal_rollback_to_savepoint(WAL *wal, const char *name);

// Crash recovery: replays committed transactions from disk
// Returns number of operations replayed, -1 on error
int wal_recover(void);

#endif // HEAVENDB_WAL_H