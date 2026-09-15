#include "table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Table *table_create(const char *name) {
    Table *table = (Table*)malloc(sizeof(Table));
    if (!table) return NULL;
    
    memset(table, 0, sizeof(Table));
    
    strncpy(table->name, name, MAX_TABLE_NAME - 1);
    table->name[MAX_TABLE_NAME - 1] = '\0';
    
    for (int i = 0; i < MAX_COLUMNS; i++) {
        table->columns[i].index = NULL;
        table->columns[i].is_primary_key = 0;
        table->columns[i].is_unique = 0;
        table->columns[i].is_not_null = 0;
        table->columns[i].is_auto_increment = 0;
        table->columns[i].next_auto_value = 1;
    }
    
    return table;
}

int table_add_column(Table *table, const char *name, ColumnType type) {
    if (!table || !name) return -1;
    if (table->column_count >= MAX_COLUMNS) return -1;
    
    // Reject duplicate column names
    if (table_get_column_index(table, name) != -1) {
        printf("ERROR: Column '%s' already exists\n", name);
        return -1;
    }
    
    Column *col = &table->columns[table->column_count];
    strncpy(col->name, name, 63);
    col->name[63] = '\0';
    col->type = type;
    col->is_primary_key = 0;
    col->is_unique = 0;
    col->is_not_null = 0;
    col->is_auto_increment = 0;
    col->next_auto_value = 1;
    col->index = NULL;   // <-- no B-Tree automatically
    
    table->column_count++;
    return 0;
}

// Creates a B-Tree index on the given column and populates it from
// existing rows. Used for PRIMARY KEY, UNIQUE, and CREATE INDEX.
// Returns 0 on success, -1 on failure.
int table_create_index(Table *table, int col_idx) {
    if (!table) return -1;
    if (col_idx < 0 || col_idx >= table->column_count) return -1;
    
    Column *col = &table->columns[col_idx];
    
    // Only INTEGER columns can be indexed by the current B-Tree
    if (col->type != TYPE_INTEGER && col->type != TYPE_BOOLEAN) {
        return 0;   // silently no-op for non-integer columns
    }
    
    // Already indexed
    if (col->index != NULL) {
        return 0;
    }
    
    col->index = btree_create();
    if (!col->index) return -1;
    
    // Populate from existing rows. If a duplicate value is found,
    // the B-Tree insert fails and we roll back the index.
    for (size_t r = 0; r < table->row_count; r++) {
        void **row = (void**)table->rows[r];
        int key = *(int*)row[col_idx];
        
        if (btree_insert(col->index, key, row) != 0) {
            // Duplicate found. Since we're building a UNIQUE index,
            // this is a real error. Destroy and report.
            printf("ERROR: Cannot create unique index on '%s' — duplicate value %d\n",
                   col->name, key);
            btree_destroy(col->index);
            col->index = NULL;
            return -1;
        }
    }
    
    return 0;
}

BTree *table_get_index(Table *table, int column_index) {
    if (!table || column_index < 0 || column_index >= table->column_count) return NULL;
    return table->columns[column_index].index;
}

