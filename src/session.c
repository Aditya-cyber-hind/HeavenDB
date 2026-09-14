#include "session.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <windows.h>
#include <bcrypt.h>

// ==================== HELPERS ====================

static int secure_random_bytes(uint8_t *out, size_t len) {
    return BCryptGenRandom(NULL, out, (ULONG)len,
                           BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0;
}

static void bytes_to_hex(const uint8_t *bytes, size_t len, char *out) {
    static const char hex[] = "0123456789abcdef";
    for (size_t i = 0; i < len; i++) {
        out[i*2]     = hex[(bytes[i] >> 4) & 0x0F];
        out[i*2 + 1] = hex[bytes[i] & 0x0F];
    }
    out[len*2] = '\0';
}

// ==================== LIFECYCLE ====================

SessionStore *session_store_create(void) {
    SessionStore *store = (SessionStore*)malloc(sizeof(SessionStore));
    if (!store) return NULL;
    memset(store->sessions, 0, sizeof(store->sessions));
    return store;
}

void session_store_destroy(SessionStore *store) {
    if (store) free(store);
}

// ==================== CREATE ====================

const char *session_create(SessionStore *store, const char *username) {
    if (!store || !username) return NULL;
    
    // Find a free slot. If none, expire old sessions and retry.
    int slot = -1;
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (!store->sessions[i].in_use) {
            slot = i;
            break;
        }
    }
    
    if (slot < 0) {
        session_expire(store);
        for (int i = 0; i < MAX_SESSIONS; i++) {
            if (!store->sessions[i].in_use) {
                slot = i;
                break;
            }
        }
    }
    
    if (slot < 0) {
        // All slots in use — refuse rather than evict a live session
        return NULL;
    }
    
    Session *s = &store->sessions[slot];
    
    // Generate random token
    uint8_t raw[SESSION_TOKEN_BYTES];
    if (!secure_random_bytes(raw, SESSION_TOKEN_BYTES)) {
        return NULL;
    }
    bytes_to_hex(raw, SESSION_TOKEN_BYTES, s->token);
    
    strncpy(s->username, username, sizeof(s->username) - 1);
    s->username[sizeof(s->username) - 1] = '\0';
    
    s->created_at = time(NULL);
    s->expires_at = s->created_at + SESSION_LIFETIME_SEC;
    s->in_use = 1;
    
    return s->token;
}

// ==================== VALIDATE ====================

int session_validate(SessionStore *store, const char *token,
                     char *username_out, size_t out_size)
{
    if (!store || !token) return 0;
    
    time_t now = time(NULL);
    
    for (int i = 0; i < MAX_SESSIONS; i++) {
        Session *s = &store->sessions[i];
        if (!s->in_use) continue;
        if (strcmp(s->token, token) != 0) continue;
        
        if (now >= s->expires_at) {
            s->in_use = 0;
            return 0;
        }
        
        if (username_out && out_size > 0) {
            strncpy(username_out, s->username, out_size - 1);
            username_out[out_size - 1] = '\0';
        }
        return 1;
    }
    return 0;
}

// ==================== DESTROY ====================

int session_destroy(SessionStore *store, const char *token) {
    if (!store || !token) return -1;
    
    for (int i = 0; i < MAX_SESSIONS; i++) {
        Session *s = &store->sessions[i];
        if (s->in_use && strcmp(s->token, token) == 0) {
            s->in_use = 0;
            return 0;
        }
    }
    return -1;
}

// ==================== EXPIRE ====================

void session_expire(SessionStore *store) {
    if (!store) return;
    time_t now = time(NULL);
    for (int i = 0; i < MAX_SESSIONS; i++) {
        Session *s = &store->sessions[i];
        if (s->in_use && now >= s->expires_at) {
            s->in_use = 0;
        }
    }
}