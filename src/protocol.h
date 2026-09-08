#ifndef HEAVENDB_PROTOCOL_H
#define HEAVENDB_PROTOCOL_H

#include <stdint.h>

// Magic bytes to identify HeavenDB protocol
#define HDB_MAGIC 0x48444231 // "HDB1" in hex

// Command types
typedef enum {
    CMD_SET = 1,
    CMD_GET = 2,
    CMD_DELETE = 3,
    CMD_PING = 4,
    CMD_BENCHMARK = 5,
    CMD_SHUTDOWN = 6
} CommandType;

// Response codes
typedef enum {
    RESP_OK = 200,
    RESP_ERROR = 400,
    RESP_NOT_FOUND = 404,
    RESP_SERVER_ERROR = 500
} ResponseCode;

// Maximum sizes
#define MAX_KEY_SIZE 128
#define MAX_VALUE_SIZE 4096

// The wire format for requests
typedef struct {
    uint32_t magic;          // Magic bytes
    uint32_t command_type;   // What to do
    uint32_t key_length;     // Length of key
    uint32_t value_length;   // Length of value
    // Followed by key bytes, then value bytes
} RequestHeader;

// The wire format for responses
typedef struct {
    uint32_t magic;          // Magic bytes
    uint32_t response_code;  // Success or error
    uint32_t value_length;   // Length of returned value
    // Followed by value bytes (if any)
} ResponseHeader;

#endif // HEAVENDB_PROTOCOL_H