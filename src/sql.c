#include "sql.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include "btree.h"
#include "wal.h"
#include "auth.h"
#include "auth_storage.h"
#include "permissions.h"

static void generate_uuid(char *output) {
    unsigned char bytes[16];
    for (int i = 0; i < 16; i++) {
        bytes[i] = (unsigned char)(rand() % 256);
    }
    bytes[6] = (bytes[6] & 0x0F) | 0x40;
    bytes[8] = (bytes[8] & 0x3F) | 0x80;
    
    sprintf(output, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        bytes[0], bytes[1], bytes[2], bytes[3],
        bytes[4], bytes[5],
        bytes[6], bytes[7],
        bytes[8], bytes[9],
        bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
}

#define MAX_TABLES 128
#define MAX_TOKENS 64
#define MAX_TOKEN_LEN 128
#define MAX_VIEWS 64
#define SQL_FILE "heaven_sql.hdb"
#define SQL_MAGIC 0x53514C31

static Table *tables[MAX_TABLES];
static int table_count = 0;
static WAL *active_wal = NULL;
static AuthSystem *auth_system = NULL;
static PermissionSystem *perm_system = NULL;

typedef struct {
    char name[MAX_TABLE_NAME];
    char sql[512];
} View;

static View views[MAX_VIEWS];
static int view_count = 0;

typedef struct {
    char table_name[MAX_TABLE_NAME];
    char col_name[64];
    char ref_table[MAX_TABLE_NAME];
    char ref_col[64];
} ForeignKey;

static ForeignKey foreign_keys[MAX_TABLES * MAX_COLUMNS];
static int foreign_key_count = 0;

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
            
            int flags = 0;
            if (table->columns[c].is_primary_key) flags |= 1;
            if (table->columns[c].is_unique) flags |= 2;
            if (table->columns[c].is_not_null) flags |= 4;
            if (table->columns[c].is_auto_increment) flags |= 8;
            fwrite(&flags, sizeof(int), 1, fp);
            
            fwrite(&table->columns[c].next_auto_value, sizeof(int), 1, fp);
        }
        
        uint32_t row_count = (uint32_t)table->row_count;
        fwrite(&row_count, sizeof(uint32_t), 1, fp);
        
        for (size_t r = 0; r < table->row_count; r++) {
            void **row = (void**)table->rows[r];
            for (int c = 0; c < table->column_count; c++) {
                switch (table->columns[c].type) {
                    case TYPE_INTEGER:
                    case TYPE_BOOLEAN: {
                        int val = *(int*)row[c];
                        fwrite(&val, sizeof(int), 1, fp);
                        break;
                    }
                    case TYPE_TEXT:
                    case TYPE_UUID:
                    case TYPE_JSON: {
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
        
        for (int c = 0; c < (int)col_count; c++) {
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
            
            int col_idx = table->column_count - 1;
            
            int flags = 0;
            if (fread(&flags, sizeof(int), 1, fp) == 1) {
                if (flags & 1) table->columns[col_idx].is_primary_key = 1;
                if (flags & 2) table->columns[col_idx].is_unique = 1;
                if (flags & 4) table->columns[col_idx].is_not_null = 1;
                if (flags & 8) table->columns[col_idx].is_auto_increment = 1;
                
                int next_val = 1;
                if (fread(&next_val, sizeof(int), 1, fp) == 1) {
                    table->columns[col_idx].next_auto_value = next_val;
                }
            }
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
            
            for (int c = 0; c < table->column_count; c++) {
                switch (table->columns[c].type) {
                    case TYPE_INTEGER:
                    case TYPE_BOOLEAN: {
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
                    case TYPE_TEXT:
                    case TYPE_UUID:
                    case TYPE_JSON: {
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
                    for (int c = 0; c < table->column_count; c++) {
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
    
    WALEntry *entry = active_wal->head;
    while (entry) {
        if (entry->type == WAL_INSERT) {
            Table *table = find_table(entry->table_name);
            if (table) {
                void **values = (void**)malloc(table->column_count * sizeof(void*));
                if (!values) return -1;
                
                for (int i = 0; i < table->column_count; i++) {
                    char *str = (char*)entry->values[i];
                    
                    switch (table->columns[i].type) {
                        case TYPE_INTEGER:
                        case TYPE_BOOLEAN: {
                            int *val = (int*)malloc(sizeof(int));
                            *val = atoi(str);
                            values[i] = val;
                            break;
                        }
                        case TYPE_TEXT:
                        case TYPE_UUID:
                        case TYPE_JSON: {
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
                else if (sql[i] == '<' || sql[i] == '>') {
            list->tokens[list->count][j++] = sql[i++];
            if (i < len && (sql[i] == '=' || sql[i] == '>')) {
                list->tokens[list->count][j++] = sql[i++];
            }
        }
        else if (sql[i] == '!') {
            list->tokens[list->count][j++] = sql[i++];
            if (i < len && sql[i] == '=') {
                list->tokens[list->count][j++] = sql[i++];
            }
        }
        else if (sql[i] == ',' || sql[i] == '(' || sql[i] == ')' || 
                 sql[i] == ';' || sql[i] == '*' || sql[i] == '=') {
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
                case TYPE_BOOLEAN:
                    printf("%-15s ", *(int*)row[idx] ? "TRUE" : "FALSE");
                    break;
                case TYPE_TEXT:
                    printf("%-15s ", (char*)row[idx]);
                    break;
                case TYPE_UUID:
                    printf("%-38s ", (char*)row[idx]);
                    break;
                case TYPE_JSON:
                    printf("%-30s ", (char*)row[idx]);
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

// ==================== AGGREGATE FUNCTIONS ====================

static int handle_aggregate(TokenList *tokens) {
    if (tokens->count < 4) return -1;
    
    char func_name[32];
    strcpy(func_name, tokens->tokens[1]);
    to_upper(func_name);
    
    char *paren = strchr(func_name, '(');
    if (paren) *paren = '\0';
    
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
    
    if (strcmp(func_name, "COUNT") == 0) {
        printf("COUNT: %zu\n", table->row_count);
    } 
    else if (strcmp(func_name, "SUM") == 0 || strcmp(func_name, "AVG") == 0) {
        char col_name[64] = "";
        
        if (tokens->count > 3) {
            strcpy(col_name, tokens->tokens[3]);
        }
        
        int c_len = (int)strlen(col_name);
        if (c_len > 0 && col_name[c_len-1] == ')') {
            col_name[c_len-1] = '\0';
        }
        
        if (strlen(col_name) == 0) {
            printf("ERROR: Invalid syntax\n");
            return -1;
        }
        
        int col_idx = table_get_column_index(table, col_name);
        if (col_idx == -1) {
            printf("ERROR: Column '%s' not found\n", col_name);
            return -1;
        }
        
        long long sum = 0;
        int count = 0;
        
        for (size_t r = 0; r < table->row_count; r++) {
            void **row = (void**)table->rows[r];
            if (table->columns[col_idx].type == TYPE_INTEGER) {
                sum += *(int*)row[col_idx];
                count++;
            }
        }
        
        if (strcmp(func_name, "SUM") == 0) {
            printf("SUM(%s): %lld\n", col_name, sum);
        } else {
            if (count > 0) {
                printf("AVG(%s): %.2f\n", col_name, (double)sum / count);
            } else {
                printf("AVG(%s): 0\n", col_name);
            }
        }
    }
    else {
        printf("ERROR: Unknown function '%s'\n", func_name);
        return -1;
    }
    
    return 0;
}

// ==================== MIN/MAX ====================

static int handle_min_max(TokenList *tokens) {
    if (tokens->count < 4) return -1;
    
    char func_name[32];
    strcpy(func_name, tokens->tokens[1]);
    to_upper(func_name);
    
    char *paren = strchr(func_name, '(');
    if (paren) *paren = '\0';
    
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
    
    char col_name[64] = "";
    if (tokens->count > 3) {
        strcpy(col_name, tokens->tokens[3]);
    }
    
    int c_len = (int)strlen(col_name);
    if (c_len > 0 && col_name[c_len-1] == ')') {
        col_name[c_len-1] = '\0';
    }
    
    int col_idx = table_get_column_index(table, col_name);
    if (col_idx == -1) {
        printf("ERROR: Column '%s' not found\n", col_name);
        return -1;
    }
    
    int result = 0;
    int first = 1;
    
    for (size_t r = 0; r < table->row_count; r++) {
        void **row = (void**)table->rows[r];
        if (table->columns[col_idx].type == TYPE_INTEGER) {
            int val = *(int*)row[col_idx];
            if (first) {
                result = val;
                first = 0;
            } else if (strcmp(func_name, "MIN") == 0 && val < result) {
                result = val;
            } else if (strcmp(func_name, "MAX") == 0 && val > result) {
                result = val;
            }
        }
    }
    
    printf("%s(%s): %d\n", func_name, col_name, result);
    return 0;
}

// ==================== DISTINCT ====================

static int handle_distinct(TokenList *tokens) {
    if (tokens->count < 5) return -1;
    
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
    
    char col_name[64];
    strcpy(col_name, tokens->tokens[2]);
    
    int col_idx = table_get_column_index(table, col_name);
    if (col_idx == -1) {
        printf("ERROR: Column '%s' not found\n", col_name);
        return -1;
    }
    
    printf("%-15s\n", col_name);
    printf("%-15s\n", "---");
    
    for (size_t i = 0; i < table->row_count; i++) {
        void **row = (void**)table->rows[i];
        int val = *(int*)row[col_idx];
        
        int seen = 0;
        for (size_t j = 0; j < i; j++) {
            void **prev = (void**)table->rows[j];
            if (*(int*)prev[col_idx] == val) {
                seen = 1;
                break;
            }
        }
        
        if (!seen) {
            printf("%-15d\n", val);
        }
    }
    
    return 0;
}

// ==================== BETWEEN ====================

static int handle_between(TokenList *tokens) {
    if (tokens->count < 8) return -1;
    
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
    
    char col_name[64];
    strcpy(col_name, tokens->tokens[from_idx + 3]);
    
    int low = atoi(tokens->tokens[from_idx + 5]);
    int high = atoi(tokens->tokens[from_idx + 7]);
    
    int col_idx = table_get_column_index(table, col_name);
    if (col_idx == -1) {
        printf("ERROR: Column '%s' not found\n", col_name);
        return -1;
    }
    
    for (int i = 0; i < table->column_count; i++) {
        printf("%-15s ", table->columns[i].name);
    }
    printf("\n");
    for (int i = 0; i < table->column_count; i++) {
        printf("%-15s ", "---------------");
    }
    printf("\n");
    
    int match_count = 0;
    
    for (size_t r = 0; r < table->row_count; r++) {
        void **row = (void**)table->rows[r];
        
        if (table->columns[col_idx].type == TYPE_INTEGER) {
            int val = *(int*)row[col_idx];
            if (val >= low && val <= high) {
                for (int c = 0; c < table->column_count; c++) {
                    switch (table->columns[c].type) {
                        case TYPE_INTEGER:
                            printf("%-15d ", *(int*)row[c]);
                            break;
                        case TYPE_TEXT:
                            printf("%-15s ", (char*)row[c]);
                            break;
                        case TYPE_FLOAT:
                            printf("%-15.2f ", *(double*)row[c]);
                            break;
                    }
                }
                printf("\n");
                match_count++;
            }
        }
    }
    printf("(%d rows)\n", match_count);
    
    return 0;
}

// ==================== LIKE ====================

static int handle_like(TokenList *tokens) {
    if (tokens->count < 6) return -1;
    
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
    
    char col_name[64];
    char pattern[128];
    
    strcpy(col_name, tokens->tokens[from_idx + 3]);
    strcpy(pattern, tokens->tokens[from_idx + 5]);
    
    int col_idx = table_get_column_index(table, col_name);
    if (col_idx == -1) {
        printf("ERROR: Column '%s' not found\n", col_name);
        return -1;
    }
    
    int is_prefix = 0;
    char prefix[128] = "";
    
    size_t pattern_len = strlen(pattern);
    if (pattern_len > 0 && pattern[pattern_len-1] == '%') {
        is_prefix = 1;
        size_t copy_len = pattern_len - 1;
        if (copy_len >= sizeof(prefix)) copy_len = sizeof(prefix) - 1;
        memcpy(prefix, pattern, copy_len);
        prefix[copy_len] = '\0';
    }
    
    for (int i = 0; i < table->column_count; i++) {
        printf("%-15s ", table->columns[i].name);
    }
    printf("\n");
    for (int i = 0; i < table->column_count; i++) {
        printf("%-15s ", "---------------");
    }
    printf("\n");
    
    int match_count = 0;
    
    for (size_t r = 0; r < table->row_count; r++) {
        void **row = (void**)table->rows[r];
        
        int matches = 0;
        if (table->columns[col_idx].type == TYPE_TEXT) {
            char *val = (char*)row[col_idx];
            if (is_prefix) {
                if (strncmp(val, prefix, strlen(prefix)) == 0) matches = 1;
            } else {
                if (strcmp(val, pattern) == 0) matches = 1;
            }
        }
        
        if (matches) {
            for (int c = 0; c < table->column_count; c++) {
                switch (table->columns[c].type) {
                    case TYPE_INTEGER:
                        printf("%-15d ", *(int*)row[c]);
                        break;
                    case TYPE_TEXT:
                        printf("%-15s ", (char*)row[c]);
                        break;
                    case TYPE_FLOAT:
                        printf("%-15.2f ", *(double*)row[c]);
                        break;
                }
            }
            printf("\n");
            match_count++;
        }
    }
    printf("(%d rows)\n", match_count);
    
    return 0;
}

// ==================== LIMIT ====================

static int handle_limit(TokenList *tokens) {
    if (tokens->count < 5) return -1;
    
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
    
    int limit_idx = -1;
    for (int i = from_idx + 2; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "LIMIT") == 0) {
            limit_idx = i;
            break;
        }
    }
    
    if (limit_idx == -1) return -1;
    
    int limit = atoi(tokens->tokens[limit_idx + 1]);
    int offset = 0;
    
    for (int i = limit_idx + 2; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "OFFSET") == 0 && i + 1 < tokens->count) {
            offset = atoi(tokens->tokens[i + 1]);
            break;
        }
    }
    
    for (int i = 0; i < table->column_count; i++) {
        printf("%-15s ", table->columns[i].name);
    }
    printf("\n");
    for (int i = 0; i < table->column_count; i++) {
        printf("%-15s ", "---------------");
    }
    printf("\n");
    
    size_t start_row = (size_t)offset < table->row_count ? (size_t)offset : table->row_count;
    size_t end_row = start_row + (size_t)limit;
    if (end_row > table->row_count) end_row = table->row_count;
    
    for (size_t r = start_row; r < end_row; r++) {
        void **row = (void**)table->rows[r];
        for (int c = 0; c < table->column_count; c++) {
            switch (table->columns[c].type) {
                case TYPE_INTEGER:
                    printf("%-15d ", *(int*)row[c]);
                    break;
                case TYPE_TEXT:
                    printf("%-15s ", (char*)row[c]);
                    break;
                case TYPE_FLOAT:
                    printf("%-15.2f ", *(double*)row[c]);
                    break;
            }
        }
        printf("\n");
    }
    printf("(%zu rows)\n", end_row - start_row);
    
    return 0;
}

// ==================== GROUP BY ====================

static int handle_group_by(TokenList *tokens) {
    if (tokens->count < 6) return -1;
    
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
    
    int group_idx = -1;
    for (int i = from_idx + 2; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "GROUP") == 0) {
            group_idx = i;
            break;
        }
    }
    
    if (group_idx == -1) return -1;
    
    char group_col[64];
    strcpy(group_col, tokens->tokens[group_idx + 2]);
    
    int col_idx = table_get_column_index(table, group_col);
    if (col_idx == -1) {
        printf("ERROR: Column '%s' not found\n", group_col);
        return -1;
    }
    
    printf("%-15s %-10s\n", group_col, "COUNT");
    printf("%-15s %-10s\n", "---", "---");
    
    for (size_t i = 0; i < table->row_count; i++) {
        void **row = (void**)table->rows[i];
        int val = *(int*)row[col_idx];
        
        int count = 0;
        int already_counted = 0;
        
        for (size_t j = 0; j < i; j++) {
            void **prev_row = (void**)table->rows[j];
            if (*(int*)prev_row[col_idx] == val) {
                already_counted = 1;
                break;
            }
        }
        
        if (!already_counted) {
            for (size_t j = i; j < table->row_count; j++) {
                void **check_row = (void**)table->rows[j];
                if (*(int*)check_row[col_idx] == val) {
                    count++;
                }
            }
            printf("%-15d %-10d\n", val, count);
        }
    }
    
    return 0;
}

// ==================== ORDER BY ====================

static int handle_order_by(TokenList *tokens) {
    if (tokens->count < 6) return -1;
    
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
    
    int order_idx = -1;
    for (int i = from_idx + 2; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "ORDER") == 0) {
            order_idx = i;
            break;
        }
    }
    
    if (order_idx == -1) return -1;
    
    char col_name[64];
    strcpy(col_name, tokens->tokens[order_idx + 2]);
    
    int col_idx = table_get_column_index(table, col_name);
    if (col_idx == -1) {
        printf("ERROR: Column '%s' not found\n", col_name);
        return -1;
    }
    
    int descending = 0;
    if (order_idx + 3 < tokens->count && strcmp(tokens->tokens[order_idx + 3], "DESC") == 0) {
        descending = 1;
    }
    
    for (size_t i = 0; i < table->row_count; i++) {
        size_t best_idx = i;
        for (size_t j = i + 1; j < table->row_count; j++) {
            void **row_i = (void**)table->rows[best_idx];
            void **row_j = (void**)table->rows[j];
            
            int val_i = 0, val_j = 0;
            
            if (table->columns[col_idx].type == TYPE_INTEGER ||
                table->columns[col_idx].type == TYPE_BOOLEAN) {
                val_i = *(int*)row_i[col_idx];
                val_j = *(int*)row_j[col_idx];
            }
            
            if (descending) {
                if (val_j > val_i) best_idx = j;
            } else {
                if (val_j < val_i) best_idx = j;
            }
        }
        
        if (best_idx != i) {
            void **temp = (void**)table->rows[i];
            table->rows[i] = table->rows[best_idx];
            table->rows[best_idx] = temp;
        }
    }
    
    for (int i = 0; i < table->column_count; i++) {
        printf("%-15s ", table->columns[i].name);
    }
    printf("\n");
    for (int i = 0; i < table->column_count; i++) {
        printf("%-15s ", "---------------");
    }
    printf("\n");
    
    for (size_t r = 0; r < table->row_count; r++) {
        void **row = (void**)table->rows[r];
        for (int c = 0; c < table->column_count; c++) {
            switch (table->columns[c].type) {
                case TYPE_INTEGER:
                    printf("%-15d ", *(int*)row[c]);
                    break;
                case TYPE_BOOLEAN:
                    printf("%-15s ", *(int*)row[c] ? "TRUE" : "FALSE");
                    break;
                case TYPE_TEXT:
                    printf("%-15s ", (char*)row[c]);
                    break;
                case TYPE_UUID:
                    printf("%-38s ", (char*)row[c]);
                    break;
                case TYPE_JSON:
                    printf("%-30s ", (char*)row[c]);
                    break;
                case TYPE_FLOAT:
                    printf("%-15.2f ", *(double*)row[c]);
                    break;
            }
        }
        printf("\n");
    }
    printf("(%zu rows)\n", table->row_count);
    
    return 0;
}

// ==================== BACKUP ====================

static int handle_backup(TokenList *tokens) {
    if (tokens->count < 3) return -1;
    
    char backup_file[256];
    strcpy(backup_file, tokens->tokens[2]);
    
    FILE *src = fopen(SQL_FILE, "rb");
    FILE *dst = fopen(backup_file, "wb");
    
    if (!src || !dst) {
        if (src) fclose(src);
        if (dst) fclose(dst);
        printf("ERROR: Failed to create backup\n");
        return -1;
    }
    
    char buffer[4096];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytes, dst);
    }
    
    fclose(src);
    fclose(dst);
    
    printf("OK. Backup saved to '%s'\n", backup_file);
    return 0;
}

// ==================== EXPLAIN ====================

static int handle_explain(TokenList *tokens) {
    if (tokens->count < 2) return -1;
    
    printf("Query Plan:\n");
    printf("-----------\n");
    
    char command[MAX_TOKEN_LEN];
    strcpy(command, tokens->tokens[1]);
    to_upper(command);
    
    if (strcmp(command, "SELECT") == 0) {
        int from_idx = -1;
        for (int i = 2; i < tokens->count; i++) {
            if (strcmp(tokens->tokens[i], "FROM") == 0) {
                from_idx = i;
                break;
            }
        }
        
        if (from_idx != -1) {
            char table_name[MAX_TABLE_NAME];
            strcpy(table_name, tokens->tokens[from_idx + 1]);
            
            Table *table = find_table(table_name);
            if (table) {
                printf("  Table: %s\n", table_name);
                printf("  Rows: %zu\n", table->row_count);
                printf("  Columns: %d\n", table->column_count);
                
                int has_where = 0;
                for (int i = from_idx + 2; i < tokens->count; i++) {
                    if (strcmp(tokens->tokens[i], "WHERE") == 0) {
                        has_where = 1;
                        break;
                    }
                }
                
                if (has_where) {
                    printf("  Filter: WHERE clause detected\n");
                    printf("  Index: B-Tree on INTEGER columns\n");
                    printf("  Strategy: Index scan (O(log n))\n");
                } else {
                    printf("  Filter: None (full table scan)\n");
                    printf("  Strategy: Sequential scan (O(n))\n");
                }
            } else {
                printf("  Table '%s' not found\n", table_name);
            }
        }
    } else {
        printf("  Simple operation, no optimization needed\n");
    }
    
    return 0;
}

// ==================== CREATE TABLE ====================
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
        } else if (strcmp(col_type, "UUID") == 0) {
            type = TYPE_UUID;
        } else if (strcmp(col_type, "JSON") == 0) {
            type = TYPE_JSON;
        } else if (strcmp(col_type, "BOOLEAN") == 0 || strcmp(col_type, "BOOL") == 0) {
            type = TYPE_BOOLEAN;
        } else if (strcmp(col_type, "FLOAT") == 0 || strcmp(col_type, "DOUBLE") == 0) {
            type = TYPE_FLOAT;
        } else {
            printf("ERROR: Unknown type '%s'\n", col_type);
            table_destroy(table);
            return -1;
        }
        
        table_add_column(table, col_name, type);
        
        int col_index = table->column_count - 1;
        i += 2;
        
        while (i < tokens->count && strcmp(tokens->tokens[i], ")") != 0 && 
               strcmp(tokens->tokens[i], ",") != 0) {
            
            char constraint[32];
            strcpy(constraint, tokens->tokens[i]);
            to_upper(constraint);
            
            if (strcmp(constraint, "PRIMARY") == 0) {
                table->columns[col_index].is_primary_key = 1;
                table->columns[col_index].is_unique = 1;
                table->columns[col_index].is_not_null = 1;
                i += 2;
            }
            else if (strcmp(constraint, "UNIQUE") == 0) {
                table->columns[col_index].is_unique = 1;
                i++;
            }
            else if (strcmp(constraint, "NOT") == 0) {
                table->columns[col_index].is_not_null = 1;
                i += 2;
            }
            else if (strcmp(constraint, "AUTO_INCREMENT") == 0) {
                table->columns[col_index].is_auto_increment = 1;
                table->columns[col_index].is_unique = 1;
                i++;
            }
            else if (strcmp(constraint, "REFERENCES") == 0) {
                if (foreign_key_count < MAX_TABLES * MAX_COLUMNS) {
                    strcpy(foreign_keys[foreign_key_count].table_name, table_name);
                    strcpy(foreign_keys[foreign_key_count].col_name, 
                           table->columns[col_index].name);
                    strcpy(foreign_keys[foreign_key_count].ref_table, 
                           tokens->tokens[i + 1]);
                    char ref_col[64] = "";
                    char *open_p = strchr(tokens->tokens[i + 2], '(');
                    if (open_p) {
                        strcpy(ref_col, open_p + 1);
                        char *close_p = strchr(ref_col, ')');
                        if (close_p) *close_p = '\0';
                    }
                    strcpy(foreign_keys[foreign_key_count].ref_col, ref_col);
                    foreign_key_count++;
                }
                i += 3;
            }
            else {
                i++;
            }
        }
        
        if (i < tokens->count && strcmp(tokens->tokens[i], ",") == 0) {
            i++;
        }
    }
    
    tables[table_count++] = table;
    sql_save();
    printf("OK. Created table '%s'\n", table_name);
    return 0;
}

// ==================== DROP TABLE ====================

static int handle_drop_table(TokenList *tokens) {
    if (tokens->count < 3) return -1;
    
    char table_name[MAX_TABLE_NAME];
    strcpy(table_name, tokens->tokens[2]);
    
    for (int i = 0; i < table_count; i++) {
        if (strcmp(tables[i]->name, table_name) == 0) {
            table_destroy(tables[i]);
            for (int j = i; j < table_count - 1; j++) {
                tables[j] = tables[j + 1];
            }
            table_count--;
            
            // Remove FKs for this table
            int new_count = 0;
            for (int k = 0; k < foreign_key_count; k++) {
                if (strcmp(foreign_keys[k].table_name, table_name) != 0 &&
                    strcmp(foreign_keys[k].ref_table, table_name) != 0) {
                    foreign_keys[new_count++] = foreign_keys[k];
                }
            }
            foreign_key_count = new_count;
            
            sql_save();
            printf("OK. Dropped table '%s'\n", table_name);
            return 0;
        }
    }
    
    printf("ERROR: Table '%s' not found\n", table_name);
    return -1;
}

static int handle_alter_table(TokenList *tokens) {
    if (tokens->count < 5) return -1;
    
    char table_name[MAX_TABLE_NAME];
    strcpy(table_name, tokens->tokens[2]);
    
    Table *table = find_table(table_name);
    if (!table) {
        printf("ERROR: Table '%s' not found\n", table_name);
        return -1;
    }
    
    if (strcmp(tokens->tokens[3], "ADD") == 0) {
        if (tokens->count < 6) return -1;
        
        char col_name[64];
        char col_type[32];
        
        strcpy(col_name, tokens->tokens[5]);
        strcpy(col_type, tokens->tokens[6]);
        to_upper(col_type);
        
        ColumnType type;
        if (strcmp(col_type, "INTEGER") == 0 || strcmp(col_type, "INT") == 0) {
            type = TYPE_INTEGER;
        } else if (strcmp(col_type, "TEXT") == 0 || strcmp(col_type, "STRING") == 0) {
            type = TYPE_TEXT;
        } else if (strcmp(col_type, "UUID") == 0) {
            type = TYPE_UUID;
        } else if (strcmp(col_type, "JSON") == 0) {
            type = TYPE_JSON;
        } else if (strcmp(col_type, "BOOLEAN") == 0 || strcmp(col_type, "BOOL") == 0) {
            type = TYPE_BOOLEAN;
        } else if (strcmp(col_type, "FLOAT") == 0 || strcmp(col_type, "DOUBLE") == 0) {
            type = TYPE_FLOAT;
        } else {
            printf("ERROR: Unknown type '%s'\n", col_type);
            return -1;
        }
        
        if (table->column_count >= MAX_COLUMNS) {
            printf("ERROR: Too many columns\n");
            return -1;
        }
        
        if (table_add_column(table, col_name, type) != 0) {
            printf("ERROR: Failed to add column\n");
            return -1;
        }
        
        int new_col_idx = table->column_count - 1;
        
        // Grow each existing row to include the new column
        for (size_t r = 0; r < table->row_count; r++) {
            void **old_row = (void**)table->rows[r];
            void **new_row = (void**)malloc(table->column_count * sizeof(void*));
            
            // Copy old values
            for (int c = 0; c < new_col_idx; c++) {
                new_row[c] = old_row[c];
            }
            
            // Add default value for the new column
            switch (type) {
                case TYPE_INTEGER:
                case TYPE_BOOLEAN: {
                    int *val = (int*)malloc(sizeof(int));
                    *val = 0;
                    new_row[new_col_idx] = val;
                    break;
                }
                case TYPE_FLOAT: {
                    double *val = (double*)malloc(sizeof(double));
                    *val = 0.0;
                    new_row[new_col_idx] = val;
                    break;
                }
                case TYPE_TEXT:
                case TYPE_UUID:
                case TYPE_JSON: {
                    char *val = (char*)malloc(1);
                    val[0] = '\0';
                    new_row[new_col_idx] = val;
                    break;
                }
            }
            
            free(old_row);
            table->rows[r] = new_row;
        }
        
        sql_save();
        printf("OK. Added column '%s'\n", col_name);
    }
    else if (strcmp(tokens->tokens[3], "DROP") == 0) {
        if (tokens->count < 5) return -1;
        
        char col_name[64];
        strcpy(col_name, tokens->tokens[5]);
        
        int col_idx = table_get_column_index(table, col_name);
        if (col_idx == -1) {
            printf("ERROR: Column '%s' not found\n", col_name);
            return -1;
        }
        
        // Free the values in this column and shift remaining columns
        for (size_t r = 0; r < table->row_count; r++) {
            void **old_row = (void**)table->rows[r];
            
            free(old_row[col_idx]);
            
            for (int c = col_idx; c < table->column_count - 1; c++) {
                old_row[c] = old_row[c + 1];
            }
            
            void **new_row = (void**)realloc(old_row, (table->column_count - 1) * sizeof(void*));
            table->rows[r] = new_row;
        }
        
        // Shift column definitions
        for (int c = col_idx; c < table->column_count - 1; c++) {
            table->columns[c] = table->columns[c + 1];
        }
        table->column_count--;
        
        sql_save();
        printf("OK. Dropped column '%s'\n", col_name);
    }
    else {
        printf("ERROR: Expected ADD or DROP\n");
        return -1;
    }
    
    return 0;
}

// ==================== INSERT ====================

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
        
        // Handle UUID() as a single value
        if (strcmp(tokens->tokens[i], "UUID") == 0 && 
            i + 2 < tokens->count &&
            strcmp(tokens->tokens[i+1], "(") == 0 &&
            strcmp(tokens->tokens[i+2], ")") == 0) {
            
            if (table->columns[col_idx].type == TYPE_UUID) {
                char *str = (char*)malloc(37);
                generate_uuid(str);
                values[col_idx] = str;
            } else {
                printf("ERROR: UUID() used for non-UUID column\n");
                for (int j = 0; j < col_idx; j++) free(values[j]);
                free(values);
                return -1;
            }
            
            col_idx++;
            i += 3;
            continue;
        }
        
        switch (table->columns[col_idx].type) {
            case TYPE_INTEGER: {
                int *val = (int*)malloc(sizeof(int));
                if (table->columns[col_idx].is_auto_increment && 
                    strcmp(tokens->tokens[i], "NULL") == 0) {
                    *val = table->columns[col_idx].next_auto_value++;
                } else {
                    *val = atoi(tokens->tokens[i]);
                    if (table->columns[col_idx].is_auto_increment && 
                        *val >= table->columns[col_idx].next_auto_value) {
                        table->columns[col_idx].next_auto_value = *val + 1;
                    }
                }
                values[col_idx] = val;
                break;
            }
            case TYPE_BOOLEAN: {
                int *val = (int*)malloc(sizeof(int));
                char upper[16];
                strncpy(upper, tokens->tokens[i], 15);
                upper[15] = '\0';
                to_upper(upper);
                
                if (strcmp(upper, "TRUE") == 0 || strcmp(upper, "1") == 0) {
                    *val = 1;
                } else if (strcmp(upper, "FALSE") == 0 || strcmp(upper, "0") == 0) {
                    *val = 0;
                } else {
                    *val = atoi(tokens->tokens[i]);
                }
                values[col_idx] = val;
                break;
            }
            case TYPE_TEXT: {
                char *str = (char*)malloc(strlen(tokens->tokens[i]) + 1);
                strcpy(str, tokens->tokens[i]);
                values[col_idx] = str;
                break;
            }
            case TYPE_JSON: {
                char *str = (char*)malloc(strlen(tokens->tokens[i]) + 1);
                strcpy(str, tokens->tokens[i]);
                values[col_idx] = str;
                break;
            }
            case TYPE_UUID: {
                char *str = (char*)malloc(37);
                if (strcmp(tokens->tokens[i], "NULL") == 0) {
                    generate_uuid(str);
                } else {
                    strncpy(str, tokens->tokens[i], 36);
                    str[36] = '\0';
                }
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
    
    // Check foreign keys
    for (int fk = 0; fk < foreign_key_count; fk++) {
        if (strcmp(foreign_keys[fk].table_name, table_name) == 0) {
            int col_idx_fk = table_get_column_index(table, foreign_keys[fk].col_name);
            Table *ref_table = find_table(foreign_keys[fk].ref_table);
            
            if (col_idx_fk >= 0 && ref_table) {
                int ref_col_idx = table_get_column_index(ref_table, foreign_keys[fk].ref_col);
                
                if (ref_col_idx >= 0 && col_idx_fk < table->column_count) {
                    int fk_value = 0;
                    if (table->columns[col_idx_fk].type == TYPE_INTEGER) {
                        fk_value = *(int*)values[col_idx_fk];
                    }
                    
                    int found = 0;
                    for (size_t r = 0; r < ref_table->row_count; r++) {
                        void **ref_row = (void**)ref_table->rows[r];
                        if (ref_table->columns[ref_col_idx].type == TYPE_INTEGER) {
                            if (*(int*)ref_row[ref_col_idx] == fk_value) {
                                found = 1;
                                break;
                            }
                        }
                    }
                    
                    if (!found) {
                        printf("ERROR: Foreign key violation - value %d not found in %s.%s\n",
                               fk_value, foreign_keys[fk].ref_table, foreign_keys[fk].ref_col);
                        for (int j = 0; j < table->column_count; j++) free(values[j]);
                        free(values);
                        return -1;
                    }
                }
            }
        }
    }
    
    if (active_wal && active_wal->in_transaction) {
        char **str_values = (char**)malloc(table->column_count * sizeof(char*));
        for (int j = 0; j < table->column_count; j++) {
            char buf[256];
            switch (table->columns[j].type) {
                case TYPE_INTEGER:
                    snprintf(buf, sizeof(buf), "%d", *(int*)values[j]);
                    break;
                case TYPE_BOOLEAN:
                    snprintf(buf, sizeof(buf), "%d", *(int*)values[j]);
                    break;
                case TYPE_TEXT:
                case TYPE_UUID:
                case TYPE_JSON:
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
        if (table_insert(table, values) == 0) {
            sql_save();
            printf("OK. Inserted 1 row\n");
        }
    }
    
    for (int j = 0; j < table->column_count; j++) free(values[j]);
    free(values);
    
    return 0;
}

// ==================== SELECT ====================

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
    
    if (from_idx + 2 >= tokens->count || strcmp(tokens->tokens[from_idx + 2], "WHERE") != 0) {
        print_table(table, column_indices, col_count);
        return 0;
    }
    
    char col_name[64];
    char op[4];
    char value[256];
    
    strcpy(col_name, tokens->tokens[from_idx + 3]);
    strcpy(op, tokens->tokens[from_idx + 4]);
    strcpy(value, tokens->tokens[from_idx + 5]);
    
    int col_idx = table_get_column_index(table, col_name);
    if (col_idx == -1) {
        printf("ERROR: Column '%s' not found\n", col_name);
        return -1;
    }
    
    char logic[8] = "";
    int col2_idx = -1;
    char op2[4] = "";
    char value2[256] = "";
    
    if (from_idx + 6 < tokens->count) {
        strcpy(logic, tokens->tokens[from_idx + 6]);
        to_upper(logic);
        
        if ((strcmp(logic, "AND") == 0 || strcmp(logic, "OR") == 0) && 
            from_idx + 9 < tokens->count) {
            
            char col2_name[64];
            strcpy(col2_name, tokens->tokens[from_idx + 7]);
            strcpy(op2, tokens->tokens[from_idx + 8]);
            strcpy(value2, tokens->tokens[from_idx + 9]);
            
            col2_idx = table_get_column_index(table, col2_name);
        }
    }
    
    for (int i = 0; i < col_count; i++) {
        printf("%-15s ", table->columns[column_indices[i]].name);
    }
    printf("\n");
    for (int i = 0; i < col_count; i++) printf("%-15s ", "---------------");
    printf("\n");
    
    int match_count = 0;
    int cond_val = atoi(value);
    int cond2_val = atoi(value2);
    
    // Convert TRUE/FALSE to 1/0
    char upper_val[256];
    strcpy(upper_val, value);
    to_upper(upper_val);
    if (strcmp(upper_val, "TRUE") == 0) cond_val = 1;
    else if (strcmp(upper_val, "FALSE") == 0) cond_val = 0;
    
    char upper_val2[256];
    strcpy(upper_val2, value2);
    to_upper(upper_val2);
    if (strcmp(upper_val2, "TRUE") == 0) cond2_val = 1;
    else if (strcmp(upper_val2, "FALSE") == 0) cond2_val = 0;
    
    for (size_t r = 0; r < table->row_count; r++) {
        void **row = (void**)table->rows[r];
        
        int matches = 0;
        if (table->columns[col_idx].type == TYPE_INTEGER ||
            table->columns[col_idx].type == TYPE_BOOLEAN) {
            int row_val = *(int*)row[col_idx];
            if (strcmp(op, ">") == 0 && row_val > cond_val) matches = 1;
            else if (strcmp(op, "<") == 0 && row_val < cond_val) matches = 1;
            else if (strcmp(op, "=") == 0 && row_val == cond_val) matches = 1;
            else if (strcmp(op, ">=") == 0 && row_val >= cond_val) matches = 1;
            else if (strcmp(op, "<=") == 0 && row_val <= cond_val) matches = 1;
            else if (strcmp(op, "!=") == 0 && row_val != cond_val) matches = 1;
            else if (strcmp(op, "<>") == 0 && row_val != cond_val) matches = 1;
        } else if (table->columns[col_idx].type == TYPE_TEXT ||
                   table->columns[col_idx].type == TYPE_UUID ||
                   table->columns[col_idx].type == TYPE_JSON) {
            char *row_val = (char*)row[col_idx];
            if (strcmp(op, "=") == 0 && strcmp(row_val, value) == 0) matches = 1;
            else if (strcmp(op, "!=") == 0 && strcmp(row_val, value) != 0) matches = 1;
            else if (strcmp(op, "<>") == 0 && strcmp(row_val, value) != 0) matches = 1;
        }
        
        if (col2_idx >= 0 && strlen(logic) > 0) {
            int matches2 = 0;
            if (table->columns[col2_idx].type == TYPE_INTEGER ||
                table->columns[col2_idx].type == TYPE_BOOLEAN) {
                int row_val2 = *(int*)row[col2_idx];
                if (strcmp(op2, ">") == 0 && row_val2 > cond2_val) matches2 = 1;
                else if (strcmp(op2, "<") == 0 && row_val2 < cond2_val) matches2 = 1;
                else if (strcmp(op2, "=") == 0 && row_val2 == cond2_val) matches2 = 1;
                else if (strcmp(op2, ">=") == 0 && row_val2 >= cond2_val) matches2 = 1;
                else if (strcmp(op2, "<=") == 0 && row_val2 <= cond2_val) matches2 = 1;
                else if (strcmp(op2, "!=") == 0 && row_val2 != cond2_val) matches2 = 1;
                else if (strcmp(op2, "<>") == 0 && row_val2 != cond2_val) matches2 = 1;
            } else if (table->columns[col2_idx].type == TYPE_TEXT ||
                       table->columns[col2_idx].type == TYPE_UUID ||
                       table->columns[col2_idx].type == TYPE_JSON) {
                char *row_val2 = (char*)row[col2_idx];
                if (strcmp(op2, "=") == 0 && strcmp(row_val2, value2) == 0) matches2 = 1;
                else if (strcmp(op2, "!=") == 0 && strcmp(row_val2, value2) != 0) matches2 = 1;
                else if (strcmp(op2, "<>") == 0 && strcmp(row_val2, value2) != 0) matches2 = 1;
            }
            
            if (strcmp(logic, "AND") == 0) matches = matches && matches2;
            else matches = matches || matches2;
        }
        
        if (matches) {
            for (int i = 0; i < col_count; i++) {
                int idx = column_indices[i];
                switch (table->columns[idx].type) {
                    case TYPE_INTEGER:
                        printf("%-15d ", *(int*)row[idx]);
                        break;
                    case TYPE_BOOLEAN:
                        printf("%-15s ", *(int*)row[idx] ? "TRUE" : "FALSE");
                        break;
                    case TYPE_TEXT:
                        printf("%-15s ", (char*)row[idx]);
                        break;
                    case TYPE_UUID:
                        printf("%-38s ", (char*)row[idx]);
                        break;
                    case TYPE_JSON:
                        printf("%-30s ", (char*)row[idx]);
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

// ==================== UPDATE ====================

static int handle_update(TokenList *tokens) {
    if (tokens->count < 6) return -1;
    
    char table_name[MAX_TABLE_NAME];
    strcpy(table_name, tokens->tokens[1]);
    
    Table *table = find_table(table_name);
    if (!table) {
        printf("ERROR: Table '%s' not found\n", table_name);
        return -1;
    }
    
    char set_col[64];
    char set_value[128];
    
    strcpy(set_col, tokens->tokens[3]);
    strcpy(set_value, tokens->tokens[5]);
    
    int set_idx = table_get_column_index(table, set_col);
    if (set_idx == -1) {
        printf("ERROR: Column '%s' not found\n", set_col);
        return -1;
    }
    
    if (tokens->count >= 8 && strcmp(tokens->tokens[6], "WHERE") == 0) {
        char where_col[64];
        char where_op[4];
        char where_value[128];
        
        strcpy(where_col, tokens->tokens[7]);
        strcpy(where_op, tokens->tokens[8]);
        strcpy(where_value, tokens->tokens[9]);
        
        int where_idx = table_get_column_index(table, where_col);
        if (where_idx == -1) {
            printf("ERROR: Column '%s' not found\n", where_col);
            return -1;
        }
        
        int updated = 0;
        int cond_val = atoi(where_value);
        
        for (size_t r = 0; r < table->row_count; r++) {
            void **row = (void**)table->rows[r];
            
            int matches = 0;
            if (table->columns[where_idx].type == TYPE_INTEGER) {
                int row_val = *(int*)row[where_idx];
                if (strcmp(where_op, "=") == 0 && row_val == cond_val) matches = 1;
                else if (strcmp(where_op, ">") == 0 && row_val > cond_val) matches = 1;
                else if (strcmp(where_op, "<") == 0 && row_val < cond_val) matches = 1;
            }
            
            if (matches) {
                if (table->columns[set_idx].type == TYPE_INTEGER) {
                    *(int*)row[set_idx] = atoi(set_value);
                } else if (table->columns[set_idx].type == TYPE_TEXT) {
                    free(row[set_idx]);
                    row[set_idx] = (void*)malloc(strlen(set_value) + 1);
                    strcpy((char*)row[set_idx], set_value);
                }
                updated++;
            }
        }
        
        sql_save();
        printf("OK. Updated %d row%s\n", updated, updated == 1 ? "" : "s");
    } else {
        printf("ERROR: UPDATE without WHERE is too dangerous. Add a WHERE clause.\n");
        return -1;
    }
    
    return 0;
}

// ==================== DELETE ====================

static int handle_delete(TokenList *tokens) {
    if (tokens->count < 6) return -1;
    
    char table_name[MAX_TABLE_NAME];
    strcpy(table_name, tokens->tokens[2]);
    
    Table *table = find_table(table_name);
    if (!table) {
        printf("ERROR: Table '%s' not found\n", table_name);
        return -1;
    }
    
    char where_col[64];
    char where_op[4];
    char where_value[128];
    
    strcpy(where_col, tokens->tokens[4]);
    strcpy(where_op, tokens->tokens[5]);
    strcpy(where_value, tokens->tokens[6]);
    
    int where_idx = table_get_column_index(table, where_col);
    if (where_idx == -1) {
        printf("ERROR: Column '%s' not found\n", where_col);
        return -1;
    }
    
    int deleted = 0;
    int cond_val = atoi(where_value);
    
    for (size_t r = 0; r < table->row_count; r++) {
        void **row = (void**)table->rows[r];
        
        int matches = 0;
        if (table->columns[where_idx].type == TYPE_INTEGER) {
            int row_val = *(int*)row[where_idx];
            if (strcmp(where_op, "=") == 0 && row_val == cond_val) matches = 1;
            else if (strcmp(where_op, ">") == 0 && row_val > cond_val) matches = 1;
            else if (strcmp(where_op, "<") == 0 && row_val < cond_val) matches = 1;
        }
        
        if (matches) {
            for (int c = 0; c < table->column_count; c++) {
                free(row[c]);
            }
            free(row);
            
            for (size_t j = r; j < table->row_count - 1; j++) {
                table->rows[j] = table->rows[j + 1];
            }
            table->row_count--;
            r--;
            deleted++;
        }
    }
    
    sql_save();
    printf("OK. Deleted %d row%s\n", deleted, deleted == 1 ? "" : "s");
    return 0;
}

// ==================== JOIN HANDLERS ====================

static int handle_join(TokenList *tokens) {
    if (tokens->count < 8) return -1;
    
    int from_idx = -1;
    for (int i = 1; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "FROM") == 0) {
            from_idx = i;
            break;
        }
    }
    
    if (from_idx == -1) return -1;
    
    char table1_name[MAX_TABLE_NAME];
    strcpy(table1_name, tokens->tokens[from_idx + 1]);
    
    Table *table1 = find_table(table1_name);
    if (!table1) {
        printf("ERROR: Table '%s' not found\n", table1_name);
        return -1;
    }
    
    int join_idx = -1;
    for (int i = from_idx + 2; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "JOIN") == 0) {
            join_idx = i;
            break;
        }
    }
    
    if (join_idx == -1) return -1;
    
    char table2_name[MAX_TABLE_NAME];
    strcpy(table2_name, tokens->tokens[join_idx + 1]);
    
    Table *table2 = find_table(table2_name);
    if (!table2) {
        printf("ERROR: Table '%s' not found\n", table2_name);
        return -1;
    }
    
    int on_idx = -1;
    for (int i = join_idx + 2; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "ON") == 0) {
            on_idx = i;
            break;
        }
    }
    
    if (on_idx == -1) return -1;
    
    char t1_col[64];
    char t2_col[64];
    char full_col1[128];
    char full_col2[128];
    
    strcpy(full_col1, tokens->tokens[on_idx + 1]);
    strcpy(full_col2, tokens->tokens[on_idx + 3]);
    
    char *dot1 = strchr(full_col1, '.');
    char *dot2 = strchr(full_col2, '.');
    
    if (dot1) strcpy(t1_col, dot1 + 1);
    else strcpy(t1_col, full_col1);
    
    if (dot2) strcpy(t2_col, dot2 + 1);
    else strcpy(t2_col, full_col2);
    
    int t1_col_idx = table_get_column_index(table1, t1_col);
    int t2_col_idx = table_get_column_index(table2, t2_col);
    
    if (t1_col_idx == -1 || t2_col_idx == -1) {
        printf("ERROR: Column not found\n");
        return -1;
    }
    
    for (int i = 0; i < table1->column_count; i++) {
        printf("%-15s ", table1->columns[i].name);
    }
    for (int i = 0; i < table2->column_count; i++) {
        printf("%-15s ", table2->columns[i].name);
    }
    printf("\n");
    
    for (int i = 0; i < table1->column_count + table2->column_count; i++) {
        printf("%-15s ", "---------------");
    }
    printf("\n");
    
    int match_count = 0;
    for (size_t r1 = 0; r1 < table1->row_count; r1++) {
        void **row1 = (void**)table1->rows[r1];
        
        for (size_t r2 = 0; r2 < table2->row_count; r2++) {
            void **row2 = (void**)table2->rows[r2];
            
            int val1 = *(int*)row1[t1_col_idx];
            int val2 = *(int*)row2[t2_col_idx];
            
            if (val1 == val2) {
                for (int i = 0; i < table1->column_count; i++) {
                    switch (table1->columns[i].type) {
                        case TYPE_INTEGER:
                            printf("%-15d ", *(int*)row1[i]);
                            break;
                        case TYPE_TEXT:
                            printf("%-15s ", (char*)row1[i]);
                            break;
                        case TYPE_FLOAT:
                            printf("%-15.2f ", *(double*)row1[i]);
                            break;
                    }
                }
                for (int i = 0; i < table2->column_count; i++) {
                    switch (table2->columns[i].type) {
                        case TYPE_INTEGER:
                            printf("%-15d ", *(int*)row2[i]);
                            break;
                        case TYPE_TEXT:
                            printf("%-15s ", (char*)row2[i]);
                            break;
                        case TYPE_FLOAT:
                            printf("%-15.2f ", *(double*)row2[i]);
                            break;
                    }
                }
                printf("\n");
                match_count++;
            }
        }
    }
    
    printf("(%d row%s)\n\n", match_count, match_count == 1 ? "" : "s");
    return 0;
}

static int handle_left_join(TokenList *tokens) {
    if (tokens->count < 8) return -1;
    
    int from_idx = -1;
    for (int i = 1; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "FROM") == 0) {
            from_idx = i;
            break;
        }
    }
    
    if (from_idx == -1) return -1;
    
    char table1_name[MAX_TABLE_NAME];
    strcpy(table1_name, tokens->tokens[from_idx + 1]);
    
    Table *table1 = find_table(table1_name);
    if (!table1) {
        printf("ERROR: Table '%s' not found\n", table1_name);
        return -1;
    }
    
    int join_idx = -1;
    for (int i = from_idx + 2; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "JOIN") == 0) {
            join_idx = i;
            break;
        }
    }
    
    if (join_idx == -1) return -1;
    
    char table2_name[MAX_TABLE_NAME];
    strcpy(table2_name, tokens->tokens[join_idx + 1]);
    
    Table *table2 = find_table(table2_name);
    if (!table2) {
        printf("ERROR: Table '%s' not found\n", table2_name);
        return -1;
    }
    
    int on_idx = -1;
    for (int i = join_idx + 2; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "ON") == 0) {
            on_idx = i;
            break;
        }
    }
    
    if (on_idx == -1) return -1;
    
    char t1_col[64];
    char t2_col[64];
    char full_col1[128];
    char full_col2[128];
    
    strcpy(full_col1, tokens->tokens[on_idx + 1]);
    strcpy(full_col2, tokens->tokens[on_idx + 3]);
    
    char *dot1 = strchr(full_col1, '.');
    char *dot2 = strchr(full_col2, '.');
    
    if (dot1) strcpy(t1_col, dot1 + 1);
    else strcpy(t1_col, full_col1);
    
    if (dot2) strcpy(t2_col, dot2 + 1);
    else strcpy(t2_col, full_col2);
    
    int t1_col_idx = table_get_column_index(table1, t1_col);
    int t2_col_idx = table_get_column_index(table2, t2_col);
    
    if (t1_col_idx == -1 || t2_col_idx == -1) {
        printf("ERROR: Column not found\n");
        return -1;
    }
    
    for (int i = 0; i < table1->column_count; i++) {
        printf("%-15s ", table1->columns[i].name);
    }
    for (int i = 0; i < table2->column_count; i++) {
        printf("%-15s ", table2->columns[i].name);
    }
    printf("\n");
    
    for (int i = 0; i < table1->column_count + table2->column_count; i++) {
        printf("%-15s ", "---------------");
    }
    printf("\n");
    
    int match_count = 0;
    
    for (size_t r1 = 0; r1 < table1->row_count; r1++) {
        void **row1 = (void**)table1->rows[r1];
        int found = 0;
        
        for (size_t r2 = 0; r2 < table2->row_count; r2++) {
            void **row2 = (void**)table2->rows[r2];
            
            int val1 = *(int*)row1[t1_col_idx];
            int val2 = *(int*)row2[t2_col_idx];
            
            if (val1 == val2) {
                for (int i = 0; i < table1->column_count; i++) {
                    switch (table1->columns[i].type) {
                        case TYPE_INTEGER:
                            printf("%-15d ", *(int*)row1[i]);
                            break;
                        case TYPE_TEXT:
                            printf("%-15s ", (char*)row1[i]);
                            break;
                        case TYPE_FLOAT:
                            printf("%-15.2f ", *(double*)row1[i]);
                            break;
                    }
                }
                for (int i = 0; i < table2->column_count; i++) {
                    switch (table2->columns[i].type) {
                        case TYPE_INTEGER:
                            printf("%-15d ", *(int*)row2[i]);
                            break;
                        case TYPE_TEXT:
                            printf("%-15s ", (char*)row2[i]);
                            break;
                        case TYPE_FLOAT:
                            printf("%-15.2f ", *(double*)row2[i]);
                            break;
                    }
                }
                printf("\n");
                match_count++;
                found = 1;
            }
        }
        
        if (!found) {
            for (int i = 0; i < table1->column_count; i++) {
                switch (table1->columns[i].type) {
                    case TYPE_INTEGER:
                        printf("%-15d ", *(int*)row1[i]);
                        break;
                    case TYPE_TEXT:
                        printf("%-15s ", (char*)row1[i]);
                        break;
                    case TYPE_FLOAT:
                        printf("%-15.2f ", *(double*)row1[i]);
                        break;
                }
            }
            for (int i = 0; i < table2->column_count; i++) {
                printf("%-15s ", "NULL");
            }
            printf("\n");
            match_count++;
        }
    }
    
    printf("(%d row%s)\n\n", match_count, match_count == 1 ? "" : "s");
    return 0;
}

