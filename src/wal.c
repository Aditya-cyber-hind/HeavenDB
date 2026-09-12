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
    wal->savepoints = NULL;
    return wal;
}

int wal_begin(WAL *wal) {
    if (!wal) return -1;
    if (wal->in_transaction) return -1;
    
    wal->in_transaction = 1;
    
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
    
    Savepoint *sp = wal->savepoints;
    while (sp) {
        Savepoint *next = sp->next;
        free(sp);
        sp = next;
    }
    wal->savepoints = NULL;
    
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
    
    wal->in_transaction = 0;
    
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
        
        uint32_t commit_marker = WAL_COMMIT;
        fwrite(&commit_marker, sizeof(uint32_t), 1, fp);
        fclose(fp);
    }
    
    return 0;
}

int wal_rollback(WAL *wal) {
    if (!wal || !wal->in_transaction) return -1;
    
    wal->in_transaction = 0;
    
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
    
    Savepoint *sp = wal->savepoints;
    while (sp) {
        Savepoint *next = sp->next;
        free(sp);
        sp = next;
    }
    wal->savepoints = NULL;
    
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
    
    Savepoint *sp = wal->savepoints;
    while (sp) {
        Savepoint *next = sp->next;
        free(sp);
        sp = next;
    }
    
    free(wal);
}

// ==================== SAVEPOINT FUNCTIONS ====================

int wal_savepoint(WAL *wal, const char *name) {
    if (!wal || !wal->in_transaction || !name) return -1;
    
    // Check for duplicate savepoint
    Savepoint *sp = wal->savepoints;
    while (sp) {
        if (strcmp(sp->name, name) == 0) return -1;
        sp = sp->next;
    }
    
    Savepoint *new_sp = (Savepoint*)malloc(sizeof(Savepoint));
    if (!new_sp) return -1;
    
    strncpy(new_sp->name, name, sizeof(new_sp->name) - 1);
    new_sp->name[sizeof(new_sp->name) - 1] = '\0';
    new_sp->entry_count = wal->count;
    new_sp->next = NULL;
    
    // Append to TAIL (chronological order)
    if (!wal->savepoints) {
        wal->savepoints = new_sp;
    } else {
        Savepoint *tail = wal->savepoints;
        while (tail->next) tail = tail->next;
        tail->next = new_sp;
    }
    
    return 0;
}

int wal_release_savepoint(WAL *wal, const char *name) {
    if (!wal || !name) return -1;
    
    Savepoint *prev = NULL;
    Savepoint *sp = wal->savepoints;
    
    while (sp) {
        if (strcmp(sp->name, name) == 0) {
            // Cut off this savepoint and all newer ones
            if (prev) {
                prev->next = NULL;
            } else {
                wal->savepoints = NULL;
            }
            
            Savepoint *to_free = sp;
            while (to_free) {
                Savepoint *next = to_free->next;
                free(to_free);
                to_free = next;
            }
            return 0;
        }
        prev = sp;
        sp = sp->next;
    }
    
    return -1;
}

int wal_rollback_to_savepoint(WAL *wal, const char *name) {
    if (!wal || !wal->in_transaction || !name) return -1;
    
    // Find the savepoint
    Savepoint *sp = wal->savepoints;
    while (sp) {
        if (strcmp(sp->name, name) == 0) break;
        sp = sp->next;
    }
    
    if (!sp) return -1;
    
    int target_count = sp->entry_count;
    
    // Truncate WAL entries
    while (wal->count > target_count) {
        WALEntry *current = wal->head;
        WALEntry *prev = NULL;
        
        while (current && current->next) {
            prev = current;
            current = current->next;
        }
        
        if (current) {
            free(current->table_name);
            if (current->values) {
                for (int i = 0; i < current->column_count; i++) {
                    free(current->values[i]);
                }
                free(current->values);
            }
            free(current);
            
            if (prev) {
                prev->next = NULL;
                wal->tail = prev;
            } else {
                wal->head = NULL;
                wal->tail = NULL;
            }
            wal->count--;
        }
    }
    
    // Release savepoints newer than this one
    Savepoint *to_free = sp->next;
    sp->next = NULL;
    while (to_free) {
        Savepoint *next = to_free->next;
        free(to_free);
        to_free = next;
    }
    
    return 0;
}