#ifndef HEAVENDB_AUTH_H
#define HEAVENDB_AUTH_H

#define MAX_USERS 100
#define MAX_USERNAME 64
#define MAX_PASSWORD 128

typedef struct {
    char username[MAX_USERNAME];
    char password_hash[64]; // Simple hash for now
    int is_active;
} User;

typedef struct {
    User users[MAX_USERS];
    int user_count;
    int current_user_index; // -1 if not logged in
} AuthSystem;

// Creates a new auth system
AuthSystem *auth_create(void);

// Creates a new user
int auth_create_user(AuthSystem *auth, const char *username, const char *password);

// Attempts to login
int auth_login(AuthSystem *auth, const char *username, const char *password);

// Logs out current user
void auth_logout(AuthSystem *auth);

// Checks if a user is logged in
int auth_is_logged_in(AuthSystem *auth);

// Gets the current username
const char *auth_current_user(AuthSystem *auth);

// Frees the auth system
void auth_destroy(AuthSystem *auth);

#endif // HEAVENDB_AUTH_H