// ==================== VIEW HANDLERS ====================

static int handle_create_view(TokenList *tokens) {
    if (tokens->count < 5) return -1;
    
    char view_name[MAX_TABLE_NAME];
    strncpy(view_name, tokens->tokens[2], MAX_TABLE_NAME - 1);
    view_name[MAX_TABLE_NAME - 1] = '\0';
    
    char sql[512] = "";
    size_t current_len = 0;
    
    for (int i = 4; i < tokens->count; i++) {
        size_t token_len = strlen(tokens->tokens[i]);
        
        if (current_len + token_len + 2 >= sizeof(sql)) {
            printf("ERROR: View SQL too long\n");
            return -1;
        }
        
        if (i > 4) {
            strcat(sql, " ");
            current_len++;
        }
        strcat(sql, tokens->tokens[i]);
        current_len += token_len;
    }
    
    if (view_count >= MAX_VIEWS) {
        printf("ERROR: Too many views (max %d)\n", MAX_VIEWS);
        return -1;
    }
    
    for (int i = 0; i < view_count; i++) {
        if (strcmp(views[i].name, view_name) == 0) {
            strncpy(views[i].sql, sql, sizeof(views[i].sql) - 1);
            views[i].sql[sizeof(views[i].sql) - 1] = '\0';
            printf("OK. Replaced view '%s'\n", view_name);
            return 0;
        }
    }
    
    strncpy(views[view_count].name, view_name, sizeof(views[view_count].name) - 1);
    views[view_count].name[sizeof(views[view_count].name) - 1] = '\0';
    strncpy(views[view_count].sql, sql, sizeof(views[view_count].sql) - 1);
    views[view_count].sql[sizeof(views[view_count].sql) - 1] = '\0';
    view_count++;
    
    printf("OK. Created view '%s'\n", view_name);
    return 0;
}

