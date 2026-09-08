#ifndef HEAVENDB_DATABASE_H
#define HEAVENDB_DATABASE_H

#include "hashmap.h"
#include "buffer.h"
#include <stddef.h>

#define HDB_FILE_EXTENSION ".hdb"
#define HDB_DEFAULT_FILE "heaven.hdb"

// Database handle
typedef struct {
    char *filename;
    HashMap *memory;
    WriteBuffer *write_buffer;
    int is_dirty;
} Database;

// Opens or creates a database. Returns NULL on error.
Database *db_open(const char *filename);

// Closes the database and frees all memory.
void db_close(Database *db);

// Sets a key-value pair. Returns 0 on success, -1 on error.
int db_set(Database *db, const char *key, const char *value, size_t value_len);

// Gets a value by key. Returns NULL if not found. Sets value_len if provided.
char *db_get(Database *db, const char *key, size_t *value_len);

// Deletes a key. Returns 0 on success, -1 if key not found.
int db_delete(Database *db, const char *key);

// Forces a flush of all buffered writes to disk. Returns 0 on success.
int db_flush(Database *db);

// Loads all data from disk into memory. Returns 0 on success.
int db_load(Database *db);

// Returns the number of keys in the database.
size_t db_size(const Database *db);

#endif // HEAVENDB_DATABASE_H