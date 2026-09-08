#include "auth_storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int auth_save(AuthSystem *auth) {
    if (!auth) return -1;
    
    FILE *fp = fopen(AUTH_FILE, "wb");
    if (!fp) return -1;
    
    uint32_t magic = AUTH_MAGIC;
    fwrite(&magic, sizeof(uint32_t), 1, fp);
    
    uint32_t count = (uint32_t)auth->user_count;
    fwrite(&count, sizeof(uint32_t), 1, fp);
    
    for (int i = 0; i < auth->user_count; i++) {
        User *user = &auth->users[i];
        
        // Write username
        uint32_t name_len = (uint32_t)strlen(user->username);
        fwrite(&name_len, sizeof(uint32_t), 1, fp);
        fwrite(user->username, sizeof(char), name_len, fp);
        
        // Write password hash
        uint32_t hash_len = (uint32_t)strlen(user->password_hash);
        fwrite(&hash_len, sizeof(uint32_t), 1, fp);
        fwrite(user->password_hash, sizeof(char), hash_len, fp);
        
        // Write flags
        fwrite(&user->is_active, sizeof(int), 1, fp);
        fwrite(&user->failed_attempts, sizeof(int), 1, fp);
        fwrite(&user->is_locked, sizeof(int), 1, fp);
        fwrite(&user->must_change_password, sizeof(int), 1, fp);
    }
    
    fclose(fp);
    return 0;
}

int auth_load(AuthSystem *auth) {
    if (!auth) return -1;
    
    FILE *fp = fopen(AUTH_FILE, "rb");
    if (!fp) return 0; // No file yet
    
    uint32_t magic;
    if (fread(&magic, sizeof(uint32_t), 1, fp) != 1) {
        fclose(fp);
        return -1;
    }
    
    if (magic != AUTH_MAGIC) {
        fclose(fp);
        return -1;
    }
    
    uint32_t count;
    if (fread(&count, sizeof(uint32_t), 1, fp) != 1) {
        fclose(fp);
        return -1;
    }
    
    auth->user_count = 0;
    
    for (uint32_t i = 0; i < count && i < MAX_USERS; i++) {
        User *user = &auth->users[i];
        
        // Read username
        uint32_t name_len;
        if (fread(&name_len, sizeof(uint32_t), 1, fp) != 1) break;
        if (fread(user->username, sizeof(char), name_len, fp) != name_len) break;
        user->username[name_len] = '\0';
        
        // Read password hash
        uint32_t hash_len;
        if (fread(&hash_len, sizeof(uint32_t), 1, fp) != 1) break;
        if (fread(user->password_hash, sizeof(char), hash_len, fp) != hash_len) break;
        user->password_hash[hash_len] = '\0';
        
        // Read flags
        fread(&user->is_active, sizeof(int), 1, fp);
        fread(&user->failed_attempts, sizeof(int), 1, fp);
        fread(&user->is_locked, sizeof(int), 1, fp);
        fread(&user->must_change_password, sizeof(int), 1, fp);
        
        auth->user_count++;
    }
    
    fclose(fp);
    return 0;
}