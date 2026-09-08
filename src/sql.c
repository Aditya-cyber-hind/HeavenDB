#include "sql.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include "btree.h"
#include "wal.h"

#define MAX_TABLES 128
#define MAX_TOKENS 64
#define MAX_TOKEN_LEN 128
#define SQL_FILE "heaven_sql.hdb"
#define SQL_MAGIC 0x53514C31

static Table *tables[MAX_TABLES];
static int table_count = 0;
static WAL *active_wal = NULL;

static Table *find_table(const char *name);

// ==================== PERSISTENCE ====================

int sql_save(void) {
    FILE *fp = fopen(SQL_FILE, "wb");
    if (!fp) return -1;
    
    uint32_t magic = SQL_MAGIC;
    fwrite(&magic, sizeof(uint32_t), 1, fp);
    
    uint32_t count = (uint32_t)table_count;
    fwrite(&count, sizeof(uint32_t), 1, fp);
    
    for (int t = 0; t < table_count; t++) {
        Table *table = tables[t];
        
        uint32_t name_len = (uint32_t)strlen(table->name);
        fwrite(&name_len, sizeof(uint32_t), 1, fp);
        fwrite(table->name, sizeof(char), name_len, fp);
        
        uint32_t col_count = (uint32_t)table->column_count;
        fwrite(&col_count, sizeof(uint32_t), 1, fp);
        
        for (int c = 0; c < table->column_count; c++) {
            uint32_t col_name_len = (uint32_t)strlen(table->columns[c].name);
            fwrite(&col_name_len, sizeof(uint32_t), 1, fp);
            fwrite(table->columns[c].name, sizeof(char), col_name_len, fp);
            
            uint32_t col_type = (uint32_t)table->columns[c].type;
            fwrite(&col_type, sizeof(uint32_t), 1, fp);
        }
        
        uint32_t row_count = (uint32_t)table->row_count;
        fwrite(&row_count, sizeof(uint32_t), 1, fp);
        
        for (size_t r = 0; r < table->row_count; r++) {
            void **row = (void**)table->rows[r];
            for (int c = 0; c < table->column_count; c++) {
                switch (table->columns[c].type) {
                    case TYPE_INTEGER: {
                        int val = *(int*)row[c];
                        fwrite(&val, sizeof(int), 1, fp);
                        break;
                    }
                    case TYPE_TEXT: {
                        char *str = (char*)row[c];
                        uint32_t str_len = (uint32_t)strlen(str);
                        fwrite(&str_len, sizeof(uint32_t), 1, fp);
                        fwrite(str, sizeof(char), str_len, fp);
                        break;
                    }
                    case TYPE_FLOAT: {
                        double val = *(double*)row[c];
                        fwrite(&val, sizeof(double), 1, fp);
                        break;
                    }
                }
            }
        }
    }
    
    fclose(fp);
    return 0;
}

