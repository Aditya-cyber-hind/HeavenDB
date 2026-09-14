#include "permissions.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

PermissionSystem *perm_create(void) {
    PermissionSystem *perm = (PermissionSystem*)malloc(sizeof(PermissionSystem));
    if (!perm) return NULL;
    perm->permission_count = 0;
    return perm;
}

int perm_grant(PermissionSystem *perm, const char *username, const char *table_name, int permission_type) {
    if (!perm || !username || !table_name) return -1;
    if (perm->permission_count >= MAX_PERMISSIONS) return -1;
    
    // Check if permission already exists for this user/table
    for (int i = 0; i < perm->permission_count; i++) {
        if (strcmp(perm->permissions[i].username, username) == 0 &&
            strcmp(perm->permissions[i].table_name, table_name) == 0) {
            // Add to existing permissions
            perm->permissions[i].permissions |= permission_type;
            return 0;
        }
    }
    
    // Create new permission entry
    Permission *p = &perm->permissions[perm->permission_count];
    strncpy(p->username, username, MAX_PERM_USERNAME - 1);
    p->username[MAX_PERM_USERNAME - 1] = '\0';
    strncpy(p->table_name, table_name, MAX_PERM_TABLE - 1);
    p->table_name[MAX_PERM_TABLE - 1] = '\0';
    p->permissions = permission_type;
    
    perm->permission_count++;
    return 0;
}

int perm_revoke(PermissionSystem *perm, const char *username, const char *table_name, int permission_type) {
    if (!perm || !username || !table_name) return -1;
    
    for (int i = 0; i < perm->permission_count; i++) {
        if (strcmp(perm->permissions[i].username, username) == 0 &&
            strcmp(perm->permissions[i].table_name, table_name) == 0) {
            // Remove the permission bit
            perm->permissions[i].permissions &= ~permission_type;
            return 0;
        }
    }
    
    return -1;
}

int perm_check(PermissionSystem *perm, const char *username, const char *table_name, int permission_type) {
    if (!perm || !username || !table_name) return 0;
    
    for (int i = 0; i < perm->permission_count; i++) {
        if (strcmp(perm->permissions[i].username, username) == 0 &&
            strcmp(perm->permissions[i].table_name, table_name) == 0) {
            return (perm->permissions[i].permissions & permission_type) != 0;
        }
    }
    
    return 0;
}

void perm_destroy(PermissionSystem *perm) {
    if (perm) {
        free(perm);
    }
}