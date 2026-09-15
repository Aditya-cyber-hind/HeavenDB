#include "http_server.h"
#include "sql.h"
#include "session.h"
#include "auth.h"
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
static SessionStore *session_store = NULL;

// Forward declaration — auth_system is defined in sql.c
extern AuthSystem *auth_get_system(void);

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

// ==================== HTTP HELPERS ====================

static void send_response(SOCKET client, int code, const char *status,
                          const char *content_type, const char *body, int body_len)
{
    char headers[512];
    int n = snprintf(headers, sizeof(headers),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Headers: Authorization, Content-Type\r\n"
        "Connection: close\r\n"
        "\r\n",
        code, status, content_type, body_len);
    
    send(client, headers, n, 0);
    if (body && body_len > 0) {
        send(client, body, body_len, 0);
    }
}

// Extract a header value from the raw HTTP request. Returns a pointer into
// the request string (not null-terminated), or NULL if not found.
// out_len is set to the length of the value (trailing CR trimmed).
static const char *find_header(const char *request, const char *name, int *out_len) {
    size_t name_len = strlen(name);
    const char *p = request;
    
    while ((p = strstr(p, name)) != NULL) {
        // Check that it's the start of a header (previous char is \n)
        if (p == request || p[-1] == '\n') {
            p += name_len;
            if (p[0] == ':' && p[1] == ' ') {
                p += 2;
                const char *end = strstr(p, "\r\n");
                if (!end) return NULL;
                *out_len = (int)(end - p);
                return p;
            }
        }
        p += name_len;
    }
    return NULL;
}

// ==================== STATIC FILES ====================

static void serve_file(SOCKET client, const char *path) {
    if (strstr(path, "..") != NULL ||
        strchr(path, '\\') != NULL ||
        strchr(path, ':') != NULL ||
        strstr(path, "//") != NULL) {
        send_response(client, 400, "Bad Request", "text/plain",
                      "Bad Request", 11);
        return;
    }
    
    char full_path[512];
    if (strcmp(path, "/") == 0 || strlen(path) == 0) {
        snprintf(full_path, sizeof(full_path), "%s\\index.html", DASHBOARD_DIR);
    } else {
        snprintf(full_path, sizeof(full_path), "%s%s", DASHBOARD_DIR, path);
    }
    
    FILE *fp = fopen(full_path, "rb");
    if (!fp) {
        send_response(client, 404, "Not Found", "text/plain",
                      "Not Found", 9);
        return;
    }
    
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (file_size > 1024 * 1024) {   // 1 MB cap
        fclose(fp);
        send_response(client, 500, "Server Error", "text/plain",
                      "File too large", 13);
        return;
    }
    
    char *body = (char*)malloc(file_size);
    if (!body) {
        fclose(fp);
        send_response(client, 500, "Server Error", "text/plain",
                      "Out of memory", 13);
        return;
    }
    
    fread(body, 1, file_size, fp);
    fclose(fp);
    
    send_response(client, 200, "OK", get_content_type(full_path),
                  body, (int)file_size);
    free(body);
}

// ==================== /login ====================

static void handle_login(SOCKET client, const char *body) {
    // Parse form-encoded body: username=...&password=...
    char username[64] = "";
    char password[256] = "";
    
    const char *u = strstr(body, "username=");
    const char *p = strstr(body, "password=");
    
    if (u) {
        u += 9;
        const char *end = strchr(u, '&');
        int len = end ? (int)(end - u) : (int)strlen(u);
        if (len >= (int)sizeof(username)) len = sizeof(username) - 1;
        strncpy(username, u, len);
        username[len] = '\0';
    }
    
    if (p) {
        p += 9;
        const char *end = strchr(p, '&');
        int len = end ? (int)(end - p) : (int)strlen(p);
        if (len >= (int)sizeof(password)) len = sizeof(password) - 1;
        strncpy(password, p, len);
        password[len] = '\0';
    }
    
    // TODO: URL-decode username and password (they may contain %XX)
    
    AuthSystem *auth = auth_get_system();
    if (!auth) {
        send_response(client, 500, "Server Error", "text/plain",
                      "Auth not initialized", 20);
        return;
    }
    
    if (auth_login(auth, username, password) != 0) {
        send_response(client, 401, "Unauthorized", "text/plain",
                      "Invalid credentials", 19);
        return;
    }
    
    const char *token = session_create(session_store, username);
    if (!token) {
        send_response(client, 500, "Server Error", "text/plain",
                      "Session limit reached", 21);
        return;
    }
    
    // Return JSON {"token":"..."}
    char json[128];
    int n = snprintf(json, sizeof(json), "{\"token\":\"%s\"}", token);
    send_response(client, 200, "OK", "application/json", json, n);
}

