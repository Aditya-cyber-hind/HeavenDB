#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/hashmap.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) printf("  [TEST] %s... ", name)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); tests_failed++; } while(0)

void test_create_destroy(void) {
    TEST("create and destroy empty map");
    HashMap *map = hashmap_create();
    if (!map) { FAIL("create returned NULL"); return; }
    if (hashmap_size(map) != 0) { FAIL("size not 0"); hashmap_destroy(map); return; }
    hashmap_destroy(map);
    PASS();
}

void test_set_get(void) {
    TEST("set and get a single value");
    HashMap *map = hashmap_create();
    hashmap_set(map, "name", "Aditya", 6);
    size_t len;
    char *val = hashmap_get(map, "name", &len);
    if (!val) { FAIL("get returned NULL"); hashmap_destroy(map); return; }
    if (strcmp(val, "Aditya") != 0) { FAIL("wrong value"); hashmap_destroy(map); return; }
    if (len != 6) { FAIL("wrong length"); hashmap_destroy(map); return; }
    hashmap_destroy(map);
    PASS();
}

void test_update(void) {
    TEST("update existing key");
    HashMap *map = hashmap_create();
    hashmap_set(map, "key", "value1", 6);
    hashmap_set(map, "key", "value2", 6);
    size_t len;
    char *val = hashmap_get(map, "key", &len);
    if (strcmp(val, "value2") != 0) { FAIL("update failed"); hashmap_destroy(map); return; }
    if (hashmap_size(map) != 1) { FAIL("size should be 1 after update"); hashmap_destroy(map); return; }
    hashmap_destroy(map);
    PASS();
}

void test_delete(void) {
    TEST("delete a key");
    HashMap *map = hashmap_create();
    hashmap_set(map, "key", "value", 5);
    if (hashmap_delete(map, "key") != 0) { FAIL("delete failed"); hashmap_destroy(map); return; }
    if (hashmap_get(map, "key", NULL) != NULL) { FAIL("key still exists"); hashmap_destroy(map); return; }
    if (hashmap_size(map) != 0) { FAIL("size not 0"); hashmap_destroy(map); return; }
    hashmap_destroy(map);
    PASS();
}

void test_missing_key(void) {
    TEST("get missing key returns NULL");
    HashMap *map = hashmap_create();
    if (hashmap_get(map, "nonexistent", NULL) != NULL) { FAIL("returned non-NULL"); hashmap_destroy(map); return; }
    hashmap_destroy(map);
    PASS();
}

void test_many_keys(void) {
    TEST("insert 10,000 keys");
    HashMap *map = hashmap_create();
    char key[32], value[32];
    for (int i = 0; i < 10000; i++) {
        snprintf(key, sizeof(key), "key_%d", i);
        snprintf(value, sizeof(value), "val_%d", i);
        hashmap_set(map, key, value, strlen(value));
    }
    if (hashmap_size(map) != 10000) { FAIL("wrong size"); hashmap_destroy(map); return; }
    
    // Verify all
    for (int i = 0; i < 10000; i++) {
        snprintf(key, sizeof(key), "key_%d", i);
        char *val = hashmap_get(map, key, NULL);
        if (!val) { FAIL("missing key after insert"); hashmap_destroy(map); return; }
    }
    hashmap_destroy(map);
    PASS();
}

void test_collision(void) {
    TEST("hash collisions handled correctly");
    HashMap *map = hashmap_create();
    // These strings hash to similar buckets in most hash functions
    hashmap_set(map, "abc", "1", 1);
    hashmap_set(map, "acb", "2", 1);
    hashmap_set(map, "bac", "3", 1);
    hashmap_set(map, "bca", "4", 1);
    hashmap_set(map, "cab", "5", 1);
    hashmap_set(map, "cba", "6", 1);
    
    if (strcmp(hashmap_get(map, "abc", NULL), "1") != 0) { FAIL("abc wrong"); hashmap_destroy(map); return; }
    if (strcmp(hashmap_get(map, "acb", NULL), "2") != 0) { FAIL("acb wrong"); hashmap_destroy(map); return; }
    if (strcmp(hashmap_get(map, "cba", NULL), "6") != 0) { FAIL("cba wrong"); hashmap_destroy(map); return; }
    hashmap_destroy(map);
    PASS();
}

void test_empty_value(void) {
    TEST("empty value stored correctly");
    HashMap *map = hashmap_create();
    hashmap_set(map, "empty", "", 0);
    char *val = hashmap_get(map, "empty", NULL);
    if (!val) { FAIL("empty value returned NULL"); hashmap_destroy(map); return; }
    if (strlen(val) != 0) { FAIL("value not empty"); hashmap_destroy(map); return; }
    hashmap_destroy(map);
    PASS();
}

void test_large_value(void) {
    TEST("large value (1000 chars) stored correctly");
    HashMap *map = hashmap_create();
    char large_value[1001];
    for (int i = 0; i < 1000; i++) large_value[i] = 'A' + (i % 26);
    large_value[1000] = '\0';
    
    hashmap_set(map, "big", large_value, 1000);
    size_t len;
    char *val = hashmap_get(map, "big", &len);
    if (len != 1000) { FAIL("wrong length"); hashmap_destroy(map); return; }
    if (strcmp(val, large_value) != 0) { FAIL("wrong content"); hashmap_destroy(map); return; }
    hashmap_destroy(map);
    PASS();
}

void test_delete_missing(void) {
    TEST("delete missing key returns -1");
    HashMap *map = hashmap_create();
    if (hashmap_delete(map, "nonexistent") != -1) { FAIL("should return -1"); hashmap_destroy(map); return; }
    hashmap_destroy(map);
    PASS();
}

void test_replace_value(void) {
    TEST("set replaces existing value");
    HashMap *map = hashmap_create();
    hashmap_set(map, "k", "short", 5);
    hashmap_set(map, "k", "much longer value here", 22);
    char *val = hashmap_get(map, "k", NULL);
    if (strcmp(val, "much longer value here") != 0) { FAIL("replace failed"); hashmap_destroy(map); return; }
    hashmap_destroy(map);
    PASS();
}

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║  HeavenDB Hashmap Unit Tests           ║\n");
    printf("╚════════════════════════════════════════╝\n\n");
    
    test_create_destroy();
    test_set_get();
    test_update();
    test_delete();
    test_missing_key();
    test_many_keys();
    test_collision();
    test_empty_value();
    test_large_value();
    test_delete_missing();
    test_replace_value();
    
    printf("\n");
    printf("════════════════════════════════════════\n");
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("════════════════════════════════════════\n\n");
    
    return tests_failed == 0 ? 0 : 1;
}