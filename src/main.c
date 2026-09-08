#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "database.h"
#include "hashmap.h"
#include "sql.h"

#define DB_FILE "heaven.hdb"

static void print_usage(void) {
    printf("HeavenDB - A High-Performance Database Engine\n");
    printf("=============================================\n\n");
    printf("Usage:\n");
    printf("  heavendb set <key> <value>       - Store a value\n");
    printf("  heavendb get <key>               - Retrieve a value\n");
    printf("  heavendb delete <key>            - Delete a key\n");
    printf("  heavendb size                    - Show number of keys\n");
    printf("  heavendb benchmark <n>           - Run n random operations\n");
    printf("  heavendb flush                   - Force write to disk\n");
    printf("  heavendb sql \"<SQL query>\"       - Execute SQL command\n");
    printf("  heavendb help                    - Show this help\n\n");
}

static void run_benchmark(Database *db, int operations) {
    printf("Running benchmark: %d operations...\n\n", operations);
    
    // Phase 1: In-Memory GET benchmark (tests Hash Map speed)
    printf("Phase 1: In-Memory GET Benchmark\n");
    printf("----------------------------------\n");
    
    // Insert 1000 keys into memory directly
    for (int i = 0; i < 1000; i++) {
        char key[32];
        char value[64];
        snprintf(key, sizeof(key), "bench_key_%d", i);
        snprintf(value, sizeof(value), "bench_value_%d", i);
        hashmap_set(db->memory, key, value, strlen(value));
    }
    
    clock_t start = clock();
    
    for (int i = 0; i < operations; i++) {
        char key[32];
        snprintf(key, sizeof(key), "bench_key_%d", i % 1000);
        size_t value_len;
        hashmap_get(db->memory, key, &value_len);
    }
    
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    double ops_per_sec = operations / elapsed;
    
    printf("  Operations: %d\n", operations);
    printf("  Time: %.3f seconds\n", elapsed);
    printf("  Throughput: %.0f ops/sec\n\n", ops_per_sec);
    
    // Phase 2: Disk SET benchmark (tests Group Commit)
    printf("Phase 2: Disk SET Benchmark (Group Commit)\n");
    printf("------------------------------------------\n");
    
    int disk_ops = operations / 10;
    start = clock();
    
    for (int i = 0; i < disk_ops; i++) {
        char key[32];
        char value[64];
        snprintf(key, sizeof(key), "disk_key_%d", i % 100);
        snprintf(value, sizeof(value), "disk_value_%d", i);
        db_set(db, key, value, strlen(value));
    }
    
    // Flush any remaining buffered writes
    db_flush(db);
    
    end = clock();
    elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    ops_per_sec = disk_ops / elapsed;
    
    printf("  Operations: %d\n", disk_ops);
    printf("  Time: %.3f seconds\n", elapsed);
    printf("  Throughput: %.0f ops/sec\n\n", ops_per_sec);
    
    printf("HeavenDB Performance Summary:\n");
    printf("  Memory: Extremely fast (Hash Map)\n");
    printf("  Disk: Optimized with Group Commit\n");
    printf("  Buffer Size: 100 writes per flush\n\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }
    
    if (strcmp(argv[1], "help") == 0) {
        print_usage();
        return 0;
    }
    
    // Handle SQL mode separately (no need for key-value DB)
    if (strcmp(argv[1], "sql") == 0) {
        if (argc != 3) {
            printf("Usage: heavendb sql \"<SQL query>\"\n");
            return 1;
        }
        sql_init();
        sql_execute(argv[2]);
        sql_shutdown();
        return 0;
    }
    
    // Open key-value database
    Database *db = db_open(DB_FILE);
    if (!db) {
        fprintf(stderr, "ERROR: Failed to open database\n");
        return 1;
    }
    
    if (strcmp(argv[1], "set") == 0) {
        if (argc != 4) {
            printf("Usage: heavendb set <key> <value>\n");
            db_close(db);
            return 1;
        }
        if (db_set(db, argv[2], argv[3], strlen(argv[3])) == 0) {
            printf("OK\n");
        } else {
            printf("ERROR: Failed to set key\n");
        }
    }
    else if (strcmp(argv[1], "get") == 0) {
        if (argc != 3) {
            printf("Usage: heavendb get <key>\n");
            db_close(db);
            return 1;
        }
        size_t value_len;
        char *value = db_get(db, argv[2], &value_len);
        if (value) {
            printf("%s\n", value);
        } else {
            printf("(nil)\n");
        }
    }
    else if (strcmp(argv[1], "delete") == 0) {
        if (argc != 3) {
            printf("Usage: heavendb delete <key>\n");
            db_close(db);
            return 1;
        }
        if (db_delete(db, argv[2]) == 0) {
            printf("OK\n");
        } else {
            printf("ERROR: Key not found\n");
        }
    }
    else if (strcmp(argv[1], "size") == 0) {
        printf("%zu\n", db_size(db));
    }
    else if (strcmp(argv[1], "flush") == 0) {
        if (db_flush(db) == 0) {
            printf("OK\n");
        } else {
            printf("ERROR: Flush failed\n");
        }
    }
    else if (strcmp(argv[1], "benchmark") == 0) {
        if (argc != 3) {
            printf("Usage: heavendb benchmark <operations>\n");
            db_close(db);
            return 1;
        }
        int ops = atoi(argv[2]);
        if (ops <= 0) {
            printf("ERROR: Operations must be positive\n");
            db_close(db);
            return 1;
        }
        run_benchmark(db, ops);
    }
    else {
        printf("ERROR: Unknown command '%s'\n", argv[1]);
        print_usage();
    }
    
    db_close(db);
    return 0;
}