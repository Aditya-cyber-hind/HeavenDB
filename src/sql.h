#ifndef HEAVENDB_SQL_H
#define HEAVENDB_SQL_H

#include "table.h"

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

#endif // HEAVENDB_SQL_H