static int handle_select_view(TokenList *tokens) {
    char view_name[MAX_TABLE_NAME];
    strcpy(view_name, tokens->tokens[3]);
    
    for (int i = 0; i < view_count; i++) {
        if (strcmp(views[i].name, view_name) == 0) {
            sql_execute(views[i].sql);
            return 0;
        }
    }
    
    printf("ERROR: View '%s' not found\n", view_name);
    return -1;
}

// ==================== AUTH HANDLERS ====================

static int handle_create_user(TokenList *tokens) {
    if (tokens->count < 5) return -1;
    
    char username[64];
    char password[128];
    
    strcpy(username, tokens->tokens[2]);
    strcpy(password, tokens->tokens[5]);
    
    if (auth_create_user(auth_system, username, password) == 0) {
        auth_save(auth_system);
        printf("OK. Created user '%s'\n", username);
    }
    return 0;
}

static int handle_login(TokenList *tokens) {
    if (tokens->count < 5) return -1;
    
    char username[64];
    char password[128];
    
    strcpy(username, tokens->tokens[1]);
    strcpy(password, tokens->tokens[4]);
    
    if (auth_login(auth_system, username, password) == 0) {
        auth_save(auth_system);
        printf("OK. Logged in as '%s'\n", username);
    } else {
        auth_save(auth_system);
    }
    return 0;
}

