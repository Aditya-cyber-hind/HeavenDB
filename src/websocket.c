#include "websocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    unsigned int state[5];
    unsigned int count[2];
    unsigned char buffer[64];
} SHA1_CTX;

#define SHA1_ROTL(a,b) (((a) << (b)) | ((a) >> (32-(b))))

static void sha1_transform(unsigned int state[5], const unsigned char buffer[64]) {
    unsigned int a, b, c, d, e, w[80];
    int i;
    
    for (i = 0; i < 16; i++) {
        w[i] = (buffer[i*4] << 24) | (buffer[i*4+1] << 16) | (buffer[i*4+2] << 8) | buffer[i*4+3];
    }
    for (i = 16; i < 80; i++) {
        w[i] = SHA1_ROTL(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);
    }
    
    a = state[0]; b = state[1]; c = state[2]; d = state[3]; e = state[4];
    
    for (i = 0; i < 20; i++) {
        unsigned int temp = SHA1_ROTL(a, 5) + ((b & c) | ((~b) & d)) + e + w[i] + 0x5A827999;
        e = d; d = c; c = SHA1_ROTL(b, 30); b = a; a = temp;
    }
    for (i = 20; i < 40; i++) {
        unsigned int temp = SHA1_ROTL(a, 5) + (b ^ c ^ d) + e + w[i] + 0x6ED9EBA1;
        e = d; d = c; c = SHA1_ROTL(b, 30); b = a; a = temp;
    }
    for (i = 40; i < 60; i++) {
        unsigned int temp = SHA1_ROTL(a, 5) + ((b & c) | (b & d) | (c & d)) + e + w[i] + 0x8F1BBCDC;
        e = d; d = c; c = SHA1_ROTL(b, 30); b = a; a = temp;
    }
    for (i = 60; i < 80; i++) {
        unsigned int temp = SHA1_ROTL(a, 5) + (b ^ c ^ d) + e + w[i] + 0xCA62C1D6;
        e = d; d = c; c = SHA1_ROTL(b, 30); b = a; a = temp;
    }
    
    state[0] += a; state[1] += b; state[2] += c; state[3] += d; state[4] += e;
}

static void sha1_init(SHA1_CTX *ctx) {
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xC3D2E1F0;
    ctx->count[0] = 0;
    ctx->count[1] = 0;
}

static void sha1_update(SHA1_CTX *ctx, const unsigned char *data, unsigned int len) {
    unsigned int i, j;
    
    j = (ctx->count[0] >> 3) & 63;
    ctx->count[0] += len << 3;
    if (ctx->count[0] < (len << 3)) ctx->count[1]++;
    ctx->count[1] += len >> 29;
    
    if ((j + len) > 63) {
        memcpy(&ctx->buffer[j], data, (i = 64 - j));
        sha1_transform(ctx->state, ctx->buffer);
        for (; i + 63 < len; i += 64) {
            sha1_transform(ctx->state, &data[i]);
        }
        j = 0;
    } else {
        i = 0;
    }
    memcpy(&ctx->buffer[j], &data[i], len - i);
}

static void sha1_final(SHA1_CTX *ctx, unsigned char digest[20]) {
    unsigned int i;
    unsigned char finalcount[8];
    
    for (i = 0; i < 8; i++) {
        finalcount[i] = (unsigned char)((ctx->count[(i >= 4 ? 0 : 1)] >> ((3 - (i & 3)) * 8)) & 255);
    }
    
    unsigned char pad = 0x80;
    sha1_update(ctx, &pad, 1);
    
    unsigned char zero = 0;
    while ((ctx->count[0] & 504) != 448) {
        sha1_update(ctx, &zero, 1);
    }
    
    sha1_update(ctx, finalcount, 8);
    
    for (i = 0; i < 20; i++) {
        digest[i] = (unsigned char)((ctx->state[i >> 2] >> ((3 - (i & 3)) * 8)) & 255);
    }
}

static void real_sha1(const char *input, int input_len, unsigned char *output) {
    SHA1_CTX ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, (const unsigned char*)input, (unsigned int)input_len);
    sha1_final(&ctx, output);
}

