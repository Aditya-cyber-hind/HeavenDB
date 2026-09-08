#include "tcp_server.h"
#include "sql.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <io.h>
#include <fcntl.h>

static int server_running = 0;
static SOCKET server_socket;

// Thread function to handle a client
DWORD WINAPI handle_client(LPVOID client_socket_ptr) {
    SOCKET client_socket = (SOCKET)client_socket_ptr;
    char buffer[BUFFER_SIZE];
    char line[BUFFER_SIZE];
    int line_pos = 0;
    
    // Send welcome message
    char welcome[] = "Welcome to HeavenDB Server\nType SQL commands or 'exit' to disconnect.\n\n";
    send(client_socket, welcome, (int)strlen(welcome), 0);
    
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_received <= 0) {
            break; // Client disconnected
        }
        
        // Process each character received
        for (int i = 0; i < bytes_received; i++) {
            char c = buffer[i];
            
            if (c == '\n' || c == '\r') {
                if (line_pos > 0) {
                    line[line_pos] = '\0';
                    
                    if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0) {
                        char bye[] = "Bye!\n";
                        send(client_socket, bye, (int)strlen(bye), 0);
                        closesocket(client_socket);
                        return 0;
                    }
                    
                    if (strlen(line) > 0) {
                        // Send prompt back to client
                        char prompt[BUFFER_SIZE];
                        snprintf(prompt, BUFFER_SIZE, "heavendb> %s\n", line);
                        send(client_socket, prompt, (int)strlen(prompt), 0);
                        
                        // Redirect stdout to a pipe so we capture the output
                        int pipe_fds[2];
                        if (_pipe(pipe_fds, 65536, O_BINARY) == 0) {
                            int old_stdout = _dup(1);
                            _dup2(pipe_fds[1], 1);
                            _close(pipe_fds[1]);
                            
                            // Execute the SQL command
                            sql_execute(line);
                            
                            // Flush and restore stdout
                            fflush(stdout);
                            _dup2(old_stdout, 1);
                            _close(old_stdout);
                            
                            // Read the captured output
                            char output_buffer[8192];
                            memset(output_buffer, 0, sizeof(output_buffer));
                            int bytes_read = _read(pipe_fds[0], output_buffer, sizeof(output_buffer) - 1);
                            _close(pipe_fds[0]);
                            
                            if (bytes_read > 0) {
                                output_buffer[bytes_read] = '\0';
                                send(client_socket, output_buffer, bytes_read, 0);
                            }
                        } else {
                            // Fallback: just execute without capture
                            sql_execute(line);
                        }
                    }
                    
                    line_pos = 0;
                }
            } else {
                if (line_pos < BUFFER_SIZE - 1) {
                    line[line_pos++] = c;
                }
            }
        }
    }
    
    closesocket(client_socket);
    return 0;
}

int tcp_server_start(int port) {
    WSADATA wsa;
    struct sockaddr_in server_addr;
    
    printf("========================================\n");
    printf("     HeavenDB TCP Server Starting...    \n");
    printf("========================================\n");
    
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("ERROR: Failed to initialize Winsock\n");
        return -1;
    }
    
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("ERROR: Failed to create socket\n");
        WSACleanup();
        return -1;
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("ERROR: Failed to bind port %d\n", port);
        closesocket(server_socket);
        WSACleanup();
        return -1;
    }
    
    if (listen(server_socket, MAX_CLIENTS) == SOCKET_ERROR) {
        printf("ERROR: Failed to listen\n");
        closesocket(server_socket);
        WSACleanup();
        return -1;
    }
    
    server_running = 1;
    printf("HeavenDB Server listening on port %d\n", port);
    printf("Connect using: telnet localhost %d\n\n", port);
    
    while (server_running) {
        struct sockaddr_in client_addr;
        int client_addr_len = sizeof(client_addr);
        
        SOCKET client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        
        if (client_socket == INVALID_SOCKET) {
            if (!server_running) break;
            continue;
        }
        
        printf("Client connected!\n");
        
        HANDLE thread = CreateThread(NULL, 0, handle_client, (LPVOID)client_socket, 0, NULL);
        
        if (thread) {
            CloseHandle(thread);
        }
    }
    
    closesocket(server_socket);
    WSACleanup();
    return 0;
}

void tcp_server_stop(void) {
    server_running = 0;
    closesocket(server_socket);
}