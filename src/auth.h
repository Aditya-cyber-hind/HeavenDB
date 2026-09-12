#ifndef HEAVENDB_AUTH_H
#define HEAVENDB_AUTH_H

#include <stddef.h>
#include <stdint.h>

#define MAX_USERS 100
#define MAX_USERNAME 64
#define MAX_PASSWORD 128
#define MAX_LOGIN_ATTEMPTS 5
#define PBKDF2_ITERATIONS 100000
#define SALT_SIZE 16
#define HASH_SIZE 32

typedef struct {
    char username[MAX_USERNAME];
    uint8_t salt[SALT_SIZE];
    uint8_t password_hash[HASH_SIZE];
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

AuthSystem *auth_create(void);
int auth_create_user(AuthSystem *auth, const char *username, const char *password);
int auth_login(AuthSystem *auth, const char *username, const char *password);
int auth_change_password(AuthSystem *auth, const char *new_password);
void auth_logout(AuthSystem *auth);
int auth_is_logged_in(AuthSystem *auth);
const char *auth_current_user(AuthSystem *auth);
int auth_validate_password(const char *password);
void auth_destroy(AuthSystem *auth);

#endif // HEAVENDB_AUTH_H