// ==================== /query ====================

static int extract_bearer(const char *request, char *token_out, size_t out_size) {
    int auth_len = 0;
    const char *auth = find_header(request, "Authorization", &auth_len);
    if (!auth) return 0;
    
    // Expect "Bearer <token>"
    if (auth_len < 8) return 0;
    if (strncmp(auth, "Bearer ", 7) != 0) return 0;
    
    int tok_len = auth_len - 7;
    if (tok_len >= (int)out_size) return 0;
    strncpy(token_out, auth + 7, tok_len);
    token_out[tok_len] = '\0';
    return 1;
}

static void serve_query(SOCKET client, const char *request, const char *query_param) {
    // Extract and validate bearer token
    char token[128];
    char username[64];
    
    if (!extract_bearer(request, token, sizeof(token))) {
        send_response(client, 401, "Unauthorized", "text/plain",
                      "Missing or invalid Authorization header", 43);
        return;
    }
    
    if (!session_validate(session_store, token, username, sizeof(username))) {
        send_response(client, 401, "Unauthorized", "text/plain",
                      "Invalid or expired token", 24);
        return;
    }
    
    // Set the SQL session user
    sql_set_current_user(username);
    
    // Decode URL parameter
    char sql[512];
    int sql_len = 0;
    for (int i = 0; query_param[i] && sql_len < 511; i++) {
        if (query_param[i] == ' ' || query_param[i] == '&' ||
            query_param[i] == '#' || query_param[i] == '\r' ||
            query_param[i] == '\n') {
            break;
        }
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
    
    // Capture stdout
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
    
    send_response(client, 200, "OK", "text/plain", output, bytes_read);
}

// ==================== WORKER ====================

DWORD WINAPI http_worker(LPVOID param) {
    (void)param;
    
    if (!session_store) {
        session_store = session_store_create();
    }
    
    while (http_running) {
        struct sockaddr_in client_addr;
        int client_addr_len = sizeof(client_addr);
        
        SOCKET client = accept(http_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client == INVALID_SOCKET) {
            if (!http_running) break;
            continue;
        }
        
        // Read up to 8 KB (headers + body)
        char request[8192];
        int total = 0;
        while (total < (int)sizeof(request) - 1) {
            int n = recv(client, request + total, sizeof(request) - 1 - total, 0);
            if (n <= 0) break;
            total += n;
            
            // For GET, we're done once we see the header terminator
            request[total] = '\0';
            if (strstr(request, "\r\n\r\n")) break;
        }
        
        if (total > 0) {
            request[total] = '\0';
            
            char method[16] = {0};
            char path[256] = {0};
            sscanf(request, "%15s %255s", method, path);
            
            if (strcmp(method, "POST") == 0 && strcmp(path, "/login") == 0) {
                const char *body = strstr(request, "\r\n\r\n");
                if (body) {
                    body += 4;
                    handle_login(client, body);
                } else {
                    send_response(client, 400, "Bad Request", "text/plain",
                                  "Missing body", 12);
                }
            }
            else if (strcmp(method, "GET") == 0) {
                if (strncmp(path, "/query", 6) == 0) {
                    const char *query_param = strstr(request, "sql=");
                    if (query_param) {
                        serve_query(client, request, query_param + 4);
                    } else {
                        send_response(client, 400, "Bad Request", "text/plain",
                                      "Missing sql parameter", 21);
                    }
                } else {
                    serve_file(client, path);
                }
            }
            else {
                send_response(client, 405, "Method Not Allowed", "text/plain",
                              "Method Not Allowed", 18);
            }
        }
        
        closesocket(client);
    }
    
    return 0;
}

// ==================== START / STOP ====================

int http_server_start(int port) {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    
    http_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (http_socket == INVALID_SOCKET) return -1;
    
    int opt = 1;
    setsockopt(http_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    
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