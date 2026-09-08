#ifndef HEAVENDB_AUTH_STORAGE_H
#define HEAVENDB_AUTH_STORAGE_H

#include "auth.h"

#define AUTH_FILE "heaven_users.hdb"
#define AUTH_MAGIC 0x41555448 // "AUTH"

// Saves the auth system to disk
int auth_save(AuthSystem *auth);

// Loads the auth system from disk
int auth_load(AuthSystem *auth);

#endif // HEAVENDB_AUTH_STORAGE_H