int table_insert(Table *table, void **values) {
    if (!table || !values) return -1;
    
    if (table->row_count >= table->row_capacity) {
        size_t new_capacity = table->row_capacity == 0 ? 16 : table->row_capacity * 2;
        void **new_rows = (void**)realloc(table->rows, new_capacity * sizeof(void*));
        if (!new_rows) return -1;
        table->rows = new_rows;
        table->row_capacity = new_capacity;
    }
    
    // Allocate and deep-copy the row
    void **row = (void**)malloc(table->column_count * sizeof(void*));
    if (!row) return -1;
    
    for (int i = 0; i < table->column_count; i++) {
        switch (table->columns[i].type) {
            case TYPE_INTEGER:
            case TYPE_BOOLEAN: {
                int *val = (int*)malloc(sizeof(int));
                if (!val) {
                    for (int j = 0; j < i; j++) free(row[j]);
                    free(row);
                    return -1;
                }
                *val = *(int*)values[i];
                row[i] = val;
                break;
            }
            case TYPE_TEXT:
            case TYPE_UUID:
            case TYPE_JSON:
            case TYPE_DATE:
            case TYPE_TIMESTAMP: {
                char *str = (char*)values[i];
                char *copy = (char*)malloc(strlen(str) + 1);
                if (!copy) {
                    for (int j = 0; j < i; j++) free(row[j]);
                    free(row);
                    return -1;
                }
                strcpy(copy, str);
                row[i] = copy;
                break;
            }
            case TYPE_FLOAT: {
                double *val = (double*)malloc(sizeof(double));
                if (!val) {
                    for (int j = 0; j < i; j++) free(row[j]);
                    free(row);
                    return -1;
                }
                *val = *(double*)values[i];
                row[i] = val;
                break;
            }
        }
    }
    
    // Check NOT NULL constraints
    for (int i = 0; i < table->column_count; i++) {
        if (table->columns[i].is_not_null) {
            if (table->columns[i].type == TYPE_TEXT ||
                table->columns[i].type == TYPE_UUID ||
                table->columns[i].type == TYPE_JSON ||
                table->columns[i].type == TYPE_DATE ||
                table->columns[i].type == TYPE_TIMESTAMP) {
                char *str = (char*)row[i];
                if (str && strlen(str) == 0) {
                    printf("ERROR: Column '%s' cannot be NULL\n", table->columns[i].name);
                    for (int j = 0; j < table->column_count; j++) free(row[j]);
                    free(row);
                    return -1;
                }
            }
        }
    }
    
    // Check UNIQUE constraints (excluding indexed columns, handled below)
    for (int i = 0; i < table->column_count; i++) {
        if (table->columns[i].is_unique && table->columns[i].index == NULL) {
            for (size_t r = 0; r < table->row_count; r++) {
                void **existing = (void**)table->rows[r];
                if (table->columns[i].type == TYPE_INTEGER ||
                    table->columns[i].type == TYPE_BOOLEAN) {
                    if (*(int*)existing[i] == *(int*)row[i]) {
                        printf("ERROR: Duplicate value for UNIQUE column '%s'\n",
                               table->columns[i].name);
                        for (int j = 0; j < table->column_count; j++) free(row[j]);
                        free(row);
                        return -1;
                    }
                } else {
                    if (strcmp((char*)existing[i], (char*)row[i]) == 0) {
                        printf("ERROR: Duplicate value for UNIQUE column '%s'\n",
                               table->columns[i].name);
                        for (int j = 0; j < table->column_count; j++) free(row[j]);
                        free(row);
                        return -1;
                    }
                }
            }
        }
    }
    
    // Pass 1: check all indexed columns for duplicate keys BEFORE modifying anything
    for (int i = 0; i < table->column_count; i++) {
        if (table->columns[i].type == TYPE_INTEGER && table->columns[i].index) {
            int key = *(int*)row[i];
            if (btree_search(table->columns[i].index, key) != NULL) {
                printf("ERROR: Duplicate value %d in indexed column '%s'\n",
                       key, table->columns[i].name);
                for (int j = 0; j < table->column_count; j++) free(row[j]);
                free(row);
                return -1;
            }
        }
    }
    
    // Update auto-increment counters
    for (int i = 0; i < table->column_count; i++) {
        if (table->columns[i].is_auto_increment && table->columns[i].type == TYPE_INTEGER) {
            int val = *(int*)row[i];
            if (val >= table->columns[i].next_auto_value) {
                table->columns[i].next_auto_value = val + 1;
            }
        }
    }
    
    // Pass 2: insert into the table
    table->rows[table->row_count] = row;
    table->row_count++;
    
    // Pass 3: insert into B-Trees. Cannot fail now, since Pass 1 checked.
    for (int i = 0; i < table->column_count; i++) {
        if (table->columns[i].type == TYPE_INTEGER && table->columns[i].index) {
            int key = *(int*)row[i];
            btree_insert(table->columns[i].index, key, row);
        }
    }
    
    return 0;
}

int table_get_column_index(Table *table, const char *name) {
    if (!table || !name) return -1;
    
    for (int i = 0; i < table->column_count; i++) {
        if (strcmp(table->columns[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

void table_destroy(Table *table) {
    if (!table) return;
    
    for (size_t i = 0; i < table->row_count; i++) {
        void **row = (void**)table->rows[i];
        if (!row) continue;
        for (int j = 0; j < table->column_count; j++) {
            free(row[j]);
        }
        free(row);
    }
    
    for (int i = 0; i < table->column_count; i++) {
        if (table->columns[i].index) {
            btree_destroy(table->columns[i].index);
        }
    }
    
    free(table->rows);
    free(table);
}