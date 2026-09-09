#ifndef HEAVENDB_HTTP_SERVER_H
#define HEAVENDB_HTTP_SERVER_H

#define HTTP_PORT 8080
#define DASHBOARD_DIR "dashboard"

// Starts the HTTP server for the dashboard. Runs in a separate thread.
int http_server_start(int port);

// Stops the HTTP server
void http_server_stop(void);

#endif // HEAVENDB_HTTP_SERVER_H