int sql_load(void) {
    FILE *fp = fopen(SQL_FILE, "rb");
    if (!fp) return 0;
    
    uint32_t magic;
    if (fread(&magic, sizeof(uint32_t), 1, fp) != 1) {
        fclose(fp);
        return -1;
    }
    
    if (magic != SQL_MAGIC) {
        fclose(fp);
        return -1;
    }
    
    uint32_t count;
    if (fread(&count, sizeof(uint32_t), 1, fp) != 1) {
        fclose(fp);
        return -1;
    }
    
    for (uint32_t t = 0; t < count; t++) {
        uint32_t name_len;
        if (fread(&name_len, sizeof(uint32_t), 1, fp) != 1) {
            fclose(fp);
            return -1;
        }
        
        char *name = (char*)malloc(name_len + 1);
        if (!name) {
            fclose(fp);
            return -1;
        }
        if (fread(name, sizeof(char), name_len, fp) != name_len) {
            free(name);
            fclose(fp);
            return -1;
        }
        name[name_len] = '\0';
        
        Table *table = table_create(name);
        free(name);
        if (!table) {
            fclose(fp);
            return -1;
        }
        
        uint32_t col_count;
        if (fread(&col_count, sizeof(uint32_t), 1, fp) != 1) {
            table_destroy(table);
            fclose(fp);
            return -1;
        }
        
        for (uint32_t c = 0; c < col_count; c++) {
            uint32_t col_name_len;
            if (fread(&col_name_len, sizeof(uint32_t), 1, fp) != 1) {
                table_destroy(table);
                fclose(fp);
                return -1;
            }
            
            char *col_name = (char*)malloc(col_name_len + 1);
            if (!col_name) {
                table_destroy(table);
                fclose(fp);
                return -1;
            }
            if (fread(col_name, sizeof(char), col_name_len, fp) != col_name_len) {
                free(col_name);
                table_destroy(table);
                fclose(fp);
                return -1;
            }
            col_name[col_name_len] = '\0';
            
            uint32_t col_type;
            if (fread(&col_type, sizeof(uint32_t), 1, fp) != 1) {
                free(col_name);
                table_destroy(table);
                fclose(fp);
                return -1;
            }
            
            table_add_column(table, col_name, (ColumnType)col_type);
            free(col_name);
        }
        
        uint32_t row_count;
        if (fread(&row_count, sizeof(uint32_t), 1, fp) != 1) {
            table_destroy(table);
            fclose(fp);
            return -1;
        }
        
        for (uint32_t r = 0; r < row_count; r++) {
            void **values = (void**)malloc(table->column_count * sizeof(void*));
            if (!values) {
                table_destroy(table);
                fclose(fp);
                return -1;
            }
            
            for (uint32_t c = 0; c < table->column_count; c++) {
                switch (table->columns[c].type) {
                    case TYPE_INTEGER: {
                        int *val = (int*)malloc(sizeof(int));
                        if (!val) {
                            free(values);
                            table_destroy(table);
                            fclose(fp);
                            return -1;
                        }
                        if (fread(val, sizeof(int), 1, fp) != 1) {
                            free(val);
                            free(values);
                            table_destroy(table);
                            fclose(fp);
                            return -1;
                        }
                        values[c] = val;
                        break;
                    }
                    case TYPE_TEXT: {
                        uint32_t str_len;
                        if (fread(&str_len, sizeof(uint32_t), 1, fp) != 1) {
                            free(values);
                            table_destroy(table);
                            fclose(fp);
                            return -1;
                        }
                        char *str = (char*)malloc(str_len + 1);
                        if (!str) {
                            free(values);
                            table_destroy(table);
                            fclose(fp);
                            return -1;
                        }
                        if (fread(str, sizeof(char), str_len, fp) != str_len) {
                            free(str);
                            free(values);
                            table_destroy(table);
                            fclose(fp);
                            return -1;
                        }
                        str[str_len] = '\0';
                        values[c] = str;
                        break;
                    }
                    case TYPE_FLOAT: {
                        double *val = (double*)malloc(sizeof(double));
                        if (!val) {
                            free(values);
                            table_destroy(table);
                            fclose(fp);
                            return -1;
                        }
                        if (fread(val, sizeof(double), 1, fp) != 1) {
                            free(val);
                            free(values);
                            table_destroy(table);
                            fclose(fp);
                            return -1;
                        }
                        values[c] = val;
                        break;
                    }
                }
            }
            
            if (table->row_count >= table->row_capacity) {
                size_t new_capacity = table->row_capacity == 0 ? 16 : table->row_capacity * 2;
                void **new_rows = (void**)realloc(table->rows, new_capacity * sizeof(void*));
                if (!new_rows) {
                    for (uint32_t c = 0; c < table->column_count; c++) {
                        free(values[c]);
                    }
                    free(values);
                    table_destroy(table);
                    fclose(fp);
                    return -1;
                }
                table->rows = new_rows;
                table->row_capacity = new_capacity;
            }
            table->rows[table->row_count] = values;
            table->row_count++;
        }
        
        tables[table_count++] = table;
    }
    
    fclose(fp);
    return 0;
}

