#ifndef HEAVENDB_SESSION_H
#define HEAVENDB_SESSION_H

#include <time.h>

#define MAX_SESSIONS 64
#define SESSION_TOKEN_BYTES 32
#define SESSION_LIFETIME_SEC 1800   // 30 minutes

typedef struct {
    char token[SESSION_TOKEN_BYTES * 2 + 1];   // hex-encoded, 64 chars + null
    char username[64];
    time_t created_at;
    time_t expires_at;
    int in_use;
} Session;

typedef struct {
    Session sessions[MAX_SESSIONS];
} SessionStore;

SessionStore *session_store_create(void);
void          session_store_destroy(SessionStore *store);

// Creates a session for the given user. Returns a pointer to the
// token string (owned by the store), or NULL on failure.
const char   *session_create(SessionStore *store, const char *username);

// Returns the username if the token is valid and unexpired, or NULL.
// If `username_out` is non-NULL, writes the username there.
int           session_validate(SessionStore *store, const char *token,
                               char *username_out, size_t out_size);

// Removes the session with the given token. Returns 0 on success.
int           session_destroy(SessionStore *store, const char *token);

// Removes expired sessions. Call periodically.
void          session_expire(SessionStore *store);

#endif // HEAVENDB_SESSION_H