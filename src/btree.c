#include "btree.h"
#include <stdlib.h>
#include <stdio.h>

// Create a new node
static BTreeNode *node_create(int is_leaf) {
    BTreeNode *node = (BTreeNode*)malloc(sizeof(BTreeNode));
    node->keys = (int*)malloc(BTREE_MAX_KEYS * sizeof(int));
    node->values = (void**)malloc(BTREE_MAX_KEYS * sizeof(void*));
    node->children = (BTreeNode**)malloc((BTREE_MAX_KEYS + 1) * sizeof(BTreeNode*));
    node->num_keys = 0;
    node->is_leaf = is_leaf;
    return node;
}

BTree *btree_create(void) {
    BTree *tree = (BTree*)malloc(sizeof(BTree));
    tree->root = node_create(1); // Root starts as leaf
    tree->size = 0;
    return tree;
}

// Split a full child node
static void split_child(BTreeNode *parent, int index, BTreeNode *child) {
    BTreeNode *new_node = node_create(child->is_leaf);
    new_node->num_keys = BTREE_MIN_KEYS;
    
    // Copy the last BTREE_MIN_KEYS keys to the new node
    // Starting from index BTREE_MIN_KEYS + 1
    int copy_start = BTREE_MIN_KEYS + 1;
    int keys_to_copy = child->num_keys - copy_start;
    
    for (int i = 0; i < keys_to_copy; i++) {
        new_node->keys[i] = child->keys[copy_start + i];
        new_node->values[i] = child->values[copy_start + i];
    }
    new_node->num_keys = keys_to_copy;
    
    // If not leaf, copy children too
    if (!child->is_leaf) {
        for (int i = 0; i <= keys_to_copy; i++) {
            new_node->children[i] = child->children[copy_start + i];
        }
    }
    
    child->num_keys = BTREE_MIN_KEYS;
    
    // Shift parent's children to make room
    for (int i = parent->num_keys; i > index; i--) {
        parent->children[i + 1] = parent->children[i];
    }
    parent->children[index + 1] = new_node;
    
    // Shift parent's keys to make room
    for (int i = parent->num_keys - 1; i >= index; i--) {
        parent->keys[i + 1] = parent->keys[i];
        parent->values[i + 1] = parent->values[i];
    }
    
    // Move the middle key up to parent
    parent->keys[index] = child->keys[BTREE_MIN_KEYS];
    parent->values[index] = child->values[BTREE_MIN_KEYS];
    parent->num_keys++;
}

// Insert into a non-full node
static void insert_non_full(BTreeNode *node, int key, void *value) {
    int i = node->num_keys - 1;
    
    if (node->is_leaf) {
        // Find the right position
        while (i >= 0 && key < node->keys[i]) {
            node->keys[i + 1] = node->keys[i];
            node->values[i + 1] = node->values[i];
            i--;
        }
        
        node->keys[i + 1] = key;
        node->values[i + 1] = value;
        node->num_keys++;
    } else {
        // Find the child to descend into
        while (i >= 0 && key < node->keys[i]) {
            i--;
        }
        i++;
        
        // If child is full, split it
        if (node->children[i]->num_keys == BTREE_MAX_KEYS) {
            split_child(node, i, node->children[i]);
            if (key > node->keys[i]) {
                i++;
            }
        }
        insert_non_full(node->children[i], key, value);
    }
}

int btree_insert(BTree *tree, int key, void *value) {
    if (!tree) return -1;
    
    BTreeNode *root = tree->root;
    
    // If root is full, split it
    if (root->num_keys == BTREE_MAX_KEYS) {
        BTreeNode *new_root = node_create(0);
        new_root->children[0] = root;
        split_child(new_root, 0, root);
        tree->root = new_root;
        
        // Insert into the appropriate child
        int i = 0;
        if (key > new_root->keys[0]) {
            i = 1;
        }
        insert_non_full(new_root->children[i], key, value);
    } else {
        insert_non_full(root, key, value);
    }
    
    tree->size++;
    return 0;
}

void *btree_search(BTree *tree, int key) {
    if (!tree || !tree->root) return NULL;
    
    BTreeNode *node = tree->root;
    
    while (node) {
        int i = 0;
        while (i < node->num_keys && key > node->keys[i]) {
            i++;
        }
        
        if (i < node->num_keys && key == node->keys[i]) {
            return node->values[i];
        }
        
        if (node->is_leaf) {
            return NULL;
        }
        
        node = node->children[i];
    }
    
    return NULL;
}

// Recursive helper for range search
static void range_search_node(BTreeNode *node, int min, int max, void (*callback)(int key, void *value)) {
    if (!node) return;
    
    int i = 0;
    while (i < node->num_keys && node->keys[i] < min) {
        i++;
    }
    
    if (!node->is_leaf) {
        range_search_node(node->children[i], min, max, callback);
    }
    
    while (i < node->num_keys && node->keys[i] <= max) {
        callback(node->keys[i], node->values[i]);
        i++;
        if (!node->is_leaf) {
            range_search_node(node->children[i], min, max, callback);
        }
    }
}

void btree_range_search(BTree *tree, int min, int max, void (*callback)(int key, void *value)) {
    if (!tree || !tree->root || !callback) return;
    range_search_node(tree->root, min, max, callback);
}

// Recursive destroy
static void destroy_node(BTreeNode *node) {
    if (!node) return;
    
    if (!node->is_leaf) {
        for (int i = 0; i <= node->num_keys; i++) {
            destroy_node(node->children[i]);
        }
    }
    
    free(node->keys);
    free(node->values);
    free(node->children);
    free(node);
}

void btree_destroy(BTree *tree) {
    if (!tree) return;
    destroy_node(tree->root);
    free(tree);
}