// ==================== TRANSACTIONS ====================

int sql_begin(void) {
    if (!active_wal) {
        active_wal = wal_create();
        if (!active_wal) return -1;
    }
    return wal_begin(active_wal);
}

int sql_commit(void) {
    if (!active_wal) return -1;
    
    // Apply pending WAL entries to tables
    WALEntry *entry = active_wal->head;
    while (entry) {
        if (entry->type == WAL_INSERT) {
            Table *table = find_table(entry->table_name);
            if (table) {
                void **values = (void**)malloc(table->column_count * sizeof(void*));
                
                for (int i = 0; i < table->column_count; i++) {
                    char *str = (char*)entry->values[i];
                    
                    switch (table->columns[i].type) {
                        case TYPE_INTEGER: {
                            int *val = (int*)malloc(sizeof(int));
                            *val = atoi(str);
                            values[i] = val;
                            break;
                        }
                        case TYPE_TEXT: {
                            char *copy = (char*)malloc(strlen(str) + 1);
                            strcpy(copy, str);
                            values[i] = copy;
                            break;
                        }
                        case TYPE_FLOAT: {
                            double *val = (double*)malloc(sizeof(double));
                            *val = atof(str);
                            values[i] = val;
                            break;
                        }
                    }
                }
                
                table_insert(table, values);
                
                for (int i = 0; i < table->column_count; i++) {
                    free(values[i]);
                }
                free(values);
            }
        }
        entry = entry->next;
    }
    
    int result = wal_commit(active_wal);
    wal_destroy(active_wal);
    active_wal = NULL;
    return result;
}

int sql_rollback(void) {
    if (!active_wal) return -1;
    int result = wal_rollback(active_wal);
    wal_destroy(active_wal);
    active_wal = NULL;
    return result;
}

// ==================== TOKENIZER ====================

typedef struct {
    char tokens[MAX_TOKENS][MAX_TOKEN_LEN];
    int count;
} TokenList;

static void tokenize(const char *sql, TokenList *list) {
    list->count = 0;
    int i = 0;
    int len = strlen(sql);
    
    while (i < len && list->count < MAX_TOKENS) {
        while (i < len && isspace(sql[i])) i++;
        if (i >= len) break;
        
        int j = 0;
        
        if (sql[i] == '\'') {
            i++;
            while (i < len && sql[i] != '\'' && j < MAX_TOKEN_LEN - 1) {
                list->tokens[list->count][j++] = sql[i++];
            }
            i++;
        }
        else if (sql[i] == ',' || sql[i] == '(' || sql[i] == ')' || 
                 sql[i] == ';' || sql[i] == '*' || sql[i] == '=' ||
                 sql[i] == '>' || sql[i] == '<') {
            list->tokens[list->count][j++] = sql[i++];
        }
        else {
            while (i < len && !isspace(sql[i]) && 
                   sql[i] != ',' && sql[i] != '(' && sql[i] != ')' && 
                   sql[i] != ';' && sql[i] != '=' && sql[i] != '>' && 
                   sql[i] != '<' && j < MAX_TOKEN_LEN - 1) {
                list->tokens[list->count][j++] = sql[i++];
            }
        }
        
        list->tokens[list->count][j] = '\0';
        if (j > 0) {
            list->count++;
        }
    }
}

static void to_upper(char *str) {
    while (*str) {
        *str = toupper((unsigned char)*str);
        str++;
    }
}

static Table *find_table(const char *name) {
    for (int i = 0; i < table_count; i++) {
        if (strcmp(tables[i]->name, name) == 0) {
            return tables[i];
        }
    }
    return NULL;
}

