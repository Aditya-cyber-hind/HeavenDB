#ifndef HEAVENDB_AUTH_H
#define HEAVENDB_AUTH_H

#define MAX_USERS 100
#define MAX_USERNAME 64
#define MAX_PASSWORD 128
#define MAX_LOGIN_ATTEMPTS 5

typedef struct {
    char username[MAX_USERNAME];
    char password_hash[64];
    int is_active;
    int failed_attempts;
    int is_locked;
    int must_change_password;
} User;

typedef struct {
    User users[MAX_USERS];
    int user_count;
    int current_user_index;
} AuthSystem;

// Creates a new auth system
AuthSystem *auth_create(void);

// Creates a new user with password validation
int auth_create_user(AuthSystem *auth, const char *username, const char *password);

// Attempts to login with lockout tracking
int auth_login(AuthSystem *auth, const char *username, const char *password);

// Changes password for the current user
int auth_change_password(AuthSystem *auth, const char *new_password);

// Logs out current user
void auth_logout(AuthSystem *auth);

// Checks if a user is logged in
int auth_is_logged_in(AuthSystem *auth);

// Gets the current username
const char *auth_current_user(AuthSystem *auth);

// Checks if password meets minimum requirements
int auth_validate_password(const char *password);

// Frees the auth system
void auth_destroy(AuthSystem *auth);

#endif // HEAVENDB_AUTH_H