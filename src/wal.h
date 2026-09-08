#ifndef HEAVENDB_WAL_H
#define HEAVENDB_WAL_H

#include <stdint.h>
#include <stddef.h>

#define WAL_FILE "heaven.wal"
#define WAL_MAGIC 0x57414C31 // "WAL1"

// WAL record types
#define WAL_BEGIN 1
#define WAL_INSERT 2
#define WAL_COMMIT 3
#define WAL_ROLLBACK 4

// A single WAL entry (stored as a linked list in memory)
typedef struct WALEntry {
    uint32_t type;
    char *table_name;
    void **values;
    int column_count;
    struct WALEntry *next;
} WALEntry;

// The WAL structure
typedef struct {
    WALEntry *head;
    WALEntry *tail;
    int count;
    int in_transaction;
} WAL;

// Creates a new WAL
WAL *wal_create(void);

// Begins a transaction
int wal_begin(WAL *wal);

// Adds an insert operation to the WAL
int wal_log_insert(WAL *wal, const char *table_name, void **values, int column_count);

// Commits the transaction (applies to DB)
int wal_commit(WAL *wal);

// Rolls back the transaction (discards all changes)
int wal_rollback(WAL *wal);

// Frees the WAL
void wal_destroy(WAL *wal);

#endif // HEAVENDB_WAL_H