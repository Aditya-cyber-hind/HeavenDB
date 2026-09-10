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
    TYPE_FLOAT,
    TYPE_UUID,
    TYPE_JSON
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

Table *table_create(const char *name);
int table_add_column(Table *table, const char *name, ColumnType type);
int table_insert(Table *table, void **values);
void table_destroy(Table *table);
int table_get_column_index(Table *table, const char *name);
BTree *table_get_index(Table *table, int column_index);

#endif // HEAVENDB_TABLE_H