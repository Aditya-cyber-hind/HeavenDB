#ifndef HEAVENDB_TCP_SERVER_H
#define HEAVENDB_TCP_SERVER_H

#define TCP_PORT 6379
#define MAX_CLIENTS 10
#define BUFFER_SIZE 4096

// Starts the TCP server. Blocks until server stops.
int tcp_server_start(int port);

// Stops the TCP server
void tcp_server_stop(void);

#endif // HEAVENDB_TCP_SERVER_H