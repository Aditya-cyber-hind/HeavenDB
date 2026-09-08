#include "wal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

WAL *wal_create(void) {
    WAL *wal = (WAL*)malloc(sizeof(WAL));
    if (!wal) return NULL;
    wal->head = NULL;
    wal->tail = NULL;
    wal->count = 0;
    wal->in_transaction = 0;
    return wal;
}

int wal_begin(WAL *wal) {
    if (!wal) return -1;
    if (wal->in_transaction) return -1; // Already in transaction
    
    wal->in_transaction = 1;
    
    // Clear any old entries
    WALEntry *current = wal->head;
    while (current) {
        WALEntry *next = current->next;
        free(current->table_name);
        if (current->values) {
            for (int i = 0; i < current->column_count; i++) {
                free(current->values[i]);
            }
            free(current->values);
        }
        free(current);
        current = next;
    }
    
    wal->head = NULL;
    wal->tail = NULL;
    wal->count = 0;
    
    return 0;
}

int wal_log_insert(WAL *wal, const char *table_name, void **values, int column_count) {
    if (!wal || !wal->in_transaction) return -1;
    
    WALEntry *entry = (WALEntry*)malloc(sizeof(WALEntry));
    if (!entry) return -1;
    
    entry->type = WAL_INSERT;
    entry->table_name = (char*)malloc(strlen(table_name) + 1);
    strcpy(entry->table_name, table_name);
    
    entry->column_count = column_count;
    entry->values = (void**)malloc(column_count * sizeof(void*));
    
    for (int i = 0; i < column_count; i++) {
        // We assume values are strings for simplicity
        // In a real implementation, we'd need type info
        char *str = (char*)values[i];
        entry->values[i] = (void*)malloc(strlen(str) + 1);
        strcpy((char*)entry->values[i], str);
    }
    
    entry->next = NULL;
    
    if (wal->tail) {
        wal->tail->next = entry;
    } else {
        wal->head = entry;
    }
    wal->tail = entry;
    wal->count++;
    
    return 0;
}

int wal_commit(WAL *wal) {
    if (!wal || !wal->in_transaction) return -1;
    
    // In a real implementation, we would:
    // 1. Write all entries to the WAL file
    // 2. fsync to ensure durability
    // 3. Apply changes to the main database
    // 4. Write a COMMIT marker
    // 5. fsync again
    
    // For now, we just mark as committed
    wal->in_transaction = 0;
    
    // Write to WAL file for crash recovery
    FILE *fp = fopen(WAL_FILE, "ab");
    if (fp) {
        uint32_t magic = WAL_MAGIC;
        fwrite(&magic, sizeof(uint32_t), 1, fp);
        
        WALEntry *current = wal->head;
        while (current) {
            uint32_t type = current->type;
            fwrite(&type, sizeof(uint32_t), 1, fp);
            
            uint32_t name_len = (uint32_t)strlen(current->table_name);
            fwrite(&name_len, sizeof(uint32_t), 1, fp);
            fwrite(current->table_name, sizeof(char), name_len, fp);
            
            current = current->next;
        }
        
        // Write commit marker
        uint32_t commit_marker = WAL_COMMIT;
        fwrite(&commit_marker, sizeof(uint32_t), 1, fp);
        fclose(fp);
    }
    
    return 0;
}

int wal_rollback(WAL *wal) {
    if (!wal || !wal->in_transaction) return -1;
    
    wal->in_transaction = 0;
    
    // Discard all entries
    WALEntry *current = wal->head;
    while (current) {
        WALEntry *next = current->next;
        free(current->table_name);
        if (current->values) {
            for (int i = 0; i < current->column_count; i++) {
                free(current->values[i]);
            }
            free(current->values);
        }
        free(current);
        current = next;
    }
    
    wal->head = NULL;
    wal->tail = NULL;
    wal->count = 0;
    
    return 0;
}

void wal_destroy(WAL *wal) {
    if (!wal) return;
    
    WALEntry *current = wal->head;
    while (current) {
        WALEntry *next = current->next;
        free(current->table_name);
        if (current->values) {
            for (int i = 0; i < current->column_count; i++) {
                free(current->values[i]);
            }
            free(current->values);
        }
        free(current);
        current = next;
    }
    
    free(wal);
}