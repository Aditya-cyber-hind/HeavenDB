#include "replication.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

ReplicationSystem *replication_create(void) {
    ReplicationSystem *rep = (ReplicationSystem*)malloc(sizeof(ReplicationSystem));
    if (!rep) return NULL;
    rep->replica_count = 0;
    return rep;
}

int replication_add_replica(ReplicationSystem *rep, const char *host, int port) {
    if (!rep || !host) return -1;
    if (rep->replica_count >= MAX_REPLICAS) return -1;
    
    Replica *replica = &rep->replicas[rep->replica_count];
    strncpy(replica->host, host, REPLICA_HOST_LEN - 1);
    replica->host[REPLICA_HOST_LEN - 1] = '\0';
    replica->port = port;
    replica->is_active = 1;
    
    rep->replica_count++;
    return 0;
}

int replication_sync(ReplicationSystem *rep, const char *data) {
    if (!rep || !data) return -1;
    
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    
    for (int i = 0; i < rep->replica_count; i++) {
        Replica *replica = &rep->replicas[i];
        
        if (!replica->is_active) continue;
        
        SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) continue;
        
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(replica->port);
        addr.sin_addr.s_addr = inet_addr(replica->host);
        
        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            closesocket(sock);
            continue;
        }
        
        send(sock, data, (int)strlen(data), 0);
        closesocket(sock);
    }
    
    WSACleanup();
    return 0;
}

void replication_destroy(ReplicationSystem *rep) {
    if (rep) {
        free(rep);
    }
}