#ifndef HEAVENDB_SQL_H
#define HEAVENDB_SQL_H

#include "table.h"
#include "auth.h"

// Executes a SQL command. Returns 0 on success, -1 on error.
// Result is printed to stdout directly for now.
int sql_execute(const char *sql);

// Initializes the SQL subsystem and loads tables from disk
void sql_init(void);

// Saves all tables to disk and frees memory
void sql_shutdown(void);

// Saves all tables to disk
int sql_save(void);

// Loads all tables from disk
int sql_load(void);

// Transaction support
int sql_begin(void);
int sql_commit(void);
int sql_rollback(void);
// Set the current logged-in user by name. Used by transport layers after
// validating a session token. Returns 0 on success, -1 if the user isn't found.
int sql_set_current_user(const char *username);

// Returns a pointer to the global auth system (used by transport layers).
AuthSystem *auth_get_system(void);

// Global flag: when set, unauthenticated access is denied (server mode).
// When clear, unauthenticated access is allowed (CLI / single-user mode).
extern int g_require_auth;
#endif // HEAVENDB_SQL_H