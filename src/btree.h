#ifndef HEAVENDB_BTREE_H
#define HEAVENDB_BTREE_H

#include <stddef.h>

#define BTREE_MAX_KEYS 4  // Max keys per node (small for testing)
#define BTREE_MIN_KEYS 2  // Min keys per node (half of max)

typedef struct BTreeNode {
    int *keys;                  // Array of keys (integers for now)
    void **values;              // Array of values (pointers to row data)
    struct BTreeNode **children; // Array of child pointers
    int num_keys;               // Current number of keys
    int is_leaf;                // 1 if leaf, 0 if internal
} BTreeNode;

typedef struct {
    BTreeNode *root;
    int size;  // Total number of keys
} BTree;

// Creates a new empty B-Tree
BTree *btree_create(void);

// Inserts a key-value pair into the B-Tree
int btree_insert(BTree *tree, int key, void *value);

// Searches for a key. Returns the value if found, NULL otherwise.
void *btree_search(BTree *tree, int key);

// Searches for all keys in a range [min, max]. Calls callback for each.
void btree_range_search(BTree *tree, int min, int max, void (*callback)(int key, void *value));

// Frees the B-Tree
void btree_destroy(BTree *tree);

#endif // HEAVENDB_BTREE_H