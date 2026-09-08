#include "database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define OP_SET 1
#define OP_DELETE 2
#define BUFFER_SIZE 100 // Flush after 100 pending writes

Database *db_open(const char *filename) {
    Database *db = (Database*)malloc(sizeof(Database));
    if (!db) return NULL;
    
    db->filename = (char*)malloc(strlen(filename) + 1);
    if (!db->filename) {
        free(db);
        return NULL;
    }
    strcpy(db->filename, filename);
    
    db->memory = hashmap_create();
    if (!db->memory) {
        free(db->filename);
        free(db);
        return NULL;
    }
    
    db->write_buffer = buffer_create(BUFFER_SIZE);
    if (!db->write_buffer) {
        free(db->filename);
        hashmap_destroy(db->memory);
        free(db);
        return NULL;
    }
    
    db->is_dirty = 0;
    
    // Ensure file exists
    FILE *fp = fopen(filename, "ab+");
    if (!fp) {
        buffer_destroy(db->write_buffer);
        free(db->filename);
        hashmap_destroy(db->memory);
        free(db);
        return NULL;
    }
    fclose(fp);
    
    // Load existing data
    if (db_load(db) != 0) {
        db_close(db);
        return NULL;
    }
    
    return db;
}

void db_close(Database *db) {
    if (!db) return;
    
    // Flush any remaining writes
    buffer_flush(db->write_buffer, db->filename);
    
    if (db->filename) free(db->filename);
    if (db->memory) hashmap_destroy(db->memory);
    if (db->write_buffer) buffer_destroy(db->write_buffer);
    free(db);
}

int db_set(Database *db, const char *key, const char *value, size_t value_len) {
    if (!db || !key || !value) return -1;
    
    // Update memory immediately (fast path)
    if (hashmap_set(db->memory, key, value, value_len) != 0) return -1;
    
    // Buffer the disk write
    if (buffer_add(db->write_buffer, OP_SET, key, value, value_len) != 0) return -1;
    
    // Auto-flush when buffer is full
    if (db->write_buffer->count >= db->write_buffer->max_entries) {
        if (buffer_flush(db->write_buffer, db->filename) != 0) return -1;
    }
    
    db->is_dirty = 1;
    return 0;
}

char *db_get(Database *db, const char *key, size_t *value_len) {
    if (!db || !key) return NULL;
    return hashmap_get(db->memory, key, value_len);
}

int db_delete(Database *db, const char *key) {
    if (!db || !key) return -1;
    
    if (!hashmap_get(db->memory, key, NULL)) return -1;
    
    if (hashmap_delete(db->memory, key) != 0) return -1;
    
    // Buffer the tombstone write
    if (buffer_add(db->write_buffer, OP_DELETE, key, "", 0) != 0) return -1;
    
    if (db->write_buffer->count >= db->write_buffer->max_entries) {
        if (buffer_flush(db->write_buffer, db->filename) != 0) return -1;
    }
    
    db->is_dirty = 1;
    return 0;
}

int db_load(Database *db) {
    if (!db) return -1;
    
    FILE *fp = fopen(db->filename, "rb");
    if (!fp) return -1;
    
    uint32_t magic, op_type, key_len, val_len;
    char *key, *value;
    
    while (1) {
        if (fread(&magic, sizeof(uint32_t), 1, fp) != 1) break;
        if (fread(&op_type, sizeof(uint32_t), 1, fp) != 1) break;
        if (fread(&key_len, sizeof(uint32_t), 1, fp) != 1) break;
        if (fread(&val_len, sizeof(uint32_t), 1, fp) != 1) break;
        
        if (magic != 0x48444231) break;
        
        key = (char*)malloc(key_len + 1);
        if (!key) break;
        if (fread(key, sizeof(char), key_len, fp) != key_len) {
            free(key);
            break;
        }
        key[key_len] = '\0';
        
        value = (char*)malloc(val_len + 1);
        if (!value) {
            free(key);
            break;
        }
        if (fread(value, sizeof(char), val_len, fp) != val_len) {
            free(key);
            free(value);
            break;
        }
        value[val_len] = '\0';
        
        if (op_type == OP_SET) {
            hashmap_set(db->memory, key, value, val_len);
        } else if (op_type == OP_DELETE) {
            hashmap_delete(db->memory, key);
        }
        
        free(key);
        free(value);
    }
    
    fclose(fp);
    return 0;
}

int db_flush(Database *db) {
    if (!db) return -1;
    return buffer_flush(db->write_buffer, db->filename);
}

size_t db_size(const Database *db) {
    return db ? hashmap_size(db->memory) : 0;
}