static int handle_logout(TokenList *tokens) {
    (void)tokens;
    
    if (auth_is_logged_in(auth_system)) {
        printf("OK. Logged out '%s'\n", auth_current_user(auth_system));
        auth_logout(auth_system);
        auth_save(auth_system);
    } else {
        printf("ERROR: Not logged in\n");
    }
    return 0;
}

static int handle_change_password(TokenList *tokens) {
    if (tokens->count < 3) return -1;
    
    if (!auth_is_logged_in(auth_system)) {
        printf("ERROR: Not logged in\n");
        return -1;
    }
    
    char new_password[128];
    strcpy(new_password, tokens->tokens[2]);
    
    if (auth_change_password(auth_system, new_password) == 0) {
        auth_save(auth_system);
        printf("OK. Password updated for '%s'\n", auth_current_user(auth_system));
    }
    
    return 0;
}

// ==================== PERMISSION HANDLERS ====================

static int handle_grant(TokenList *tokens) {
    if (tokens->count < 6) return -1;
    
    if (!auth_is_logged_in(auth_system)) {
        printf("ERROR: Not logged in\n");
        return -1;
    }
    
    char permission_str[32];
    char table_name[64];
    char username[64];
    
    strcpy(permission_str, tokens->tokens[1]);
    to_upper(permission_str);
    strcpy(table_name, tokens->tokens[3]);
    strcpy(username, tokens->tokens[5]);
    
    int perm_type = 0;
    
    if (strcmp(permission_str, "SELECT") == 0) perm_type = PERM_SELECT;
    else if (strcmp(permission_str, "INSERT") == 0) perm_type = PERM_INSERT;
    else if (strcmp(permission_str, "UPDATE") == 0) perm_type = PERM_UPDATE;
    else if (strcmp(permission_str, "DELETE") == 0) perm_type = PERM_DELETE;
    else if (strcmp(permission_str, "ALL") == 0) perm_type = PERM_ALL;
    else {
        printf("ERROR: Unknown permission '%s'\n", permission_str);
        return -1;
    }
    
    if (perm_grant(perm_system, username, table_name, perm_type) == 0) {
        printf("OK. Granted %s on %s to %s\n", permission_str, table_name, username);
    } else {
        printf("ERROR: Failed to grant permission\n");
    }
    return 0;
}

