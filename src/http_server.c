#include "http_server.h"
#include "sql.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <io.h>
#include <fcntl.h>

static int http_running = 0;
static SOCKET http_socket;
static int sql_initialized = 0;

static const char *get_content_type(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext) return "text/plain";
    
    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".js") == 0) return "text/javascript";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".ico") == 0) return "image/x-icon";
    return "text/plain";
}

static void serve_file(SOCKET client, const char *path) {
    char full_path[512];
    
    if (strcmp(path, "/") == 0 || strlen(path) == 0) {
        snprintf(full_path, sizeof(full_path), "%s\\index.html", DASHBOARD_DIR);
    } else {
        snprintf(full_path, sizeof(full_path), "%s%s", DASHBOARD_DIR, path);
    }
    
    FILE *fp = fopen(full_path, "rb");
    if (!fp) {
        char response[] = "HTTP/1.1 404 Not Found\r\n"
                         "Content-Type: text/plain\r\n"
                         "Content-Length: 9\r\n"
                         "Connection: close\r\n"
                         "\r\n"
                         "Not Found";
        send(client, response, (int)strlen(response), 0);
        return;
    }
    
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char headers[512];
    snprintf(headers, sizeof(headers),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n",
        get_content_type(full_path), file_size);
    
    send(client, headers, (int)strlen(headers), 0);
    
    char buffer[4096];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        send(client, buffer, (int)bytes_read, 0);
    }
    
    fclose(fp);
}

static void serve_query(SOCKET client, const char *query_param) {
    // Initialize SQL engine on first use
    if (!sql_initialized) {
        sql_init();
        sql_initialized = 1;
    } else {
        // Reload tables from disk to ensure we have latest data
        sql_load();
    }
    
    char sql[512];
    int sql_len = 0;
    
    for (int i = 0; query_param[i] && sql_len < 511; i++) {
        if (query_param[i] == '%' && query_param[i+1] && query_param[i+2]) {
            char hex[3] = {query_param[i+1], query_param[i+2], '\0'};
            sql[sql_len++] = (char)strtol(hex, NULL, 16);
            i += 2;
        } else if (query_param[i] == '+') {
            sql[sql_len++] = ' ';
        } else {
            sql[sql_len++] = query_param[i];
        }
    }
    sql[sql_len] = '\0';
    
    int pipe_fds[2];
    _pipe(pipe_fds, 65536, O_BINARY);
    int old_stdout = _dup(1);
    _dup2(pipe_fds[1], 1);
    _close(pipe_fds[1]);
    
    sql_execute(sql);
    
    fflush(stdout);
    _dup2(old_stdout, 1);
    _close(old_stdout);
    
    char output[8192];
    memset(output, 0, sizeof(output));
    int bytes_read = _read(pipe_fds[0], output, sizeof(output) - 1);
    _close(pipe_fds[0]);
    
    if (bytes_read <= 0) {
        strcpy(output, "OK\n");
        bytes_read = 3;
    }
    
    output[bytes_read] = '\0';
    
    char headers[512];
    snprintf(headers, sizeof(headers),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %d\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n"
        "\r\n", bytes_read);
    
    send(client, headers, (int)strlen(headers), 0);
    send(client, output, bytes_read, 0);
}

DWORD WINAPI http_worker(LPVOID param) {
    (void)param;
    
    while (http_running) {
        struct sockaddr_in client_addr;
        int client_addr_len = sizeof(client_addr);
        
        SOCKET client = accept(http_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client == INVALID_SOCKET) {
            if (!http_running) break;
            continue;
        }
        
        char request[4096];
        int bytes_received = recv(client, request, sizeof(request) - 1, 0);
        
        if (bytes_received > 0) {
            request[bytes_received] = '\0';
            
            char method[16];
            char path[256];
            
            sscanf(request, "%15s %255s", method, path);
            
            if (strcmp(method, "GET") == 0) {
                if (strncmp(path, "/query", 6) == 0) {
                    const char *query_param = strstr(request, "sql=");
                    if (query_param) {
                        query_param += 4;
                        serve_query(client, query_param);
                    }
                } else {
                    serve_file(client, path);
                }
            }
        }
        
        closesocket(client);
    }
    
    return 0;
}

int http_server_start(int port) {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    
    http_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (http_socket == INVALID_SOCKET) {
        return -1;
    }
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(http_socket, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(http_socket);
        return -1;
    }
    
    listen(http_socket, 5);
    
    http_running = 1;
    
    HANDLE thread = CreateThread(NULL, 0, http_worker, NULL, 0, NULL);
    if (thread) CloseHandle(thread);
    
    printf("Dashboard available at http://localhost:%d\n", port);
    
    return 0;
}

void http_server_stop(void) {
    http_running = 0;
    closesocket(http_socket);
}