#include "buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RECORD_MAGIC 0x48444231 // "HDB1"

static int write_record_to_file(FILE *fp, uint32_t op_type, const char *key, const char *value, size_t value_len) {
    uint32_t magic = RECORD_MAGIC;
    uint32_t key_len = (uint32_t)strlen(key);
    uint32_t val_len = (uint32_t)value_len;
    
    if (fwrite(&magic, sizeof(uint32_t), 1, fp) != 1) return -1;
    if (fwrite(&op_type, sizeof(uint32_t), 1, fp) != 1) return -1;
    if (fwrite(&key_len, sizeof(uint32_t), 1, fp) != 1) return -1;
    if (fwrite(&val_len, sizeof(uint32_t), 1, fp) != 1) return -1;
    
    if (key_len > 0 && fwrite(key, sizeof(char), key_len, fp) != key_len) return -1;
    if (val_len > 0 && fwrite(value, sizeof(char), val_len, fp) != val_len) return -1;
    
    return 0;
}

WriteBuffer *buffer_create(int max_entries) {
    WriteBuffer *buf = (WriteBuffer*)malloc(sizeof(WriteBuffer));
    if (!buf) return NULL;
    
    buf->head = NULL;
    buf->tail = NULL;
    buf->count = 0;
    buf->max_entries = max_entries;
    return buf;
}

int buffer_add(WriteBuffer *buf, uint32_t op_type, const char *key, const char *value, size_t value_len) {
    if (!buf || !key) return -1;
    
    WriteEntry *entry = (WriteEntry*)malloc(sizeof(WriteEntry));
    if (!entry) return -1;
    
    entry->op_type = op_type;
    entry->key = (char*)malloc(strlen(key) + 1);
    strcpy(entry->key, key);
    
    if (value && value_len > 0) {
        entry->value = (char*)malloc(value_len + 1);
        memcpy(entry->value, value, value_len);
        entry->value[value_len] = '\0';
    } else {
        entry->value = NULL;
    }
    entry->value_len = value_len;
    entry->next = NULL;
    
    // Add to tail
    if (buf->tail) {
        buf->tail->next = entry;
    } else {
        buf->head = entry;
    }
    buf->tail = entry;
    buf->count++;
    
    return 0;
}

int buffer_flush(WriteBuffer *buf, const char *filename) {
    if (!buf || !filename || buf->count == 0) return 0;
    
    FILE *fp = fopen(filename, "ab+");
    if (!fp) return -1;
    
    WriteEntry *current = buf->head;
    while (current) {
        if (write_record_to_file(fp, current->op_type, current->key, 
                                  current->value ? current->value : "", 
                                  current->value_len) != 0) {
            fclose(fp);
            return -1;
        }
        current = current->next;
    }
    
    fclose(fp);
    
    // Clear the buffer after successful flush
    buffer_clear(buf);
    return 0;
}

void buffer_clear(WriteBuffer *buf) {
    if (!buf) return;
    
    WriteEntry *current = buf->head;
    while (current) {
        WriteEntry *next = current->next;
        free(current->key);
        if (current->value) free(current->value);
        free(current);
        current = next;
    }
    
    buf->head = NULL;
    buf->tail = NULL;
    buf->count = 0;
}

void buffer_destroy(WriteBuffer *buf) {
    if (!buf) return;
    buffer_clear(buf);
    free(buf);
}