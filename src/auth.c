#include "auth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// djb2 hash function
static unsigned long simple_hash(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

AuthSystem *auth_create(void) {
    AuthSystem *auth = (AuthSystem*)malloc(sizeof(AuthSystem));
    if (!auth) return NULL;
    
    auth->user_count = 0;
    auth->current_user_index = -1;
    
    // Create default admin user
    strcpy(auth->users[0].username, "admin");
    snprintf(auth->users[0].password_hash, 64, "%lu", simple_hash("admin123"));
    auth->users[0].is_active = 1;
    auth->users[0].failed_attempts = 0;
    auth->users[0].is_locked = 0;
    auth->users[0].must_change_password = 1;
    auth->user_count = 1;
    
    return auth;
}

int auth_validate_password(const char *password) {
    if (!password) return 0;
    
    int len = strlen(password);
    
    // Minimum 8 characters
    if (len < 8) return 0;
    
    // Must contain at least one digit
    int has_digit = 0;
    int has_upper = 0;
    int has_lower = 0;
    
    for (int i = 0; i < len; i++) {
        if (password[i] >= '0' && password[i] <= '9') has_digit = 1;
        else if (password[i] >= 'A' && password[i] <= 'Z') has_upper = 1;
        else if (password[i] >= 'a' && password[i] <= 'z') has_lower = 1;
    }
    
    return has_digit && has_upper && has_lower;
}

int auth_create_user(AuthSystem *auth, const char *username, const char *password) {
    if (!auth || !username || !password) return -1;
    if (auth->user_count >= MAX_USERS) return -1;
    
    // Validate password
    if (!auth_validate_password(password)) {
        printf("ERROR: Password must be at least 8 characters with uppercase, lowercase, and a digit\n");
        return -2;
    }
    
    // Check if user already exists
    for (int i = 0; i < auth->user_count; i++) {
        if (strcmp(auth->users[i].username, username) == 0) {
            printf("ERROR: User '%s' already exists\n", username);
            return -1;
        }
    }
    
    User *new_user = &auth->users[auth->user_count];
    strncpy(new_user->username, username, MAX_USERNAME - 1);
    new_user->username[MAX_USERNAME - 1] = '\0';
    snprintf(new_user->password_hash, 64, "%lu", simple_hash(password));
    new_user->is_active = 1;
    new_user->failed_attempts = 0;
    new_user->is_locked = 0;
    new_user->must_change_password = 0;
    
    auth->user_count++;
    return 0;
}

int auth_login(AuthSystem *auth, const char *username, const char *password) {
    if (!auth || !username || !password) return -1;
    
    for (int i = 0; i < auth->user_count; i++) {
        if (strcmp(auth->users[i].username, username) == 0) {
            
            // Check if locked
            if (auth->users[i].is_locked) {
                printf("ERROR: Account locked. Too many failed attempts.\n");
                return -2;
            }
            
            char hash[64];
            snprintf(hash, 64, "%lu", simple_hash(password));
            
            if (auth->users[i].is_active && 
                strcmp(auth->users[i].password_hash, hash) == 0) {
                // Successful login
                auth->users[i].failed_attempts = 0;
                auth->current_user_index = i;
                return 0;
            } else {
                // Failed login
                auth->users[i].failed_attempts++;
                
                if (auth->users[i].failed_attempts >= MAX_LOGIN_ATTEMPTS) {
                    auth->users[i].is_locked = 1;
                    printf("ERROR: Account locked after %d failed attempts.\n", MAX_LOGIN_ATTEMPTS);
                } else {
                    printf("ERROR: Invalid password. %d attempts remaining.\n", 
                           MAX_LOGIN_ATTEMPTS - auth->users[i].failed_attempts);
                }
                return -1;
            }
        }
    }
    
    printf("ERROR: User '%s' not found\n", username);
    return -1;
}

int auth_change_password(AuthSystem *auth, const char *new_password) {
    if (!auth || !new_password) return -1;
    if (auth->current_user_index < 0) return -1;
    
    // Validate new password
    if (!auth_validate_password(new_password)) {
        printf("ERROR: Password must be at least 8 characters with uppercase, lowercase, and a digit\n");
        return -2;
    }
    
    User *user = &auth->users[auth->current_user_index];
    snprintf(user->password_hash, 64, "%lu", simple_hash(new_password));
    user->must_change_password = 0;
    
    return 0;
}

void auth_logout(AuthSystem *auth) {
    if (auth) {
        auth->current_user_index = -1;
    }
}

int auth_is_logged_in(AuthSystem *auth) {
    return auth && auth->current_user_index >= 0;
}

const char *auth_current_user(AuthSystem *auth) {
    if (!auth || auth->current_user_index < 0) return NULL;
    return auth->users[auth->current_user_index].username;
}

void auth_destroy(AuthSystem *auth) {
    if (auth) {
        free(auth);
    }
}