// ==================== PRINT TABLE ====================

static void print_table(Table *table, int *column_indices, int col_count) {
    if (!table) return;
    
    for (int i = 0; i < col_count; i++) {
        int idx = column_indices[i];
        printf("%-15s ", table->columns[idx].name);
    }
    printf("\n");
    
    for (int i = 0; i < col_count; i++) {
        printf("%-15s ", "---------------");
    }
    printf("\n");
    
    for (size_t r = 0; r < table->row_count; r++) {
        void **row = (void**)table->rows[r];
        for (int i = 0; i < col_count; i++) {
            int idx = column_indices[i];
            switch (table->columns[idx].type) {
                case TYPE_INTEGER:
                    printf("%-15d ", *(int*)row[idx]);
                    break;
                case TYPE_TEXT:
                    printf("%-15s ", (char*)row[idx]);
                    break;
                case TYPE_FLOAT:
                    printf("%-15.2f ", *(double*)row[idx]);
                    break;
            }
        }
        printf("\n");
    }
    printf("(%zu row%s)\n\n", table->row_count, table->row_count == 1 ? "" : "s");
}

// ==================== HANDLERS ====================

static int handle_create_table(TokenList *tokens) {
    if (tokens->count < 5) return -1;
    
    char table_name[MAX_TABLE_NAME];
    strcpy(table_name, tokens->tokens[2]);
    
    if (find_table(table_name)) {
        printf("ERROR: Table '%s' already exists\n", table_name);
        return -1;
    }
    
    Table *table = table_create(table_name);
    if (!table) return -1;
    
    int i = 4;
    
    while (i < tokens->count && strcmp(tokens->tokens[i], ")") != 0) {
        char col_name[64];
        char col_type[32];
        
        strcpy(col_name, tokens->tokens[i]);
        strcpy(col_type, tokens->tokens[i + 1]);
        to_upper(col_type);
        
        ColumnType type;
        if (strcmp(col_type, "INTEGER") == 0 || strcmp(col_type, "INT") == 0) {
            type = TYPE_INTEGER;
        } else if (strcmp(col_type, "TEXT") == 0 || strcmp(col_type, "STRING") == 0) {
            type = TYPE_TEXT;
        } else if (strcmp(col_type, "FLOAT") == 0 || strcmp(col_type, "DOUBLE") == 0) {
            type = TYPE_FLOAT;
        } else {
            printf("ERROR: Unknown type '%s'\n", col_type);
            table_destroy(table);
            return -1;
        }
        
        table_add_column(table, col_name, type);
        i += 2;
        
        if (i < tokens->count && strcmp(tokens->tokens[i], ",") == 0) {
            i++;
        }
    }
    
    tables[table_count++] = table;
    printf("OK. Created table '%s'\n", table_name);
    return 0;
}

