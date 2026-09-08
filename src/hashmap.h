#ifndef HEAVENDB_HASHMAP_H
#define HEAVENDB_HASHMAP_H

#include <stddef.h>

#define HASH_TABLE_SIZE 4096 // More buckets = fewer collisions

typedef struct HNode {
    char *key;
    char *value;
    size_t value_len;
    struct HNode *next;
} HNode;

typedef struct HashMap {
    HNode *buckets[HASH_TABLE_SIZE];
    size_t size; // Number of key-value pairs
} HashMap;

// Creates a new empty hash map
HashMap *hashmap_create(void);

// Frees all memory used by the map
void hashmap_destroy(HashMap *map);

// Inserts or updates a key-value pair. Returns 0 on success, -1 on error.
int hashmap_set(HashMap *map, const char *key, const char *value, size_t value_len);

// Gets a value by key. Returns NULL if not found. Sets value_len if provided.
char *hashmap_get(HashMap *map, const char *key, size_t *value_len);

// Deletes a key. Returns 0 on success, -1 if key not found.
int hashmap_delete(HashMap *map, const char *key);

// Returns the number of key-value pairs
size_t hashmap_size(const HashMap *map);

// Iterates over all entries. Returns 0 when done.
int hashmap_iterate(HashMap *map, char **key, char **value, size_t *value_len, void **iterator_state);

#endif // HEAVENDB_HASHMAP_H