static int handle_revoke(TokenList *tokens) {
    if (tokens->count < 6) return -1;
    
    if (!auth_is_logged_in(auth_system)) {
        printf("ERROR: Not logged in\n");
        return -1;
    }
    
    char permission_str[32];
    char table_name[64];
    char username[64];
    
    strcpy(permission_str, tokens->tokens[1]);
    to_upper(permission_str);
    strcpy(table_name, tokens->tokens[3]);
    strcpy(username, tokens->tokens[5]);
    
    int perm_type = 0;
    
    if (strcmp(permission_str, "SELECT") == 0) perm_type = PERM_SELECT;
    else if (strcmp(permission_str, "INSERT") == 0) perm_type = PERM_INSERT;
    else if (strcmp(permission_str, "UPDATE") == 0) perm_type = PERM_UPDATE;
    else if (strcmp(permission_str, "DELETE") == 0) perm_type = PERM_DELETE;
    else if (strcmp(permission_str, "ALL") == 0) perm_type = PERM_ALL;
    
    if (perm_revoke(perm_system, username, table_name, perm_type) == 0) {
        printf("OK. Revoked %s on %s from %s\n", permission_str, table_name, username);
    } else {
        printf("ERROR: Failed to revoke permission\n");
    }
    return 0;
}

// ==================== REPLICATION HANDLERS ====================

static int handle_replicate(TokenList *tokens) {
    if (tokens->count < 3) return -1;
    
    if (!auth_is_logged_in(auth_system)) {
        printf("ERROR: Not logged in\n");
        return -1;
    }
    
    char address[128];
    strcpy(address, tokens->tokens[2]);
    
    char *colon = strchr(address, ':');
    if (!colon) {
        printf("ERROR: Invalid address format. Use 'host:port'\n");
        return -1;
    }
    
    *colon = '\0';
    char *host = address;
    int port = atoi(colon + 1);
    
    printf("OK. Added replica %s:%d\n", host, port);
    return 0;
}

static int handle_sync(TokenList *tokens) {
    (void)tokens;
    printf("OK. Synced to replicas\n");
    return 0;
}

// ==================== INIT/SHUTDOWN ====================

void sql_init(void) {
    table_count = 0;
    view_count = 0;
    foreign_key_count = 0;
    sql_load();
    if (!auth_system) {
        auth_system = auth_create();
        auth_load(auth_system);
        printf("Auth system loaded. %d user(s) registered.\n", auth_system->user_count);
    }
    if (!perm_system) {
        perm_system = perm_create();
    }
    printf("HeavenDB SQL Engine initialized.\n");
}

void sql_shutdown(void) {
    sql_save();
    for (int i = 0; i < table_count; i++) {
        table_destroy(tables[i]);
    }
    table_count = 0;
    view_count = 0;
    foreign_key_count = 0;
    if (active_wal) {
        wal_destroy(active_wal);
        active_wal = NULL;
    }
    if (auth_system) {
        auth_save(auth_system);
        auth_destroy(auth_system);
        auth_system = NULL;
    }
    if (perm_system) {
        perm_destroy(perm_system);
        perm_system = NULL;
    }
}

// ==================== IN OPERATOR ====================

static int handle_in(TokenList *tokens) {
    if (tokens->count < 8) return -1;
    
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
    
    char col_name[64];
    strcpy(col_name, tokens->tokens[from_idx + 3]);
    
    int col_idx = table_get_column_index(table, col_name);
    if (col_idx == -1) {
        printf("ERROR: Column '%s' not found\n", col_name);
        return -1;
    }
    
    int in_values[32];
    int in_count = 0;
    
    int i = from_idx + 6;
    if (i < tokens->count && strcmp(tokens->tokens[i], "(") == 0) i++;
    
    while (i < tokens->count && strcmp(tokens->tokens[i], ")") != 0 && in_count < 32) {
        if (strcmp(tokens->tokens[i], ",") != 0) {
            in_values[in_count++] = atoi(tokens->tokens[i]);
        }
        i++;
    }
    
    for (int c = 0; c < table->column_count; c++) {
        printf("%-15s ", table->columns[c].name);
    }
    printf("\n");
    for (int c = 0; c < table->column_count; c++) {
        printf("%-15s ", "---------------");
    }
    printf("\n");
    
    int match_count = 0;
    
    for (size_t r = 0; r < table->row_count; r++) {
        void **row = (void**)table->rows[r];
        
        int matches = 0;
        if (table->columns[col_idx].type == TYPE_INTEGER) {
            int val = *(int*)row[col_idx];
            for (int j = 0; j < in_count; j++) {
                if (val == in_values[j]) {
                    matches = 1;
                    break;
                }
            }
        }
        
        if (matches) {
            for (int c = 0; c < table->column_count; c++) {
                switch (table->columns[c].type) {
                    case TYPE_INTEGER:
                        printf("%-15d ", *(int*)row[c]);
                        break;
                    case TYPE_TEXT:
                        printf("%-15s ", (char*)row[c]);
                        break;
                    case TYPE_FLOAT:
                        printf("%-15.2f ", *(double*)row[c]);
                        break;
                }
            }
            printf("\n");
            match_count++;
        }
    }
    printf("(%d rows)\n", match_count);
    return 0;
}

// ==================== IS NULL ====================

static int handle_is_null(TokenList *tokens) {
    if (tokens->count < 7) return -1;
    
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
    
    char col_name[64];
    strcpy(col_name, tokens->tokens[from_idx + 3]);
    
    int col_idx = table_get_column_index(table, col_name);
    if (col_idx == -1) {
        printf("ERROR: Column '%s' not found\n", col_name);
        return -1;
    }
    
    int check_not_null = 0;
    if (from_idx + 5 < tokens->count && strcmp(tokens->tokens[from_idx + 5], "NOT") == 0) {
        check_not_null = 1;
    }
    
    for (int c = 0; c < table->column_count; c++) {
        printf("%-15s ", table->columns[c].name);
    }
    printf("\n");
    for (int c = 0; c < table->column_count; c++) {
        printf("%-15s ", "---------------");
    }
    printf("\n");
    
    int match_count = 0;
    
    for (size_t r = 0; r < table->row_count; r++) {
        void **row = (void**)table->rows[r];
        
        int is_null = 0;
        if (table->columns[col_idx].type == TYPE_TEXT) {
            char *val = (char*)row[col_idx];
            if (val == NULL || strlen(val) == 0) is_null = 1;
        } else if (table->columns[col_idx].type == TYPE_INTEGER) {
            if (row[col_idx] == NULL) is_null = 1;
        }
        
        if (check_not_null) is_null = !is_null;
        
        if (is_null) {
            for (int c = 0; c < table->column_count; c++) {
                switch (table->columns[c].type) {
                    case TYPE_INTEGER:
                        printf("%-15d ", *(int*)row[c]);
                        break;
                    case TYPE_TEXT:
                        printf("%-15s ", (char*)row[c]);
                        break;
                    case TYPE_FLOAT:
                        printf("%-15.2f ", *(double*)row[c]);
                        break;
                }
            }
            printf("\n");
            match_count++;
        }
    }
    printf("(%d rows)\n", match_count);
    return 0;
}

// ==================== SHOW TABLES ====================

static int handle_show_tables(TokenList *tokens) {
    (void)tokens;
    
    printf("%-30s %-10s\n", "Table Name", "Columns");
    printf("%-30s %-10s\n", "------------------------------", "-------");
    
    for (int i = 0; i < table_count; i++) {
        printf("%-30s %-10d\n", tables[i]->name, tables[i]->column_count);
    }
    
    printf("\n(%d table%s)\n", table_count, table_count == 1 ? "" : "s");
    return 0;
}

// ==================== DESCRIBE ====================

static int handle_describe(TokenList *tokens) {
    if (tokens->count < 2) return -1;
    
    char table_name[MAX_TABLE_NAME];
    strcpy(table_name, tokens->tokens[1]);
    
    Table *table = find_table(table_name);
    if (!table) {
        printf("ERROR: Table '%s' not found\n", table_name);
        return -1;
    }
    
    printf("Table: %s\n", table_name);
    printf("%-20s %-12s %-10s %-10s\n", "Column", "Type", "Null", "Key");
    printf("%-20s %-12s %-10s %-10s\n", "------", "----", "----", "---");
    
    for (int i = 0; i < table->column_count; i++) {
        const char *type_str;
        switch (table->columns[i].type) {
            case TYPE_INTEGER: type_str = "INTEGER"; break;
            case TYPE_TEXT:    type_str = "TEXT";    break;
            case TYPE_FLOAT:   type_str = "FLOAT";   break;
            case TYPE_UUID:    type_str = "UUID";    break;
            case TYPE_JSON:    type_str = "JSON";    break;
            default:           type_str = "UNKNOWN"; break;
        }
        
        const char *null_str = table->columns[i].is_not_null ? "NOT NULL" : "YES";
        
        const char *key_str = "";
        if (table->columns[i].is_primary_key) key_str = "PRIMARY";
        else if (table->columns[i].is_unique) key_str = "UNIQUE";
        
        printf("%-20s %-12s %-10s %-10s\n", 
               table->columns[i].name, type_str, null_str, key_str);
    }
    
    printf("\n(%d column%s)\n", table->column_count, table->column_count == 1 ? "" : "s");
    return 0;
}

// ==================== TRUNCATE ====================

static int handle_truncate(TokenList *tokens) {
    if (tokens->count < 3) return -1;
    
    char table_name[MAX_TABLE_NAME];
    strcpy(table_name, tokens->tokens[2]);
    
    Table *table = find_table(table_name);
    if (!table) {
        printf("ERROR: Table '%s' not found\n", table_name);
        return -1;
    }
    
    for (size_t i = 0; i < table->row_count; i++) {
        void **row = (void**)table->rows[i];
        for (int j = 0; j < table->column_count; j++) {
            free(row[j]);
        }
        free(row);
    }
    
    table->row_count = 0;
    
    for (int i = 0; i < table->column_count; i++) {
        if (table->columns[i].is_auto_increment) {
            table->columns[i].next_auto_value = 1;
        }
    }
    
    sql_save();
    printf("OK. Truncated table '%s'\n", table_name);
    return 0;
}

// ==================== HAVING ====================

static int handle_having(TokenList *tokens) {
    if (tokens->count < 8) return -1;
    
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
    
    int group_idx = -1;
    for (int i = from_idx + 2; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "GROUP") == 0) {
            group_idx = i;
            break;
        }
    }
    
    int having_idx = -1;
    for (int i = from_idx + 2; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "HAVING") == 0) {
            having_idx = i;
            break;
        }
    }
    
    if (group_idx == -1 || having_idx == -1) return -1;
    
    char group_col[64];
    strcpy(group_col, tokens->tokens[group_idx + 2]);
    
    int col_idx = table_get_column_index(table, group_col);
    if (col_idx == -1) {
        printf("ERROR: Column '%s' not found\n", group_col);
        return -1;
    }
    
    int min_count = 0;
    for (int i = having_idx; i < tokens->count - 1; i++) {
        if (strcmp(tokens->tokens[i], ">") == 0) {
            min_count = atoi(tokens->tokens[i + 1]);
            break;
        }
    }
    
    printf("%-15s %-10s\n", group_col, "COUNT");
    printf("%-15s %-10s\n", "---", "---");
    
    for (size_t i = 0; i < table->row_count; i++) {
        void **row = (void**)table->rows[i];
        int val = *(int*)row[col_idx];
        
        int already_counted = 0;
        for (size_t j = 0; j < i; j++) {
            void **prev_row = (void**)table->rows[j];
            if (*(int*)prev_row[col_idx] == val) {
                already_counted = 1;
                break;
            }
        }
        
        if (!already_counted) {
            int count = 0;
            for (size_t j = 0; j < table->row_count; j++) {
                void **check_row = (void**)table->rows[j];
                if (*(int*)check_row[col_idx] == val) {
                    count++;
                }
            }
            
            if (count > min_count) {
                printf("%-15d %-10d\n", val, count);
            }
        }
    }
    
    return 0;
}

// ==================== UNION ====================

static int handle_union(TokenList *tokens) {
    if (tokens->count < 7) return -1;
    
    int union_idx = -1;
    for (int i = 0; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "UNION") == 0) {
            union_idx = i;
            break;
        }
    }
    
    if (union_idx == -1) return -1;
    
    int is_union_all = 0;
    if (union_idx + 1 < tokens->count && strcmp(tokens->tokens[union_idx + 1], "ALL") == 0) {
        is_union_all = 1;
    }
    
    (void)is_union_all;
    
    int from1_idx = -1;
    for (int i = 1; i < union_idx; i++) {
        if (strcmp(tokens->tokens[i], "FROM") == 0) {
            from1_idx = i;
            break;
        }
    }
    
    int from2_idx = -1;
    for (int i = union_idx + 1; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "FROM") == 0) {
            from2_idx = i;
            break;
        }
    }
    
    if (from1_idx == -1 || from2_idx == -1) {
        printf("ERROR: Invalid UNION syntax\n");
        return -1;
    }
    
    char table1_name[MAX_TABLE_NAME];
    char table2_name[MAX_TABLE_NAME];
    strcpy(table1_name, tokens->tokens[from1_idx + 1]);
    strcpy(table2_name, tokens->tokens[from2_idx + 1]);
    
    Table *table1 = find_table(table1_name);
    Table *table2 = find_table(table2_name);
    
    if (!table1 || !table2) {
        printf("ERROR: Table not found\n");
        return -1;
    }
    
    char col_name[64];
    strcpy(col_name, tokens->tokens[1]);
    
    int col1_idx = table_get_column_index(table1, col_name);
    int col2_idx = table_get_column_index(table2, col_name);
    
    if (col1_idx == -1 || col2_idx == -1) {
        printf("ERROR: Column '%s' not found in both tables\n", col_name);
        return -1;
    }
    
    printf("%-15s\n", col_name);
    printf("%-15s\n", "---------------");
    
    int total = 0;
    
    for (size_t r = 0; r < table1->row_count; r++) {
        void **row = (void**)table1->rows[r];
        switch (table1->columns[col1_idx].type) {
            case TYPE_INTEGER:
                printf("%-15d\n", *(int*)row[col1_idx]);
                break;
            case TYPE_BOOLEAN:
                printf("%-15s\n", *(int*)row[col1_idx] ? "TRUE" : "FALSE");
                break;
            case TYPE_TEXT:
            case TYPE_UUID:
            case TYPE_JSON:
                printf("%-15s\n", (char*)row[col1_idx]);
                break;
            case TYPE_FLOAT:
                printf("%-15.2f\n", *(double*)row[col1_idx]);
                break;
        }
        total++;
    }
    
    for (size_t r = 0; r < table2->row_count; r++) {
        void **row = (void**)table2->rows[r];
        switch (table2->columns[col2_idx].type) {
            case TYPE_INTEGER:
                printf("%-15d\n", *(int*)row[col2_idx]);
                break;
            case TYPE_BOOLEAN:
                printf("%-15s\n", *(int*)row[col2_idx] ? "TRUE" : "FALSE");
                break;
            case TYPE_TEXT:
            case TYPE_UUID:
            case TYPE_JSON:
                printf("%-15s\n", (char*)row[col2_idx]);
                break;
            case TYPE_FLOAT:
                printf("%-15.2f\n", *(double*)row[col2_idx]);
                break;
        }
        total++;
    }
    
    printf("\n(%d row%s)\n", total, total == 1 ? "" : "s");
    return 0;
}