static int handle_insert(TokenList *tokens) {
    if (tokens->count < 6) return -1;
    
    char table_name[MAX_TABLE_NAME];
    strcpy(table_name, tokens->tokens[2]);
    
    Table *table = find_table(table_name);
    if (!table) {
        printf("ERROR: Table '%s' not found\n", table_name);
        return -1;
    }
    
    int i = 5;
    
    void **values = (void**)malloc(table->column_count * sizeof(void*));
    if (!values) return -1;
    
    int col_idx = 0;
    while (i < tokens->count && col_idx < table->column_count) {
        if (strcmp(tokens->tokens[i], ")") == 0) break;
        if (strcmp(tokens->tokens[i], ",") == 0) {
            i++;
            continue;
        }
        
        switch (table->columns[col_idx].type) {
            case TYPE_INTEGER: {
                int *val = (int*)malloc(sizeof(int));
                *val = atoi(tokens->tokens[i]);
                values[col_idx] = val;
                break;
            }
            case TYPE_TEXT: {
                char *str = (char*)malloc(strlen(tokens->tokens[i]) + 1);
                strcpy(str, tokens->tokens[i]);
                values[col_idx] = str;
                break;
            }
            case TYPE_FLOAT: {
                double *val = (double*)malloc(sizeof(double));
                *val = atof(tokens->tokens[i]);
                values[col_idx] = val;
                break;
            }
        }
        
        col_idx++;
        i++;
    }
    
    if (col_idx != table->column_count) {
        printf("ERROR: Expected %d values, got %d\n", table->column_count, col_idx);
        for (int j = 0; j < col_idx; j++) free(values[j]);
        free(values);
        return -1;
    }
    
    // If in a transaction, log to WAL instead of direct insert
    if (active_wal && active_wal->in_transaction) {
        // For simplicity, we convert values to strings for WAL
        char **str_values = (char**)malloc(table->column_count * sizeof(char*));
        for (int j = 0; j < table->column_count; j++) {
            char buf[128];
            switch (table->columns[j].type) {
                case TYPE_INTEGER:
                    snprintf(buf, sizeof(buf), "%d", *(int*)values[j]);
                    break;
                case TYPE_TEXT:
                    snprintf(buf, sizeof(buf), "%s", (char*)values[j]);
                    break;
                case TYPE_FLOAT:
                    snprintf(buf, sizeof(buf), "%f", *(double*)values[j]);
                    break;
            }
            str_values[j] = (char*)malloc(strlen(buf) + 1);
            strcpy(str_values[j], buf);
        }
        
        wal_log_insert(active_wal, table_name, (void**)str_values, table->column_count);
        
        for (int j = 0; j < table->column_count; j++) free(str_values[j]);
        free(str_values);
        
        printf("OK. Inserted 1 row (pending commit)\n");
    } else {
        table_insert(table, values);
        printf("OK. Inserted 1 row\n");
    }
    
    for (int j = 0; j < table->column_count; j++) free(values[j]);
    free(values);
    
    return 0;
}

static int handle_select(TokenList *tokens) {
    if (tokens->count < 4) return -1;
    
    int from_idx = -1;
    for (int i = 1; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "FROM") == 0) {
            from_idx = i;
            break;
        }
    }
    
    if (from_idx == -1) return -1;
    
    char table_name[MAX_TABLE_NAME];
    strcpy(table_name, tokens->tokens[from_idx + 1]);
    
    Table *table = find_table(table_name);
    if (!table) {
        printf("ERROR: Table '%s' not found\n", table_name);
        return -1;
    }
    
    int column_indices[MAX_COLUMNS];
    int col_count = 0;
    
    if (strcmp(tokens->tokens[1], "*") == 0) {
        for (int i = 0; i < table->column_count; i++) {
            column_indices[col_count++] = i;
        }
    } else {
        int i = 1;
        while (i < from_idx) {
            int idx = table_get_column_index(table, tokens->tokens[i]);
            if (idx == -1) {
                printf("ERROR: Column '%s' not found\n", tokens->tokens[i]);
                return -1;
            }
            column_indices[col_count++] = idx;
            i++;
            if (i < from_idx && strcmp(tokens->tokens[i], ",") == 0) i++;
        }
    }
    
    // No WHERE clause
    if (from_idx + 2 >= tokens->count || strcmp(tokens->tokens[from_idx + 2], "WHERE") != 0) {
        print_table(table, column_indices, col_count);
        return 0;
    }
    
    // WHERE clause
    char col_name[64];
    char op[4];
    char value[128];
    
    strcpy(col_name, tokens->tokens[from_idx + 3]);
    strcpy(op, tokens->tokens[from_idx + 4]);
    strcpy(value, tokens->tokens[from_idx + 5]);
    
    int col_idx = table_get_column_index(table, col_name);
    if (col_idx == -1) {
        printf("ERROR: Column '%s' not found\n", col_name);
        return -1;
    }
    
    for (int i = 0; i < col_count; i++) {
        printf("%-15s ", table->columns[column_indices[i]].name);
    }
    printf("\n");
    for (int i = 0; i < col_count; i++) printf("%-15s ", "---------------");
    printf("\n");
    
    int match_count = 0;
    int cond_val = atoi(value);
    
    for (size_t r = 0; r < table->row_count; r++) {
        void **row = (void**)table->rows[r];
        
        int matches = 0;
        if (table->columns[col_idx].type == TYPE_INTEGER) {
            int row_val = *(int*)row[col_idx];
            
            if (strcmp(op, ">") == 0 && row_val > cond_val) matches = 1;
            else if (strcmp(op, "<") == 0 && row_val < cond_val) matches = 1;
            else if (strcmp(op, "=") == 0 && row_val == cond_val) matches = 1;
            else if (strcmp(op, ">=") == 0 && row_val >= cond_val) matches = 1;
            else if (strcmp(op, "<=") == 0 && row_val <= cond_val) matches = 1;
        } else if (table->columns[col_idx].type == TYPE_TEXT) {
            char *row_val = (char*)row[col_idx];
            if (strcmp(op, "=") == 0 && strcmp(row_val, value) == 0) matches = 1;
        }
        
        if (matches) {
            for (int i = 0; i < col_count; i++) {
                int idx = column_indices[i];
                switch (table->columns[idx].type) {
                    case TYPE_INTEGER:
                        printf("%-15d ", *(int*)row[idx]);
                        break;
                    case TYPE_TEXT:
                        printf("%-15s ", (char*)row[idx]);
                        break;
                    case TYPE_FLOAT:
                        printf("%-15.2f ", *(double*)row[idx]);
                        break;
                }
            }
            printf("\n");
            match_count++;
        }
    }
    printf("(%d row%s)\n\n", match_count, match_count == 1 ? "" : "s");
    
    return 0;
}

