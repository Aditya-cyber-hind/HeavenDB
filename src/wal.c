#include "wal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TABLE_NAME 64

// ==================== WAL LIFECYCLE ====================

WAL *wal_create(void) {
    WAL *wal = (WAL*)malloc(sizeof(WAL));
    if (!wal) return NULL;
    wal->head = NULL;
    wal->tail = NULL;
    wal->count = 0;
    wal->in_transaction = 0;
    wal->savepoints = NULL;
    wal->log_fp = NULL;
    return wal;
}

int wal_begin(WAL *wal) {
    if (!wal) return -1;
    if (wal->in_transaction) return -1;
    
    wal->in_transaction = 1;
    
    // Clear any old in-memory entries
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
    
    // Clear savepoints
    Savepoint *sp = wal->savepoints;
    while (sp) {
        Savepoint *next = sp->next;
        free(sp);
        sp = next;
    }
    wal->savepoints = NULL;
    
    // Open WAL file for writing (truncate if exists)
    if (wal->log_fp) {
        fclose(wal->log_fp);
    }
    wal->log_fp = fopen(WAL_FILE, "wb");
    if (!wal->log_fp) return -1;
    
    // Write a BEGIN marker
    uint32_t magic = WAL_MAGIC;
    uint32_t type = WAL_BEGIN;
    fwrite(&magic, sizeof(uint32_t), 1, wal->log_fp);
    fwrite(&type, sizeof(uint32_t), 1, wal->log_fp);
    fflush(wal->log_fp);
    
    return 0;
}

// ==================== REAL-TIME WAL WRITES ====================

int wal_log_insert(WAL *wal, const char *table_name, void **values, int column_count) {
    if (!wal || !wal->in_transaction) return -1;
    if (!wal->log_fp) return -1;
    
    // Write to disk immediately
    uint32_t magic = WAL_MAGIC;
    uint32_t type = WAL_INSERT;
    uint32_t name_len = (uint32_t)strlen(table_name);
    uint32_t col_count = (uint32_t)column_count;
    
    fwrite(&magic, sizeof(uint32_t), 1, wal->log_fp);
    fwrite(&type, sizeof(uint32_t), 1, wal->log_fp);
    fwrite(&name_len, sizeof(uint32_t), 1, wal->log_fp);
    fwrite(&col_count, sizeof(uint32_t), 1, wal->log_fp);
    fwrite(table_name, sizeof(char), name_len, wal->log_fp);
    
    for (int i = 0; i < column_count; i++) {
        char *val_str = (char*)values[i];
        uint32_t val_len = (uint32_t)strlen(val_str);
        fwrite(&val_len, sizeof(uint32_t), 1, wal->log_fp);
        fwrite(val_str, sizeof(char), val_len, wal->log_fp);
    }
    
    fflush(wal->log_fp);
    
    // Also track in memory for in-session rollback
    WALEntry *entry = (WALEntry*)malloc(sizeof(WALEntry));
    if (!entry) return -1;
    
    entry->type = WAL_INSERT;
    entry->table_name = (char*)malloc(name_len + 1);
    strcpy(entry->table_name, table_name);
    
    entry->column_count = column_count;
    entry->values = (void**)malloc(column_count * sizeof(void*));
    for (int i = 0; i < column_count; i++) {
        char *val_str = (char*)values[i];
        entry->values[i] = (void*)malloc(strlen(val_str) + 1);
        strcpy((char*)entry->values[i], val_str);
    }
    
    entry->next = NULL;
    if (wal->tail) wal->tail->next = entry;
    else wal->head = entry;
    wal->tail = entry;
    wal->count++;
    
    return 0;
}

int wal_commit(WAL *wal) {
    if (!wal || !wal->in_transaction) return -1;
    
    wal->in_transaction = 0;
    
    // Write COMMIT marker to disk
    if (wal->log_fp) {
        uint32_t magic = WAL_MAGIC;
        uint32_t type = WAL_COMMIT;
        fwrite(&magic, sizeof(uint32_t), 1, wal->log_fp);
        fwrite(&type, sizeof(uint32_t), 1, wal->log_fp);
        fflush(wal->log_fp);
        fclose(wal->log_fp);
        wal->log_fp = NULL;
    }
    
    // NOTE: We do NOT delete the WAL here. It stays on disk so that if the
    // process crashes between COMMIT and sql_save() completing, we can
    // replay the committed operations on next startup.
    //
    // The WAL is deleted after a successful sql_save() (see sql_commit),
    // or by the recovery path.
    
    return 0;
}