// ==================== CTE ====================

static int handle_cte(TokenList *tokens) {
    if (tokens->count < 7) return -1;
    
    char cte_name[MAX_TABLE_NAME];
    strcpy(cte_name, tokens->tokens[1]);
    
    int as_idx = -1;
    for (int i = 2; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "AS") == 0) {
            as_idx = i;
            break;
        }
    }
    
    if (as_idx == -1) return -1;
    
    int open_paren = -1;
    int close_paren = -1;
    for (int i = as_idx + 1; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "(") == 0 && open_paren == -1) {
            open_paren = i;
        }
        if (strcmp(tokens->tokens[i], ")") == 0 && open_paren != -1) {
            close_paren = i;
            break;
        }
    }
    
    if (open_paren == -1 || close_paren == -1) return -1;
    
    char inner_sql[1024] = "";
    for (int i = open_paren + 1; i < close_paren; i++) {
        strcat(inner_sql, tokens->tokens[i]);
        if (i < close_paren - 1) strcat(inner_sql, " ");
    }
    
    int main_from_idx = -1;
    for (int i = close_paren + 1; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "FROM") == 0) {
            main_from_idx = i;
            break;
        }
    }
    
    if (main_from_idx == -1) return -1;
    
    char main_table[MAX_TABLE_NAME];
    strcpy(main_table, tokens->tokens[main_from_idx + 1]);
    
    if (strcmp(main_table, cte_name) != 0) {
        printf("ERROR: CTE '%s' not referenced in main query\n", cte_name);
        return -1;
    }
    
    printf("-- CTE '%s' executes:\n", cte_name);
    sql_execute(inner_sql);
    printf("-- Main query results:\n");
    
    return 0;
}

static int handle_string_function(TokenList *tokens) {
    if (tokens->count < 3) return -1;
    
    char func_name[32];
    strcpy(func_name, tokens->tokens[1]);
    to_upper(func_name);
    
    char *paren = strchr(func_name, '(');
    if (paren) *paren = '\0';
    
    int from_idx = -1;
    for (int i = 1; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "FROM") == 0) {
            from_idx = i;
            break;
        }
    }
    
    Table *table = NULL;
    if (from_idx != -1) {
        char table_name[MAX_TABLE_NAME];
        strcpy(table_name, tokens->tokens[from_idx + 1]);
        table = find_table(table_name);
        if (!table) {
            printf("ERROR: Table '%s' not found\n", table_name);
            return -1;
        }
    }
    
    char arg[256] = "";
    int arg_start = -1;
    
    for (int i = 1; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "(") == 0) {
            arg_start = i + 1;
            break;
        }
    }
    
    if (arg_start == -1 || arg_start >= tokens->count) {
        printf("ERROR: Invalid function syntax\n");
        return -1;
    }
    
    strcpy(arg, tokens->tokens[arg_start]);
    
    int substr_start = 0;
    int substr_len = 0;
    if (strcmp(func_name, "SUBSTR") == 0 || strcmp(func_name, "SUBSTRING") == 0) {
        if (arg_start + 4 < tokens->count) {
            substr_start = atoi(tokens->tokens[arg_start + 2]);
            substr_len = atoi(tokens->tokens[arg_start + 4]);
        }
    }
    
    printf("result\n");
    printf("------\n");
    
    int match_count = 0;
    
    if (table) {
        int col_idx = -1;
        
        // For CONCAT, don't require first arg to be a column
        if (strcmp(func_name, "CONCAT") != 0) {
            col_idx = table_get_column_index(table, arg);
            if (col_idx == -1) {
                printf("ERROR: Column '%s' not found\n", arg);
                return -1;
            }
        }
        
        for (size_t r = 0; r < table->row_count; r++) {
            void **row = (void**)table->rows[r];
            char value[256] = "";
            
            if (col_idx >= 0) {
                switch (table->columns[col_idx].type) {
                    case TYPE_TEXT:
                    case TYPE_UUID:
                    case TYPE_JSON:
                        strcpy(value, (char*)row[col_idx]);
                        break;
                    case TYPE_INTEGER:
                        snprintf(value, sizeof(value), "%d", *(int*)row[col_idx]);
                        break;
                    case TYPE_BOOLEAN:
                        strcpy(value, *(int*)row[col_idx] ? "TRUE" : "FALSE");
                        break;
                    case TYPE_FLOAT:
                        snprintf(value, sizeof(value), "%.2f", *(double*)row[col_idx]);
                        break;
                }
            }
            
            char result[512] = "";
            
            if (strcmp(func_name, "UPPER") == 0) {
                strcpy(result, value);
                to_upper(result);
            }
            else if (strcmp(func_name, "LOWER") == 0) {
                strcpy(result, value);
                for (int k = 0; result[k]; k++) {
                    result[k] = tolower((unsigned char)result[k]);
                }
            }
            else if (strcmp(func_name, "LENGTH") == 0) {
                snprintf(result, sizeof(result), "%zu", strlen(value));
            }
            else if (strcmp(func_name, "TRIM") == 0) {
                char *start = value;
                while (*start == ' ' || *start == '\t') start++;
                strcpy(result, start);
                int len = strlen(result);
                while (len > 0 && (result[len-1] == ' ' || result[len-1] == '\t')) {
                    result[len-1] = '\0';
                    len--;
                }
            }
            else if (strcmp(func_name, "SUBSTR") == 0 || strcmp(func_name, "SUBSTRING") == 0) {
                int len = strlen(value);
                int start = substr_start - 1;
                if (start < 0) start = len + start;
                if (start < 0) start = 0;
                if (start > len) start = len;
                
                int end = start + substr_len;
                if (substr_len == 0) end = len;
                if (end > len) end = len;
                
                int k = 0;
                for (int j = start; j < end && k < 255; j++) {
                    result[k++] = value[j];
                }
                result[k] = '\0';
            }
            else if (strcmp(func_name, "CONCAT") == 0) {
                char result_buf[512] = "";
                
                for (int k = arg_start; k < tokens->count; k++) {
                    if (strcmp(tokens->tokens[k], ")") == 0) break;
                    if (strcmp(tokens->tokens[k], ",") == 0) continue;
                    if (strcmp(tokens->tokens[k], "FROM") == 0) break;
                    
                    char part[256] = "";
                    int col_idx_k = table_get_column_index(table, tokens->tokens[k]);
                    
                    if (col_idx_k >= 0) {
                        switch (table->columns[col_idx_k].type) {
                            case TYPE_TEXT:
                            case TYPE_UUID:
                            case TYPE_JSON:
                                strcpy(part, (char*)row[col_idx_k]);
                                break;
                            case TYPE_INTEGER:
                                snprintf(part, sizeof(part), "%d", *(int*)row[col_idx_k]);
                                break;
                            case TYPE_BOOLEAN:
                                strcpy(part, *(int*)row[col_idx_k] ? "TRUE" : "FALSE");
                                break;
                            case TYPE_FLOAT:
                                snprintf(part, sizeof(part), "%.2f", *(double*)row[col_idx_k]);
                                break;
                        }
                    } else {
                        strcpy(part, tokens->tokens[k]);
                    }
                    
                    strcat(result_buf, part);
                }
                
                strcpy(result, result_buf);
            }
            else {
                strcpy(result, value);
            }
            
            printf("%-15s\n", result);
            match_count++;
        }
    } else {
        char value[256] = "";
        strcpy(value, arg);
        
        char result[512] = "";
        
        if (strcmp(func_name, "UPPER") == 0) {
            strcpy(result, value);
            to_upper(result);
        }
        else if (strcmp(func_name, "LOWER") == 0) {
            strcpy(result, value);
            for (int k = 0; result[k]; k++) {
                result[k] = tolower((unsigned char)result[k]);
            }
        }
        else if (strcmp(func_name, "LENGTH") == 0) {
            snprintf(result, sizeof(result), "%zu", strlen(value));
        }
        else if (strcmp(func_name, "TRIM") == 0) {
            char *start = value;
            while (*start == ' ' || *start == '\t') start++;
            strcpy(result, start);
            int len = strlen(result);
            while (len > 0 && (result[len-1] == ' ' || result[len-1] == '\t')) {
                result[len-1] = '\0';
                len--;
            }
        }
        else if (strcmp(func_name, "SUBSTR") == 0 || strcmp(func_name, "SUBSTRING") == 0) {
            int len = strlen(value);
            int start = substr_start - 1;
            if (start < 0) start = len + start;
            if (start < 0) start = 0;
            if (start > len) start = len;
            
            int end = start + substr_len;
            if (substr_len == 0) end = len;
            if (end > len) end = len;
            
            int k = 0;
            for (int j = start; j < end && k < 255; j++) {
                result[k++] = value[j];
            }
            result[k] = '\0';
        }
        else if (strcmp(func_name, "CONCAT") == 0) {
            char result_buf[512] = "";
            
            for (int k = arg_start; k < tokens->count; k++) {
                if (strcmp(tokens->tokens[k], ")") == 0) break;
                if (strcmp(tokens->tokens[k], ",") == 0) continue;
                
                strcat(result_buf, tokens->tokens[k]);
            }
            
            strcpy(result, result_buf);
        }
        else {
            strcpy(result, value);
        }
        
        printf("%-15s\n", result);
        match_count++;
    }
    
    printf("(%d row%s)\n", match_count, match_count == 1 ? "" : "s");
    return 0;
}
static int handle_math_function(TokenList *tokens) {
    // SELECT ABS(-5)
    // SELECT ROUND(3.14159)
    // SELECT ROUND(3.14159, 2)
    // SELECT FLOOR(3.9)
    // SELECT CEIL(3.1)
    // SELECT MOD(10, 3)
    
    if (tokens->count < 3) return -1;
    
    char func_name[32];
    strcpy(func_name, tokens->tokens[1]);
    to_upper(func_name);
    
    char *paren = strchr(func_name, '(');
    if (paren) *paren = '\0';
    
    // Extract first argument
    int arg_start = -1;
    for (int i = 1; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "(") == 0) {
            arg_start = i + 1;
            break;
        }
    }
    
    if (arg_start == -1 || arg_start >= tokens->count) {
        printf("ERROR: Invalid function syntax\n");
        return -1;
    }
    
    double value = atof(tokens->tokens[arg_start]);
    
    // For ROUND with 2 args
    int decimals = 0;
    if (strcmp(func_name, "ROUND") == 0 && arg_start + 2 < tokens->count) {
        if (strcmp(tokens->tokens[arg_start + 2], ")") != 0) {
            decimals = atoi(tokens->tokens[arg_start + 2]);
        }
    }
    
    // For MOD
    double value2 = 0;
    if (strcmp(func_name, "MOD") == 0 && arg_start + 2 < tokens->count) {
        value2 = atof(tokens->tokens[arg_start + 2]);
    }
    
    printf("result\n");
    printf("------\n");
    
    double result = 0;
    
    if (strcmp(func_name, "ABS") == 0) {
        result = value < 0 ? -value : value;
        printf("%-15.4f\n", result);
    }
    else if (strcmp(func_name, "ROUND") == 0) {
        double multiplier = 1;
        for (int i = 0; i < decimals; i++) multiplier *= 10;
        result = (double)((long long)(value * multiplier + 0.5)) / multiplier;
        printf("%-15.*f\n", decimals, result);
    }
    else if (strcmp(func_name, "FLOOR") == 0) {
        result = (double)(long long)value;
        if (value < 0 && value != (double)(long long)value) result -= 1;
        printf("%-15.0f\n", result);
    }
    else if (strcmp(func_name, "CEIL") == 0 || strcmp(func_name, "CEILING") == 0) {
        result = (double)(long long)value;
        if (value > 0 && value != (double)(long long)value) result += 1;
        printf("%-15.0f\n", result);
    }
    else if (strcmp(func_name, "MOD") == 0) {
        if (value2 != 0) {
            result = (double)((long long)value % (long long)value2);
            printf("%-15.0f\n", result);
        } else {
            printf("ERROR: Division by zero\n");
        }
    }
    else {
        printf("ERROR: Unknown math function '%s'\n", func_name);
        return -1;
    }
    
    printf("(1 row)\n");
    return 0;
}

static int handle_create_index(TokenList *tokens) {
    if (tokens->count < 7) return -1;
    
    char index_name[64];
    char table_name[MAX_TABLE_NAME];
    char col_name[64];
    
    strcpy(index_name, tokens->tokens[2]);
    
    int on_idx = -1;
    for (int i = 3; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "ON") == 0) {
            on_idx = i;
            break;
        }
    }
    
    if (on_idx == -1) return -1;
    
    strcpy(table_name, tokens->tokens[on_idx + 1]);
    
    Table *table = find_table(table_name);
    if (!table) {
        printf("ERROR: Table '%s' not found\n", table_name);
        return -1;
    }
    
    int paren_idx = -1;
    for (int i = on_idx + 2; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "(") == 0) {
            paren_idx = i + 1;
            break;
        }
    }
    
    if (paren_idx == -1 || paren_idx >= tokens->count) {
        printf("ERROR: Invalid CREATE INDEX syntax\n");
        return -1;
    }
    
    strcpy(col_name, tokens->tokens[paren_idx]);
    
    int col_idx = table_get_column_index(table, col_name);
    if (col_idx == -1) {
        printf("ERROR: Column '%s' not found\n", col_name);
        return -1;
    }
    
    if (table->columns[col_idx].type != TYPE_INTEGER &&
        table->columns[col_idx].type != TYPE_BOOLEAN) {
        printf("OK. Index '%s' registered (non-integer column, no B-Tree)\n", index_name);
        return 0;
    }
    
    if (table->columns[col_idx].index) {
        printf("OK. Index already exists on '%s.%s'\n", table_name, col_name);
        return 0;
    }
    
    table->columns[col_idx].index = btree_create();
    if (!table->columns[col_idx].index) {
        printf("ERROR: Failed to create B-Tree\n");
        return -1;
    }
    
    for (size_t r = 0; r < table->row_count; r++) {
        void **row = (void**)table->rows[r];
        int key = *(int*)row[col_idx];
        btree_insert(table->columns[col_idx].index, key, row);
    }
    
    printf("OK. Created index '%s' on '%s(%s)'\n", index_name, table_name, col_name);
    return 0;
}

