#ifndef HEAVENDB_PERMISSIONS_H
#define HEAVENDB_PERMISSIONS_H

#define MAX_PERMISSIONS 256
#define MAX_PERM_USERNAME 64
#define MAX_PERM_TABLE 64

typedef enum {
    PERM_SELECT = 1,
    PERM_INSERT = 2,
    PERM_UPDATE = 4,
    PERM_DELETE = 8,
    PERM_ALL = 15
} PermissionType;

typedef struct {
    char username[MAX_PERM_USERNAME];
    char table_name[MAX_PERM_TABLE];
    int permissions; // Bitmask of PermissionType
} Permission;

typedef struct {
    Permission permissions[MAX_PERMISSIONS];
    int permission_count;
} PermissionSystem;

// Creates a new permission system
PermissionSystem *perm_create(void);

// Grants permissions to a user on a table
int perm_grant(PermissionSystem *perm, const char *username, const char *table_name, int permission_type);

// Revokes permissions from a user on a table
int perm_revoke(PermissionSystem *perm, const char *username, const char *table_name, int permission_type);

// Checks if a user has permission
int perm_check(PermissionSystem *perm, const char *username, const char *table_name, int permission_type);

// Frees the permission system
void perm_destroy(PermissionSystem *perm);

#endif // HEAVENDB_PERMISSIONS_H