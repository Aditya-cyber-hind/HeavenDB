#include "auth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simple hash function (djb2)
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
    auth->user_count = 1;
    
    return auth;
}

int auth_create_user(AuthSystem *auth, const char *username, const char *password) {
    if (!auth || !username || !password) return -1;
    if (auth->user_count >= MAX_USERS) return -1;
    
    // Check if user already exists
    for (int i = 0; i < auth->user_count; i++) {
        if (strcmp(auth->users[i].username, username) == 0) {
            return -1; // User exists
        }
    }
    
    User *new_user = &auth->users[auth->user_count];
    strncpy(new_user->username, username, MAX_USERNAME - 1);
    new_user->username[MAX_USERNAME - 1] = '\0';
    snprintf(new_user->password_hash, 64, "%lu", simple_hash(password));
    new_user->is_active = 1;
    
    auth->user_count++;
    return 0;
}

int auth_login(AuthSystem *auth, const char *username, const char *password) {
    if (!auth || !username || !password) return -1;
    
    char hash[64];
    snprintf(hash, 64, "%lu", simple_hash(password));
    
    for (int i = 0; i < auth->user_count; i++) {
        if (auth->users[i].is_active && 
            strcmp(auth->users[i].username, username) == 0 &&
            strcmp(auth->users[i].password_hash, hash) == 0) {
            auth->current_user_index = i;
            return 0;
        }
    }
    
    return -1;
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