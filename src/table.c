#include "table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Table *table_create(const char *name) {
    Table *table = (Table*)malloc(sizeof(Table));
    if (!table) return NULL;
    
    strncpy(table->name, name, MAX_TABLE_NAME - 1);
    table->name[MAX_TABLE_NAME - 1] = '\0';
    table->column_count = 0;
    table->rows = NULL;
    table->row_count = 0;
    table->row_capacity = 0;
    
    // Initialize all column indexes to NULL
    for (int i = 0; i < MAX_COLUMNS; i++) {
        table->columns[i].index = NULL;
    }
    
    return table;
}

int table_add_column(Table *table, const char *name, ColumnType type) {
    if (!table || table->column_count >= MAX_COLUMNS) return -1;
    
    Column *col = &table->columns[table->column_count];
    strncpy(col->name, name, 63);
    col->name[63] = '\0';
    col->type = type;
    
    // Create B-Tree index for INTEGER columns
    if (type == TYPE_INTEGER) {
        col->index = btree_create();
        if (!col->index) return -1;
    } else {
        col->index = NULL;
    }
    
    table->column_count++;
    return 0;
}

BTree *table_get_index(Table *table, int column_index) {
    if (!table || column_index < 0 || column_index >= table->column_count) return NULL;
    return table->columns[column_index].index;
}

int table_insert(Table *table, void **values) {
    if (!table || !values) return -1;
    
    // Allocate memory for a new row
    if (table->row_count >= table->row_capacity) {
        size_t new_capacity = table->row_capacity == 0 ? 16 : table->row_capacity * 2;
        void **new_rows = (void**)realloc(table->rows, new_capacity * sizeof(void*));
        if (!new_rows) return -1;
        table->rows = new_rows;
        table->row_capacity = new_capacity;
    }
    
    // Allocate and copy the row data
    void **row = (void**)malloc(table->column_count * sizeof(void*));
    if (!row) return -1;
    
    for (int i = 0; i < table->column_count; i++) {
        switch (table->columns[i].type) {
            case TYPE_INTEGER: {
                int *val = (int*)malloc(sizeof(int));
                *val = *(int*)values[i];
                row[i] = val;
                break;
            }
            case TYPE_TEXT: {
                char *str = (char*)values[i];
                char *copy = (char*)malloc(strlen(str) + 1);
                strcpy(copy, str);
                row[i] = copy;
                break;
            }
            case TYPE_FLOAT: {
                double *val = (double*)malloc(sizeof(double));
                *val = *(double*)values[i];
                row[i] = val;
                break;
            }
        }
    }
    
    // Store the row
    table->rows[table->row_count] = row;
    table->row_count++;
    
    // Add to B-Tree indexes for INTEGER columns
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
        for (int j = 0; j < table->column_count; j++) {
            free(row[j]);
        }
        free(row);
    }
    
    // Free B-Tree indexes
    for (int i = 0; i < table->column_count; i++) {
        if (table->columns[i].index) {
            btree_destroy(table->columns[i].index);
        }
    }
    
    free(table->rows);
    free(table);
}