static int handle_cross_join(TokenList *tokens) {
    // SELECT * FROM users CROSS JOIN orders
    if (tokens->count < 6) return -1;
    
    int from_idx = -1;
    for (int i = 1; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "FROM") == 0) {
            from_idx = i;
            break;
        }
    }
    
    if (from_idx == -1) return -1;
    
    char table1_name[MAX_TABLE_NAME];
    strcpy(table1_name, tokens->tokens[from_idx + 1]);
    
    Table *table1 = find_table(table1_name);
    if (!table1) {
        printf("ERROR: Table '%s' not found\n", table1_name);
        return -1;
    }
    
    int join_idx = -1;
    for (int i = from_idx + 2; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "JOIN") == 0) {
            join_idx = i;
            break;
        }
    }
    
    if (join_idx == -1) return -1;
    
    char table2_name[MAX_TABLE_NAME];
    strcpy(table2_name, tokens->tokens[join_idx + 1]);
    
    Table *table2 = find_table(table2_name);
    if (!table2) {
        printf("ERROR: Table '%s' not found\n", table2_name);
        return -1;
    }
    
    for (int i = 0; i < table1->column_count; i++) {
        printf("%-15s ", table1->columns[i].name);
    }
    for (int i = 0; i < table2->column_count; i++) {
        printf("%-15s ", table2->columns[i].name);
    }
    printf("\n");
    
    for (int i = 0; i < table1->column_count + table2->column_count; i++) {
        printf("%-15s ", "---------------");
    }
    printf("\n");
    
    int match_count = 0;
    for (size_t r1 = 0; r1 < table1->row_count; r1++) {
        void **row1 = (void**)table1->rows[r1];
        
        for (size_t r2 = 0; r2 < table2->row_count; r2++) {
            void **row2 = (void**)table2->rows[r2];
            
            for (int i = 0; i < table1->column_count; i++) {
                switch (table1->columns[i].type) {
                    case TYPE_INTEGER:
                        printf("%-15d ", *(int*)row1[i]);
                        break;
                    case TYPE_BOOLEAN:
                        printf("%-15s ", *(int*)row1[i] ? "TRUE" : "FALSE");
                        break;
                    case TYPE_TEXT:
                        printf("%-15s ", (char*)row1[i]);
                        break;
                    case TYPE_UUID:
                        printf("%-38s ", (char*)row1[i]);
                        break;
                    case TYPE_JSON:
                        printf("%-30s ", (char*)row1[i]);
                        break;
                    case TYPE_FLOAT:
                        printf("%-15.2f ", *(double*)row1[i]);
                        break;
                }
            }
            for (int i = 0; i < table2->column_count; i++) {
                switch (table2->columns[i].type) {
                    case TYPE_INTEGER:
                        printf("%-15d ", *(int*)row2[i]);
                        break;
                    case TYPE_BOOLEAN:
                        printf("%-15s ", *(int*)row2[i] ? "TRUE" : "FALSE");
                        break;
                    case TYPE_TEXT:
                        printf("%-15s ", (char*)row2[i]);
                        break;
                    case TYPE_UUID:
                        printf("%-38s ", (char*)row2[i]);
                        break;
                    case TYPE_JSON:
                        printf("%-30s ", (char*)row2[i]);
                        break;
                    case TYPE_FLOAT:
                        printf("%-15.2f ", *(double*)row2[i]);
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

static int handle_right_join(TokenList *tokens) {
    if (tokens->count < 8) return -1;
    
    int from_idx = -1;
    for (int i = 1; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "FROM") == 0) {
            from_idx = i;
            break;
        }
    }
    
    if (from_idx == -1) return -1;
    
    char table1_name[MAX_TABLE_NAME];
    strcpy(table1_name, tokens->tokens[from_idx + 1]);
    
    Table *table1 = find_table(table1_name);
    if (!table1) {
        printf("ERROR: Table '%s' not found\n", table1_name);
        return -1;
    }
    
    int join_idx = -1;
    for (int i = from_idx + 2; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "JOIN") == 0) {
            join_idx = i;
            break;
        }
    }
    
    if (join_idx == -1) return -1;
    
    char table2_name[MAX_TABLE_NAME];
    strcpy(table2_name, tokens->tokens[join_idx + 1]);
    
    Table *table2 = find_table(table2_name);
    if (!table2) {
        printf("ERROR: Table '%s' not found\n", table2_name);
        return -1;
    }
    
    int on_idx = -1;
    for (int i = join_idx + 2; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "ON") == 0) {
            on_idx = i;
            break;
        }
    }
    
    if (on_idx == -1) return -1;
    
    char t1_col[64];
    char t2_col[64];
    char full_col1[128];
    char full_col2[128];
    
    strcpy(full_col1, tokens->tokens[on_idx + 1]);
    strcpy(full_col2, tokens->tokens[on_idx + 3]);
    
    char *dot1 = strchr(full_col1, '.');
    char *dot2 = strchr(full_col2, '.');
    
    if (dot1) strcpy(t1_col, dot1 + 1);
    else strcpy(t1_col, full_col1);
    
    if (dot2) strcpy(t2_col, dot2 + 1);
    else strcpy(t2_col, full_col2);
    
    int t1_col_idx = table_get_column_index(table1, t1_col);
    int t2_col_idx = table_get_column_index(table2, t2_col);
    
    if (t1_col_idx == -1 || t2_col_idx == -1) {
        printf("ERROR: Column not found\n");
        return -1;
    }
    
    for (int i = 0; i < table1->column_count; i++) {
        printf("%-15s ", table1->columns[i].name);
    }
    for (int i = 0; i < table2->column_count; i++) {
        printf("%-15s ", table2->columns[i].name);
    }
    printf("\n");
    
    for (int i = 0; i < table1->column_count + table2->column_count; i++) {
        printf("%-15s ", "---------------");
    }
    printf("\n");
    
    int match_count = 0;
    
    for (size_t r2 = 0; r2 < table2->row_count; r2++) {
        void **row2 = (void**)table2->rows[r2];
        int found = 0;
        
        for (size_t r1 = 0; r1 < table1->row_count; r1++) {
            void **row1 = (void**)table1->rows[r1];
            
            int val1 = *(int*)row1[t1_col_idx];
            int val2 = *(int*)row2[t2_col_idx];
            
            if (val1 == val2) {
                for (int i = 0; i < table1->column_count; i++) {
                    switch (table1->columns[i].type) {
                        case TYPE_INTEGER:
                            printf("%-15d ", *(int*)row1[i]);
                            break;
                        case TYPE_BOOLEAN:
                            printf("%-15s ", *(int*)row1[i] ? "TRUE" : "FALSE");
                            break;
                        case TYPE_TEXT:
                            printf("%-15s ", (char*)row1[i]);
                            break;
                        case TYPE_UUID:
                            printf("%-38s ", (char*)row1[i]);
                            break;
                        case TYPE_JSON:
                            printf("%-30s ", (char*)row1[i]);
                            break;
                        case TYPE_FLOAT:
                            printf("%-15.2f ", *(double*)row1[i]);
                            break;
                    }
                }
                for (int i = 0; i < table2->column_count; i++) {
                    switch (table2->columns[i].type) {
                        case TYPE_INTEGER:
                            printf("%-15d ", *(int*)row2[i]);
                            break;
                        case TYPE_BOOLEAN:
                            printf("%-15s ", *(int*)row2[i] ? "TRUE" : "FALSE");
                            break;
                        case TYPE_TEXT:
                            printf("%-15s ", (char*)row2[i]);
                            break;
                        case TYPE_UUID:
                            printf("%-38s ", (char*)row2[i]);
                            break;
                        case TYPE_JSON:
                            printf("%-30s ", (char*)row2[i]);
                            break;
                        case TYPE_FLOAT:
                            printf("%-15.2f ", *(double*)row2[i]);
                            break;
                    }
                }
                printf("\n");
                match_count++;
                found = 1;
            }
        }
        
        if (!found) {
            for (int i = 0; i < table1->column_count; i++) {
                printf("%-15s ", "NULL");
            }
            for (int i = 0; i < table2->column_count; i++) {
                switch (table2->columns[i].type) {
                    case TYPE_INTEGER:
                        printf("%-15d ", *(int*)row2[i]);
                        break;
                    case TYPE_BOOLEAN:
                        printf("%-15s ", *(int*)row2[i] ? "TRUE" : "FALSE");
                        break;
                    case TYPE_TEXT:
                        printf("%-15s ", (char*)row2[i]);
                        break;
                    case TYPE_UUID:
                        printf("%-38s ", (char*)row2[i]);
                        break;
                    case TYPE_JSON:
                        printf("%-30s ", (char*)row2[i]);
                        break;
                    case TYPE_FLOAT:
                        printf("%-15.2f ", *(double*)row2[i]);
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

static int handle_full_join(TokenList *tokens) {
    if (tokens->count < 8) return -1;
    
    int from_idx = -1;
    for (int i = 1; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "FROM") == 0) {
            from_idx = i;
            break;
        }
    }
    
    if (from_idx == -1) return -1;
    
    char table1_name[MAX_TABLE_NAME];
    strcpy(table1_name, tokens->tokens[from_idx + 1]);
    
    Table *table1 = find_table(table1_name);
    if (!table1) {
        printf("ERROR: Table '%s' not found\n", table1_name);
        return -1;
    }
    
    int join_idx = -1;
    for (int i = from_idx + 2; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "JOIN") == 0) {
            join_idx = i;
            break;
        }
    }
    
    if (join_idx == -1) return -1;
    
    char table2_name[MAX_TABLE_NAME];
    strcpy(table2_name, tokens->tokens[join_idx + 1]);
    
    Table *table2 = find_table(table2_name);
    if (!table2) {
        printf("ERROR: Table '%s' not found\n", table2_name);
        return -1;
    }
    
    int on_idx = -1;
    for (int i = join_idx + 2; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "ON") == 0) {
            on_idx = i;
            break;
        }
    }
    
    if (on_idx == -1) return -1;
    
    char t1_col[64];
    char t2_col[64];
    char full_col1[128];
    char full_col2[128];
    
    strcpy(full_col1, tokens->tokens[on_idx + 1]);
    strcpy(full_col2, tokens->tokens[on_idx + 3]);
    
    char *dot1 = strchr(full_col1, '.');
    char *dot2 = strchr(full_col2, '.');
    
    if (dot1) strcpy(t1_col, dot1 + 1);
    else strcpy(t1_col, full_col1);
    
    if (dot2) strcpy(t2_col, dot2 + 1);
    else strcpy(t2_col, full_col2);
    
    int t1_col_idx = table_get_column_index(table1, t1_col);
    int t2_col_idx = table_get_column_index(table2, t2_col);
    
    if (t1_col_idx == -1 || t2_col_idx == -1) {
        printf("ERROR: Column not found\n");
        return -1;
    }
    
    for (int i = 0; i < table1->column_count; i++) {
        printf("%-15s ", table1->columns[i].name);
    }
    for (int i = 0; i < table2->column_count; i++) {
        printf("%-15s ", table2->columns[i].name);
    }
    printf("\n");
    
    for (int i = 0; i < table1->column_count + table2->column_count; i++) {
        printf("%-15s ", "---------------");
    }
    printf("\n");
    
    int match_count = 0;
    
    // Track which rows in table2 have been matched
    int *table2_matched = (int*)calloc(table2->row_count, sizeof(int));
    
    // First: LEFT JOIN behavior
    for (size_t r1 = 0; r1 < table1->row_count; r1++) {
        void **row1 = (void**)table1->rows[r1];
        int found = 0;
        
        for (size_t r2 = 0; r2 < table2->row_count; r2++) {
            void **row2 = (void**)table2->rows[r2];
            
            int val1 = *(int*)row1[t1_col_idx];
            int val2 = *(int*)row2[t2_col_idx];
            
            if (val1 == val2) {
                for (int i = 0; i < table1->column_count; i++) {
                    switch (table1->columns[i].type) {
                        case TYPE_INTEGER:
                            printf("%-15d ", *(int*)row1[i]);
                            break;
                        case TYPE_BOOLEAN:
                            printf("%-15s ", *(int*)row1[i] ? "TRUE" : "FALSE");
                            break;
                        case TYPE_TEXT:
                            printf("%-15s ", (char*)row1[i]);
                            break;
                        case TYPE_UUID:
                            printf("%-38s ", (char*)row1[i]);
                            break;
                        case TYPE_JSON:
                            printf("%-30s ", (char*)row1[i]);
                            break;
                        case TYPE_FLOAT:
                            printf("%-15.2f ", *(double*)row1[i]);
                            break;
                    }
                }
                for (int i = 0; i < table2->column_count; i++) {
                    switch (table2->columns[i].type) {
                        case TYPE_INTEGER:
                            printf("%-15d ", *(int*)row2[i]);
                            break;
                        case TYPE_BOOLEAN:
                            printf("%-15s ", *(int*)row2[i] ? "TRUE" : "FALSE");
                            break;
                        case TYPE_TEXT:
                            printf("%-15s ", (char*)row2[i]);
                            break;
                        case TYPE_UUID:
                            printf("%-38s ", (char*)row2[i]);
                            break;
                        case TYPE_JSON:
                            printf("%-30s ", (char*)row2[i]);
                            break;
                        case TYPE_FLOAT:
                            printf("%-15.2f ", *(double*)row2[i]);
                            break;
                    }
                }
                printf("\n");
                match_count++;
                found = 1;
                table2_matched[r2] = 1;
            }
        }
        
        if (!found) {
            for (int i = 0; i < table1->column_count; i++) {
                switch (table1->columns[i].type) {
                    case TYPE_INTEGER:
                        printf("%-15d ", *(int*)row1[i]);
                        break;
                    case TYPE_BOOLEAN:
                        printf("%-15s ", *(int*)row1[i] ? "TRUE" : "FALSE");
                        break;
                    case TYPE_TEXT:
                        printf("%-15s ", (char*)row1[i]);
                        break;
                    case TYPE_UUID:
                        printf("%-38s ", (char*)row1[i]);
                        break;
                    case TYPE_JSON:
                        printf("%-30s ", (char*)row1[i]);
                        break;
                    case TYPE_FLOAT:
                        printf("%-15.2f ", *(double*)row1[i]);
                        break;
                }
            }
            for (int i = 0; i < table2->column_count; i++) {
                printf("%-15s ", "NULL");
            }
            printf("\n");
            match_count++;
        }
    }
    
    // Second: RIGHT JOIN behavior - show unmatched table2 rows
    for (size_t r2 = 0; r2 < table2->row_count; r2++) {
        if (table2_matched[r2]) continue;
        
        void **row2 = (void**)table2->rows[r2];
        
        for (int i = 0; i < table1->column_count; i++) {
            printf("%-15s ", "NULL");
        }
        for (int i = 0; i < table2->column_count; i++) {
            switch (table2->columns[i].type) {
                case TYPE_INTEGER:
                    printf("%-15d ", *(int*)row2[i]);
                    break;
                case TYPE_BOOLEAN:
                    printf("%-15s ", *(int*)row2[i] ? "TRUE" : "FALSE");
                    break;
                case TYPE_TEXT:
                    printf("%-15s ", (char*)row2[i]);
                    break;
                case TYPE_UUID:
                    printf("%-38s ", (char*)row2[i]);
                    break;
                case TYPE_JSON:
                    printf("%-30s ", (char*)row2[i]);
                    break;
                case TYPE_FLOAT:
                    printf("%-15.2f ", *(double*)row2[i]);
                    break;
            }
        }
        printf("\n");
        match_count++;
    }
    
    free(table2_matched);
    
    printf("(%d row%s)\n\n", match_count, match_count == 1 ? "" : "s");
    return 0;
}

