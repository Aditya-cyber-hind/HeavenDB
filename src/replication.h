#ifndef HEAVENDB_REPLICATION_H
#define HEAVENDB_REPLICATION_H

#define MAX_REPLICAS 8
#define REPLICA_HOST_LEN 128

typedef struct {
    char host[REPLICA_HOST_LEN];
    int port;
    int is_active;
} Replica;

typedef struct {
    Replica replicas[MAX_REPLICAS];
    int replica_count;
} ReplicationSystem;

// Creates a replication system
ReplicationSystem *replication_create(void);

// Adds a replica
int replication_add_replica(ReplicationSystem *rep, const char *host, int port);

// Syncs data to all replicas
int replication_sync(ReplicationSystem *rep, const char *data);

// Frees the replication system
void replication_destroy(ReplicationSystem *rep);

#endif // HEAVENDB_REPLICATION_H