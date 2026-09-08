#include "hashmap.h"
#include <stdlib.h>
#include <string.h>

// djb2 hash function by Dan Bernstein
static unsigned long hash_function(const char *key) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash % HASH_TABLE_SIZE;
}

HashMap *hashmap_create(void) {
    HashMap *map = (HashMap*)malloc(sizeof(HashMap));
    if (!map) return NULL;
    
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        map->buckets[i] = NULL;
    }
    map->size = 0;
    return map;
}

void hashmap_destroy(HashMap *map) {
    if (!map) return;
    
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        HNode *current = map->buckets[i];
        while (current) {
            HNode *next = current->next;
            free(current->key);
            free(current->value);
            free(current);
            current = next;
        }
    }
    free(map);
}

int hashmap_set(HashMap *map, const char *key, const char *value, size_t value_len) {
    if (!map || !key || !value) return -1;
    
    unsigned long index = hash_function(key);
    
    // Check if key already exists
    HNode *current = map->buckets[index];
    while (current) {
        if (strcmp(current->key, key) == 0) {
            // Update existing value
            char *new_value = (char*)realloc(current->value, value_len + 1);
            if (!new_value) return -1;
            current->value = new_value;
            memcpy(current->value, value, value_len);
            current->value[value_len] = '\0';
            current->value_len = value_len;
            return 0;
        }
        current = current->next;
    }
    
    // Create new node
    HNode *new_node = (HNode*)malloc(sizeof(HNode));
    if (!new_node) return -1;
    
    new_node->key = (char*)malloc(strlen(key) + 1);
    if (!new_node->key) {
        free(new_node);
        return -1;
    }
    strcpy(new_node->key, key);
    
    new_node->value = (char*)malloc(value_len + 1);
    if (!new_node->value) {
        free(new_node->key);
        free(new_node);
        return -1;
    }
    memcpy(new_node->value, value, value_len);
    new_node->value[value_len] = '\0';
    new_node->value_len = value_len;
    
    // Insert at beginning (O(1))
    new_node->next = map->buckets[index];
    map->buckets[index] = new_node;
    map->size++;
    
    return 0;
}

char *hashmap_get(HashMap *map, const char *key, size_t *value_len) {
    if (!map || !key) return NULL;
    
    unsigned long index = hash_function(key);
    HNode *current = map->buckets[index];
    
    while (current) {
        if (strcmp(current->key, key) == 0) {
            if (value_len) *value_len = current->value_len;
            return current->value;
        }
        current = current->next;
    }
    return NULL;
}

int hashmap_delete(HashMap *map, const char *key) {
    if (!map || !key) return -1;
    
    unsigned long index = hash_function(key);
    HNode *current = map->buckets[index];
    HNode *prev = NULL;
    
    while (current) {
        if (strcmp(current->key, key) == 0) {
            if (prev) {
                prev->next = current->next;
            } else {
                map->buckets[index] = current->next;
            }
            free(current->key);
            free(current->value);
            free(current);
            map->size--;
            return 0;
        }
        prev = current;
        current = current->next;
    }
    return -1;
}

size_t hashmap_size(const HashMap *map) {
    return map ? map->size : 0;
}

int hashmap_iterate(HashMap *map, char **key, char **value, size_t *value_len, void **iterator_state) {
    if (!map || !key || !value || !iterator_state) return 0;
    
    size_t *bucket_index = (size_t*)iterator_state;
    HNode **current_node = (HNode**)(iterator_state + sizeof(size_t));
    
    // Initialize on first call
    if (*bucket_index == 0 && *current_node == NULL) {
        *bucket_index = 0;
        *current_node = map->buckets[0];
    }
    
    // Find next non-empty bucket
    while (*bucket_index < HASH_TABLE_SIZE && *current_node == NULL) {
        (*bucket_index)++;
        if (*bucket_index < HASH_TABLE_SIZE) {
            *current_node = map->buckets[*bucket_index];
        }
    }
    
    if (*bucket_index >= HASH_TABLE_SIZE) {
        return 0; // Done
    }
    
    HNode *node = *current_node;
    *key = node->key;
    *value = node->value;
    if (value_len) *value_len = node->value_len;
    
    // Move to next node
    *current_node = node->next;
    
    return 1;
}