static void base64_encode(const unsigned char *input, int input_len, char *output) {
    static const char *chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int i, j = 0;
    
    for (i = 0; i < input_len - 2; i += 3) {
        output[j++] = chars[(input[i] >> 2) & 0x3F];
        output[j++] = chars[((input[i] & 0x03) << 4) | ((input[i+1] >> 4) & 0x0F)];
        output[j++] = chars[((input[i+1] & 0x0F) << 2) | ((input[i+2] >> 6) & 0x03)];
        output[j++] = chars[input[i+2] & 0x3F];
    }
    
    if (i < input_len) {
        output[j++] = chars[(input[i] >> 2) & 0x3F];
        if (i + 1 < input_len) {
            output[j++] = chars[((input[i] & 0x03) << 4) | ((input[i+1] >> 4) & 0x0F)];
            output[j++] = chars[((input[i+1] & 0x0F) << 2)];
            output[j++] = '=';
        } else {
            output[j++] = chars[((input[i] & 0x03) << 4)];
            output[j++] = '=';
            output[j++] = '=';
        }
    }
    output[j] = '\0';
}

int ws_handshake(SOCKET client_socket, const char *request) {
    (void)client_socket;
    
    const char *key_start = strstr(request, "Sec-WebSocket-Key: ");
    if (!key_start) return -1;
    
    key_start += strlen("Sec-WebSocket-Key: ");
    
    const char *key_end = strstr(key_start, "\r\n");
    if (!key_end) return -1;
    
    char key[256];
    int key_len = key_end - key_start;
    if (key_len >= 256) return -1;
    
    strncpy(key, key_start, key_len);
    key[key_len] = '\0';
    
    while (key_len > 0 && key[key_len-1] == ' ') {
        key[key_len-1] = '\0';
        key_len--;
    }
    
    char combined[512];
    snprintf(combined, sizeof(combined), "%s%s", key, WS_GUID);
    
    unsigned char sha1_result[20];
    real_sha1(combined, (int)strlen(combined), sha1_result);
    
    char accept_key[64];
    base64_encode(sha1_result, 20, accept_key);
    
    char response[512];
    snprintf(response, sizeof(response),
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n"
        "\r\n", accept_key);
    
    send(client_socket, response, (int)strlen(response), 0);
    return 0;
}

int ws_parse_frame(const char *buffer, int buffer_len, char *payload, int payload_size) {
    if (buffer_len < 2) return -1;
    
    unsigned char opcode = buffer[0] & 0x0F;
    int masked = (buffer[1] & 0x80) != 0;
    int payload_len = buffer[1] & 0x7F;
    int offset = 2;
    
    if (payload_len == 126) {
        if (buffer_len < 4) return -1;
        payload_len = ((unsigned char)buffer[2] << 8) | (unsigned char)buffer[3];
        offset = 4;
    } else if (payload_len == 127) {
        if (buffer_len < 10) return -1;
        payload_len = 0;
        for (int i = 0; i < 8; i++) {
            payload_len = (payload_len << 8) | (unsigned char)buffer[2 + i];
        }
        offset = 10;
    }
    
    char mask_key[4] = {0};
    if (masked) {
        if (buffer_len < offset + 4) return -1;
        memcpy(mask_key, buffer + offset, 4);
        offset += 4;
    }
    
    if (buffer_len < offset + payload_len) return -1;
    if (payload_len >= payload_size) return -1;
    
    for (int i = 0; i < payload_len; i++) {
        payload[i] = buffer[offset + i] ^ (masked ? mask_key[i % 4] : 0);
    }
    payload[payload_len] = '\0';
    
    if (opcode == 0x8) return 0;
    
    return payload_len;
}

int ws_create_frame(const char *payload, int payload_len, char *frame, int frame_size) {
    int offset = 0;
    
    frame[offset++] = 0x81;
    
    if (payload_len < 126) {
        frame[offset++] = payload_len;
    } else if (payload_len < 65536) {
        frame[offset++] = 126;
        frame[offset++] = (payload_len >> 8) & 0xFF;
        frame[offset++] = payload_len & 0xFF;
    } else {
        frame[offset++] = 127;
        for (int i = 7; i >= 0; i--) {
            frame[offset++] = (payload_len >> (i * 8)) & 0xFF;
        }
    }
    
    if (offset + payload_len >= frame_size) return -1;
    
    memcpy(frame + offset, payload, payload_len);
    offset += payload_len;
    
    return offset;
}