#ifndef HEAVENDB_WEBSOCKET_H
#define HEAVENDB_WEBSOCKET_H

#include <winsock2.h>

#define WS_GUID "258EAFA5-E914-47DA-95CA-5AB0DC85B11"

// Performs WebSocket handshake. Returns 0 on success.
int ws_handshake(SOCKET client_socket, const char *request);

// Parses a WebSocket frame. Returns payload length, -1 on error.
int ws_parse_frame(const char *buffer, int buffer_len, char *payload, int payload_size);

// Creates a WebSocket text frame
int ws_create_frame(const char *payload, int payload_len, char *frame, int frame_size);

#endif // HEAVENDB_WEBSOCKET_H