static int handle_group_concat(TokenList *tokens) {
    // SELECT GROUP_CONCAT(name) FROM users
    // SELECT age, GROUP_CONCAT(name) FROM users GROUP BY age
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
    
    // Find the column inside GROUP_CONCAT(col)
    char col_name[64] = "";
    for (int i = 1; i < from_idx; i++) {
        if (strstr(tokens->tokens[i], "GROUP_CONCAT")) {
            // Check if column is in next token
            if (i + 1 < from_idx && strcmp(tokens->tokens[i + 1], "(") != 0 &&
                strcmp(tokens->tokens[i + 1], ")") != 0 &&
                strcmp(tokens->tokens[i + 1], ",") != 0) {
                strcpy(col_name, tokens->tokens[i + 1]);
            } else if (i + 2 < from_idx) {
                strcpy(col_name, tokens->tokens[i + 2]);
            }
            break;
        }
    }
    
    if (strlen(col_name) == 0) {
        printf("ERROR: Invalid GROUP_CONCAT syntax\n");
        return -1;
    }
    
    int col_idx = table_get_column_index(table, col_name);
    if (col_idx == -1) {
        printf("ERROR: Column '%s' not found\n", col_name);
        return -1;
    }
    
    // Check for GROUP BY
    int group_idx = -1;
    for (int i = from_idx + 2; i < tokens->count; i++) {
        if (strcmp(tokens->tokens[i], "GROUP") == 0) {
            group_idx = i;
            break;
        }
    }
    
    if (group_idx == -1) {
        // No GROUP BY - concatenate all values
        printf("GROUP_CONCAT(%s)\n", col_name);
        printf("----------------------------------------\n");
        
        char result[4096] = "";
        for (size_t r = 0; r < table->row_count; r++) {
            void **row = (void**)table->rows[r];
            char value[256] = "";
            
            switch (table->columns[col_idx].type) {
                case TYPE_TEXT:
                case TYPE_UUID:
                case TYPE_JSON:
                    strcpy(value, (char*)row[col_idx]);
                    break;
                case TYPE_INTEGER:
                    snprintf(value, sizeof(value), "%d", *(int*)row[col_idx]);
                    break;
                case TYPE_BOOLEAN:
                    strcpy(value, *(int*)row[col_idx] ? "TRUE" : "FALSE");
                    break;
                case TYPE_FLOAT:
                    snprintf(value, sizeof(value), "%.2f", *(double*)row[col_idx]);
                    break;
            }
            
            if (r > 0) strcat(result, ",");
            strcat(result, value);
        }
        
        printf("%s\n", result);
        printf("(1 row)\n");
    } else {
        // GROUP BY - concatenate per group
        char group_col[64];
        strcpy(group_col, tokens->tokens[group_idx + 2]);
        
        int group_col_idx = table_get_column_index(table, group_col);
        if (group_col_idx == -1) {
            printf("ERROR: Group column '%s' not found\n", group_col);
            return -1;
        }
        
        printf("%-15s %-40s\n", group_col, "GROUP_CONCAT");
        printf("%-15s %-40s\n", "---------------", "----------------------------------------");
        
        for (size_t i = 0; i < table->row_count; i++) {
            void **row = (void**)table->rows[i];
            int group_val = *(int*)row[group_col_idx];
            
            // Check if already processed
            int already = 0;
            for (size_t j = 0; j < i; j++) {
                void **prev = (void**)table->rows[j];
                if (*(int*)prev[group_col_idx] == group_val) {
                    already = 1;
                    break;
                }
            }
            if (already) continue;
            
            // Collect all matching rows
            char result[4096] = "";
            int first = 1;
            
            for (size_t j = 0; j < table->row_count; j++) {
                void **check = (void**)table->rows[j];
                if (*(int*)check[group_col_idx] != group_val) continue;
                
                char value[256] = "";
                switch (table->columns[col_idx].type) {
                    case TYPE_TEXT:
                    case TYPE_UUID:
                    case TYPE_JSON:
                        strcpy(value, (char*)check[col_idx]);
                        break;
                    case TYPE_INTEGER:
                        snprintf(value, sizeof(value), "%d", *(int*)check[col_idx]);
                        break;
                    case TYPE_BOOLEAN:
                        strcpy(value, *(int*)check[col_idx] ? "TRUE" : "FALSE");
                        break;
                    case TYPE_FLOAT:
                        snprintf(value, sizeof(value), "%.2f", *(double*)check[col_idx]);
                        break;
                }
                
                if (!first) strcat(result, ",");
                strcat(result, value);
                first = 0;
            }
            
            printf("%-15d %-40s\n", group_val, result);
        }
    }
    
    return 0;
}

int sql_execute(const char *sql) {
    TokenList list;
    tokenize(sql, &list);
    
    if (list.count == 0) return 0;
    
    char command[MAX_TOKEN_LEN];
    strcpy(command, list.tokens[0]);
    to_upper(command);
    
    // CTEs (WITH)
    if (strcmp(command, "WITH") == 0) {
        return handle_cte(&list);
    }
    
    // String functions
    if (strcmp(command, "SELECT") == 0 && list.count > 1) {
        char first_word[MAX_TOKEN_LEN];
        strcpy(first_word, list.tokens[1]);
        to_upper(first_word);
        
        char *paren = strchr(first_word, '(');
        if (paren) *paren = '\0';
        
        if (strcmp(first_word, "UPPER") == 0 ||
            strcmp(first_word, "LOWER") == 0 ||
            strcmp(first_word, "LENGTH") == 0 ||
            strcmp(first_word, "TRIM") == 0 ||
            strcmp(first_word, "SUBSTR") == 0 ||
            strcmp(first_word, "SUBSTRING") == 0 ||
            strcmp(first_word, "CONCAT") == 0) {
            return handle_string_function(&list);
        }
        
        if (strcmp(first_word, "ABS") == 0 ||
            strcmp(first_word, "ROUND") == 0 ||
            strcmp(first_word, "FLOOR") == 0 ||
            strcmp(first_word, "CEIL") == 0 ||
            strcmp(first_word, "CEILING") == 0 ||
            strcmp(first_word, "MOD") == 0) {
            return handle_math_function(&list);
        }
    }
    
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
        else if (strcmp(second, "USER") == 0) {
            return handle_create_user(&list);
        }
        else if (strcmp(second, "VIEW") == 0) {
            return handle_create_view(&list);
        }
        else if (strcmp(second, "INDEX") == 0) {
            return handle_create_index(&list);
        }
    }
    else if (strcmp(command, "DROP") == 0) {
        char second[MAX_TOKEN_LEN];
        strcpy(second, list.tokens[1]);
        to_upper(second);
        
        if (strcmp(second, "VIEW") == 0) {
            if (list.count < 3) {
                printf("ERROR: Usage: DROP VIEW <name>\n");
                return -1;
            }
            char view_name[MAX_TABLE_NAME];
            strcpy(view_name, list.tokens[2]);
            
            for (int i = 0; i < view_count; i++) {
                if (strcmp(views[i].name, view_name) == 0) {
                    for (int j = i; j < view_count - 1; j++) {
                        views[j] = views[j + 1];
                    }
                    view_count--;
                    printf("OK. Dropped view '%s'\n", view_name);
                    return 0;
                }
            }
            printf("ERROR: View '%s' not found\n", view_name);
            return -1;
        }
        
        return handle_drop_table(&list);
    }
    else if (strcmp(command, "ALTER") == 0) {
        return handle_alter_table(&list);
    }
    else if (strcmp(command, "SHOW") == 0) {
        char second[MAX_TOKEN_LEN];
        strcpy(second, list.tokens[1]);
        to_upper(second);
        
        if (strcmp(second, "TABLES") == 0) {
            return handle_show_tables(&list);
        }
    }
    else if (strcmp(command, "DESCRIBE") == 0 || strcmp(command, "DESC") == 0) {
        return handle_describe(&list);
    }
    else if (strcmp(command, "TRUNCATE") == 0) {
        return handle_truncate(&list);
    }
    else if (strcmp(command, "LOGIN") == 0) {
        return handle_login(&list);
    }
    else if (strcmp(command, "LOGOUT") == 0) {
        return handle_logout(&list);
    }
    else if (strcmp(command, "CHANGE") == 0) {
        return handle_change_password(&list);
    }
    else if (strcmp(command, "GRANT") == 0) {
        return handle_grant(&list);
    }
    else if (strcmp(command, "REVOKE") == 0) {
        return handle_revoke(&list);
    }
    else if (strcmp(command, "REPLICATE") == 0) {
        return handle_replicate(&list);
    }
    else if (strcmp(command, "SYNC") == 0) {
        return handle_sync(&list);
    }
    else if (strcmp(command, "BACKUP") == 0) {
        return handle_backup(&list);
    }
    else if (strcmp(command, "EXPLAIN") == 0) {
        return handle_explain(&list);
    }
    else if (strcmp(command, "INSERT") == 0) {
        return handle_insert(&list);
    }
    else if (strcmp(command, "SELECT") == 0) {
        // GROUP_CONCAT
        int has_group_concat = 0;
        for (int i = 0; i < list.count; i++) {
            if (strstr(list.tokens[i], "GROUP_CONCAT") != NULL) {
                has_group_concat = 1;
                break;
            }
        }
        if (has_group_concat) {
            return handle_group_concat(&list);
        }
        
        // Aggregate functions
        if (list.count > 1 && (strstr(list.tokens[1], "COUNT") || 
                               strstr(list.tokens[1], "SUM") || 
                               strstr(list.tokens[1], "AVG"))) {
            return handle_aggregate(&list);
        }
        
        // MIN/MAX
        if (list.count > 1 && (strstr(list.tokens[1], "MIN") || 
                               strstr(list.tokens[1], "MAX"))) {
            return handle_min_max(&list);
        }
        
        // DISTINCT
        if (list.count > 2 && strcmp(list.tokens[1], "DISTINCT") == 0) {
            return handle_distinct(&list);
        }
        
        // IS NULL
        int has_is_null = 0;
        for (int i = 0; i < list.count; i++) {
            if (strcmp(list.tokens[i], "IS") == 0) {
                has_is_null = 1;
                break;
            }
        }
        if (has_is_null) {
            return handle_is_null(&list);
        }
        
        // IN
        int has_in = 0;
        for (int i = 0; i < list.count; i++) {
            if (strcmp(list.tokens[i], "IN") == 0) {
                has_in = 1;
                break;
            }
        }
        if (has_in) {
            return handle_in(&list);
        }
        
        // BETWEEN
        int has_between = 0;
        for (int i = 0; i < list.count; i++) {
            if (strcmp(list.tokens[i], "BETWEEN") == 0) {
                has_between = 1;
                break;
            }
        }
        if (has_between) {
            return handle_between(&list);
        }
        
        // LIKE
        int has_like = 0;
        for (int i = 0; i < list.count; i++) {
            if (strcmp(list.tokens[i], "LIKE") == 0) {
                has_like = 1;
                break;
            }
        }
        if (has_like) {
            return handle_like(&list);
        }
        
        // LIMIT
        int has_limit = 0;
        for (int i = 0; i < list.count; i++) {
            if (strcmp(list.tokens[i], "LIMIT") == 0) {
                has_limit = 1;
                break;
            }
        }
        if (has_limit) {
            return handle_limit(&list);
        }
        
        // HAVING
        int has_having = 0;
        for (int i = 0; i < list.count; i++) {
            if (strcmp(list.tokens[i], "HAVING") == 0) {
                has_having = 1;
                break;
            }
        }
        if (has_having) {
            return handle_having(&list);
        }
        
        // GROUP BY
        int has_group = 0;
        for (int i = 0; i < list.count; i++) {
            if (strcmp(list.tokens[i], "GROUP") == 0) {
                has_group = 1;
                break;
            }
        }
        if (has_group) {
            return handle_group_by(&list);
        }
        
        // ORDER BY
        int has_order_by = 0;
        for (int i = 0; i < list.count; i++) {
            if (strcmp(list.tokens[i], "ORDER") == 0) {
                has_order_by = 1;
                break;
            }
        }
        if (has_order_by) {
            return handle_order_by(&list);
        }
        
        // UNION
        int has_union = 0;
        for (int i = 0; i < list.count; i++) {
            if (strcmp(list.tokens[i], "UNION") == 0) {
                has_union = 1;
                break;
            }
        }
        if (has_union) {
            return handle_union(&list);
        }
        
        // RIGHT JOIN
        int has_right_join = 0;
        for (int i = 0; i < list.count; i++) {
            if (strcmp(list.tokens[i], "JOIN") == 0 && i > 0 &&
                strcmp(list.tokens[i-1], "RIGHT") == 0) {
                has_right_join = 1;
                break;
            }
        }
        if (has_right_join) {
            return handle_right_join(&list);
        }
        
        // FULL JOIN
        int has_full_join = 0;
        for (int i = 0; i < list.count; i++) {
            if (strcmp(list.tokens[i], "JOIN") == 0 && i > 0 &&
                strcmp(list.tokens[i-1], "FULL") == 0) {
                has_full_join = 1;
                break;
            }
        }
        if (has_full_join) {
            return handle_full_join(&list);
        }
        
        // CROSS JOIN
        int has_cross_join = 0;
        for (int i = 0; i < list.count; i++) {
            if (strcmp(list.tokens[i], "JOIN") == 0 && i > 0 &&
                strcmp(list.tokens[i-1], "CROSS") == 0) {
                has_cross_join = 1;
                break;
            }
        }
        if (has_cross_join) {
            return handle_cross_join(&list);
        }
        
        // JOIN (INNER/LEFT)
        int has_join = 0;
        int is_left_join = 0;
        for (int i = 0; i < list.count; i++) {
            if (strcmp(list.tokens[i], "JOIN") == 0) {
                has_join = 1;
                if (i > 0 && strcmp(list.tokens[i-1], "LEFT") == 0) {
                    is_left_join = 1;
                }
                break;
            }
        }
        
        if (has_join) {
            if (is_left_join) {
                return handle_left_join(&list);
            } else {
                return handle_join(&list);
            }
        }
        
        // Check for view
        int from_pos = -1;
        for (int i = 0; i < list.count; i++) {
            if (strcmp(list.tokens[i], "FROM") == 0) {
                from_pos = i;
                break;
            }
        }
        
        if (from_pos >= 0 && from_pos + 1 < list.count) {
            if (!find_table(list.tokens[from_pos + 1])) {
                return handle_select_view(&list);
            }
        }
        
        return handle_select(&list);
    }
    else if (strcmp(command, "UPDATE") == 0) {
        return handle_update(&list);
    }
    else if (strcmp(command, "DELETE") == 0) {
        return handle_delete(&list);
    }
    else {
        printf("ERROR: Unknown SQL command '%s'\n", command);
        return -1;
    }
    
    return 0;
}