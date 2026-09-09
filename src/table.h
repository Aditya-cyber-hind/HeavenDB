#ifndef HEAVENDB_TABLE_H
#define HEAVENDB_TABLE_H

#include <stddef.h>
#include "btree.h"

#define MAX_COLUMNS 32
#define MAX_TABLE_NAME 64
#define MAX_ROWS 100000

typedef enum {
    TYPE_INTEGER,
    TYPE_TEXT,
    TYPE_FLOAT
} ColumnType;

typedef struct {
    char name[64];
    ColumnType type;
    BTree *index;
    int is_primary_key;
    int is_unique;
    int is_not_null;
    int is_auto_increment;
    int next_auto_value;
} Column;

typedef struct {
    char name[MAX_TABLE_NAME];
    Column columns[MAX_COLUMNS];
    int column_count;
    void **rows;
    size_t row_count;
    size_t row_capacity;
} Table;

// Creates a new table
Table *table_create(const char *name);

// Adds a column to a table
int table_add_column(Table *table, const char *name, ColumnType type);

// Inserts a row into the table
int table_insert(Table *table, void **values);

// Frees a table
void table_destroy(Table *table);

// Gets the column index by name
int table_get_column_index(Table *table, const char *name);

// Gets a column's B-Tree index (creates if needed)
BTree *table_get_index(Table *table, int column_index);

#endif // HEAVENDB_TABLE_H