// ==================== INIT/SHUTDOWN ====================

void sql_init(void) {
    table_count = 0;
    sql_load();
    printf("HeavenDB SQL Engine initialized.\n");
}

void sql_shutdown(void) {
    sql_save();
    for (int i = 0; i < table_count; i++) {
        table_destroy(tables[i]);
    }
    table_count = 0;
    if (active_wal) {
        wal_destroy(active_wal);
        active_wal = NULL;
    }
}

int sql_execute(const char *sql) {
    TokenList list;
    tokenize(sql, &list);
    
    if (list.count == 0) return 0;
    
    char command[MAX_TOKEN_LEN];
    strcpy(command, list.tokens[0]);
    to_upper(command);
    
    if (strcmp(command, "BEGIN") == 0) {
        if (sql_begin() == 0) {
            printf("OK. Transaction started\n");
        } else {
            printf("ERROR: Already in transaction\n");
        }
        return 0;
    }
    else if (strcmp(command, "COMMIT") == 0) {
        if (sql_commit() == 0) {
            printf("OK. Transaction committed\n");
        } else {
            printf("ERROR: No active transaction\n");
        }
        return 0;
    }
    else if (strcmp(command, "ROLLBACK") == 0) {
        if (sql_rollback() == 0) {
            printf("OK. Transaction rolled back\n");
        } else {
            printf("ERROR: No active transaction\n");
        }
        return 0;
    }
    else if (strcmp(command, "CREATE") == 0) {
        char second[MAX_TOKEN_LEN];
        strcpy(second, list.tokens[1]);
        to_upper(second);
        
        if (strcmp(second, "TABLE") == 0) {
            return handle_create_table(&list);
        }
    }
    else if (strcmp(command, "INSERT") == 0) {
        return handle_insert(&list);
    }
    else if (strcmp(command, "SELECT") == 0) {
        return handle_select(&list);
    }
    else {
        printf("ERROR: Unknown SQL command '%s'\n", command);
        return -1;
    }
    
    return 0;
}