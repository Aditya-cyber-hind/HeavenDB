#ifndef HEAVENDB_BUFFER_H
#define HEAVENDB_BUFFER_H

#include <stdint.h>
#include <stddef.h>

// A single pending write operation
typedef struct WriteEntry {
    uint32_t op_type;
    char *key;
    char *value;
    size_t value_len;
    struct WriteEntry *next;
} WriteEntry;

// The write buffer
typedef struct {
    WriteEntry *head;
    WriteEntry *tail;
    int count;
    int max_entries; // Flush when count reaches this
} WriteBuffer;

// Creates a new write buffer
WriteBuffer *buffer_create(int max_entries);

// Adds a write to the buffer. Returns 0 on success.
int buffer_add(WriteBuffer *buf, uint32_t op_type, const char *key, const char *value, size_t value_len);

// Flushes all pending writes to disk. Returns 0 on success.
int buffer_flush(WriteBuffer *buf, const char *filename);

// Clears the buffer without flushing (for shutdown).
void buffer_clear(WriteBuffer *buf);

// Destroys the buffer and frees all memory.
void buffer_destroy(WriteBuffer *buf);

#endif // HEAVENDB_BUFFER_H