int wal_rollback(WAL *wal) {
    if (!wal || !wal->in_transaction) return -1;
    
    wal->in_transaction = 0;
    
    // Discard the WAL file
    if (wal->log_fp) {
        fclose(wal->log_fp);
        wal->log_fp = NULL;
    }
    remove(WAL_FILE);
    
    // Clear in-memory entries
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
    
    // Clear savepoints
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
    
    if (wal->log_fp) {
        fclose(wal->log_fp);
    }
    
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

// ==================== SAVEPOINTS ====================

int wal_savepoint(WAL *wal, const char *name) {
    if (!wal || !wal->in_transaction || !name) return -1;
    
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
            if (prev) prev->next = NULL;
            else wal->savepoints = NULL;
            
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
    
    Savepoint *sp = wal->savepoints;
    while (sp) {
        if (strcmp(sp->name, name) == 0) break;
        sp = sp->next;
    }
    
    if (!sp) return -1;
    
    int target_count = sp->entry_count;
    
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
    
    Savepoint *to_free = sp->next;
    sp->next = NULL;
    while (to_free) {
        Savepoint *next = to_free->next;
        free(to_free);
        to_free = next;
    }
    
    return 0;
}

// ==================== CRASH RECOVERY ====================

static WALEntry *pending_replay_head = NULL;
static WALEntry *pending_replay_current = NULL;

int wal_recover_check(void) {
    FILE *fp = fopen(WAL_FILE, "rb");
    if (!fp) {
        return 0;  // No WAL — clean shutdown
    }
    
    // First pass: check for COMMIT marker
    int has_commit = 0;
    int insert_count = 0;
    
    while (1) {
        uint32_t magic;
        if (fread(&magic, sizeof(uint32_t), 1, fp) != 1) break;
        if (magic != WAL_MAGIC) break;
        
        uint32_t type;
        if (fread(&type, sizeof(uint32_t), 1, fp) != 1) break;
        
        if (type == WAL_COMMIT) {
            has_commit = 1;
            break;
        }
        if (type == WAL_INSERT) {
            insert_count++;
            uint32_t name_len, col_count;
            if (fread(&name_len, sizeof(uint32_t), 1, fp) != 1) break;
            if (fread(&col_count, sizeof(uint32_t), 1, fp) != 1) break;
            fseek(fp, name_len, SEEK_CUR);
            for (uint32_t i = 0; i < col_count; i++) {
                uint32_t val_len;
                if (fread(&val_len, sizeof(uint32_t), 1, fp) != 1) break;
                fseek(fp, val_len, SEEK_CUR);
            }
        }
    }
    
    if (!has_commit) {
        fclose(fp);
        remove(WAL_FILE);
        if (insert_count > 0) {
            printf("WAL Recovery: Found interrupted transaction (%d inserts) -- DISCARDED\n",
                   insert_count);
        }
        return 0;
    }
    
    // Second pass: parse all INSERT entries into memory
    fseek(fp, 0, SEEK_SET);
    WALEntry *head = NULL;
    WALEntry *tail = NULL;
    
    while (1) {
        uint32_t magic;
        if (fread(&magic, sizeof(uint32_t), 1, fp) != 1) break;
        if (magic != WAL_MAGIC) break;
        
        uint32_t type;
        if (fread(&type, sizeof(uint32_t), 1, fp) != 1) break;
        
        if (type == WAL_COMMIT) break;
        if (type != WAL_INSERT) continue;
        
        uint32_t name_len, col_count;
        if (fread(&name_len, sizeof(uint32_t), 1, fp) != 1) break;
        if (fread(&col_count, sizeof(uint32_t), 1, fp) != 1) break;
        
        WALEntry *entry = (WALEntry*)malloc(sizeof(WALEntry));
        entry->type = WAL_INSERT;
        entry->table_name = (char*)malloc(name_len + 1);
        if (fread(entry->table_name, sizeof(char), name_len, fp) != name_len) {
            free(entry->table_name);
            free(entry);
            break;
        }
        entry->table_name[name_len] = '\0';
        
        entry->column_count = (int)col_count;
        entry->values = (void**)malloc(col_count * sizeof(void*));
        
        for (uint32_t i = 0; i < col_count; i++) {
            uint32_t val_len;
            if (fread(&val_len, sizeof(uint32_t), 1, fp) != 1) break;
            char *val = (char*)malloc(val_len + 1);
            if (fread(val, sizeof(char), val_len, fp) != val_len) {
                free(val);
                break;
            }
            val[val_len] = '\0';
            entry->values[i] = val;
        }
        
        entry->next = NULL;
        if (tail) tail->next = entry;
        else head = entry;
        tail = entry;
    }
    
    fclose(fp);
    
    pending_replay_head = head;
    pending_replay_current = head;
    
    if (insert_count > 0) {
        printf("WAL Recovery: Found committed transaction (%d inserts) -- REPLAYING\n",
               insert_count);
    }
    
    return 1;
}

WALEntry *wal_get_next_pending(void) {
    if (!pending_replay_current) return NULL;
    WALEntry *entry = pending_replay_current;
    pending_replay_current = pending_replay_current->next;
    return entry;
}

void wal_recover_done(void) {
    WALEntry *current = pending_replay_head;
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
    pending_replay_head = NULL;
    pending_replay_current = NULL;
    
    remove(WAL_FILE);
}