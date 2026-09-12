#include "auth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ==================== SHA-256 IMPLEMENTATION ====================

typedef struct {
    uint32_t state[8];
    uint64_t count;
    uint8_t buffer[64];
} SHA256_CTX;

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static void sha256_transform(SHA256_CTX *ctx, const uint8_t *data) {
    uint32_t a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];
    
    for (i = 0, j = 0; i < 16; ++i, j += 4)
        m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
    for (; i < 64; ++i)
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];
    
    a = ctx->state[0]; b = ctx->state[1]; c = ctx->state[2]; d = ctx->state[3];
    e = ctx->state[4]; f = ctx->state[5]; g = ctx->state[6]; h = ctx->state[7];
    
    for (i = 0; i < 64; ++i) {
        t1 = h + EP1(e) + CH(e, f, g) + K[i] + m[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    
    ctx->state[0] += a; ctx->state[1] += b; ctx->state[2] += c; ctx->state[3] += d;
    ctx->state[4] += e; ctx->state[5] += f; ctx->state[6] += g; ctx->state[7] += h;
}

static void sha256_init(SHA256_CTX *ctx) {
    ctx->count = 0;
    ctx->state[0] = 0x6a09e667; ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372; ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f; ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab; ctx->state[7] = 0x5be0cd19;
}

static void sha256_update(SHA256_CTX *ctx, const uint8_t *data, size_t len) {
    size_t i = ctx->count % 64;
    size_t fill = 64 - i;
    ctx->count += len;
    
    if (i && len >= fill) {
        memcpy(&ctx->buffer[i], data, fill);
        sha256_transform(ctx, ctx->buffer);
        data += fill;
        len -= fill;
        i = 0;
    }
    
    while (len >= 64) {
        sha256_transform(ctx, data);
        data += 64;
        len -= 64;
    }
    
    if (len) memcpy(&ctx->buffer[i], data, len);
}

static void sha256_final(SHA256_CTX *ctx, uint8_t hash[32]) {
    uint32_t i = ctx->count % 64;
    uint8_t pad[64] = {0x80};
    uint64_t bits = ctx->count * 8;
    uint8_t len_bytes[8];
    int j;
    
    for (j = 0; j < 8; j++) len_bytes[j] = (bits >> (56 - j * 8)) & 0xff;
    
    size_t pad_len = (i < 56) ? (56 - i) : (120 - i);
    sha256_update(ctx, pad, pad_len);
    sha256_update(ctx, len_bytes, 8);
    
    for (i = 0; i < 8; i++) {
        hash[i * 4] = (ctx->state[i] >> 24) & 0xff;
        hash[i * 4 + 1] = (ctx->state[i] >> 16) & 0xff;
        hash[i * 4 + 2] = (ctx->state[i] >> 8) & 0xff;
        hash[i * 4 + 3] = ctx->state[i] & 0xff;
    }
}

// ==================== HMAC-SHA256 ====================

static void hmac_sha256(const uint8_t *key, size_t key_len,
                        const uint8_t *data, size_t data_len,
                        uint8_t out[32]) {
    uint8_t k_ipad[64] = {0};
    uint8_t k_opad[64] = {0};
    uint8_t key_hash[32];
    uint8_t temp[32];
    SHA256_CTX ctx;
    
    if (key_len > 64) {
        sha256_init(&ctx);
        sha256_update(&ctx, key, key_len);
        sha256_final(&ctx, key_hash);
        key = key_hash;
        key_len = 32;
    }
    
    memcpy(k_ipad, key, key_len);
    memcpy(k_opad, key, key_len);
    for (int i = 0; i < 64; i++) {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }
    
    sha256_init(&ctx);
    sha256_update(&ctx, k_ipad, 64);
    sha256_update(&ctx, data, data_len);
    sha256_final(&ctx, temp);
    
    sha256_init(&ctx);
    sha256_update(&ctx, k_opad, 64);
    sha256_update(&ctx, temp, 32);
    sha256_final(&ctx, out);
}

// ==================== PBKDF2 ====================

static void pbkdf2(const char *password, const uint8_t *salt, size_t salt_len,
                   int iterations, uint8_t *output, size_t output_len) {
    uint8_t block[32];
    uint8_t u[32];
    uint8_t salt_block[64];
    uint32_t counter = 1;
    
    memcpy(salt_block, salt, salt_len);
    salt_block[salt_len] = (counter >> 24) & 0xff;
    salt_block[salt_len + 1] = (counter >> 16) & 0xff;
    salt_block[salt_len + 2] = (counter >> 8) & 0xff;
    salt_block[salt_len + 3] = counter & 0xff;
    
    hmac_sha256((const uint8_t*)password, strlen(password), salt_block, salt_len + 4, block);
    memcpy(u, block, 32);
    
    for (int i = 1; i < iterations; i++) {
        hmac_sha256((const uint8_t*)password, strlen(password), u, 32, u);
        for (int j = 0; j < 32; j++) block[j] ^= u[j];
    }
    
    memcpy(output, block, (output_len < 32) ? output_len : 32);
}

// ==================== CRYPTO RANDOM ====================

static void generate_salt(uint8_t *salt, size_t len) {
    static int seeded = 0;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = 1;
    }
    for (size_t i = 0; i < len; i++) {
        salt[i] = (uint8_t)(rand() & 0xff);
    }
}

void auth_generate_random_password(char *output, int length) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*";
    int charset_len = sizeof(charset) - 1;
    
    static int seeded = 0;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = 1;
    }
    
    output[0] = 'A' + (rand() % 26);
    output[1] = 'a' + (rand() % 26);
    output[2] = '0' + (rand() % 10);
    output[3] = charset[52 + (rand() % 10)];
    
    for (int i = 4; i < length - 1; i++) {
        output[i] = charset[rand() % charset_len];
    }
    
    output[length - 1] = '\0';
}

// ==================== AUTH IMPLEMENTATION ====================

AuthSystem *auth_create(void) {
    AuthSystem *auth = (AuthSystem*)malloc(sizeof(AuthSystem));
    if (!auth) return NULL;
    
    auth->user_count = 0;
    auth->current_user_index = -1;
    
    // Generate a random password for the default admin
    char random_password[32];
    auth_generate_random_password(random_password, 24);
    
    // Allow override via environment variable (useful for scripting)
    const char *env_pw = getenv("HEAVENDB_INITIAL_PASSWORD");
    const char *password_to_use = (env_pw && strlen(env_pw) > 0) ? env_pw : random_password;
    
    strcpy(auth->users[0].username, "admin");
    generate_salt(auth->users[0].salt, SALT_SIZE);
    pbkdf2(password_to_use, auth->users[0].salt, SALT_SIZE, PBKDF2_ITERATIONS, 
           auth->users[0].password_hash, HASH_SIZE);
    auth->users[0].is_active = 1;
    auth->users[0].failed_attempts = 0;
    auth->users[0].is_locked = 0;
    auth->users[0].must_change_password = 1;
    auth->user_count = 1;
    
    // Backup password to a file in user's home directory
    const char *home = getenv("USERPROFILE");
    if (!home) home = getenv("HOME");
    
    int wrote_to_file = 0;
    if (home) {
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s\\.heavendb_initial_password", home);
        
        FILE *pw_file = fopen(filepath, "w");
        if (pw_file) {
            fprintf(pw_file, "%s\n", password_to_use);
            fclose(pw_file);
            wrote_to_file = 1;
        }
    }
    
    // Print to STDERR so pipes don't swallow it
    fprintf(stderr, "\n");
    fprintf(stderr, "+==========================================================+\n");
    fprintf(stderr, "|           HeavenDB -- FIRST RUN SETUP                    |\n");
    fprintf(stderr, "+==========================================================+\n");
    fprintf(stderr, "|  Username: admin                                         |\n");
    fprintf(stderr, "|  Password: %-45s |\n", password_to_use);
    fprintf(stderr, "|                                                          |\n");
    fprintf(stderr, "|  !!! COPY THIS PASSWORD NOW -- it will NOT be shown !!!  |\n");
    fprintf(stderr, "|      again. Change it immediately after first login:     |\n");
    fprintf(stderr, "|                                                          |\n");
    fprintf(stderr, "|      CHANGE PASSWORD 'YourNewSecure456'                  |\n");
    fprintf(stderr, "+==========================================================+\n");
    
    if (wrote_to_file && home) {
        fprintf(stderr, "|  Backup saved to: ~/.heavendb_initial_password           |\n");
        fprintf(stderr, "|  (delete after copying)                                  |\n");
        fprintf(stderr, "+==========================================================+\n");
    }
    fprintf(stderr, "\n");
    
    return auth;
}

int auth_validate_password(const char *password) {
    if (!password) return 0;
    
    int len = strlen(password);
    if (len < 8) return 0;
    
    int has_digit = 0, has_upper = 0, has_lower = 0;
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
    
    if (!auth_validate_password(password)) {
        printf("ERROR: Password must be at least 8 characters with uppercase, lowercase, and a digit\n");
        return -2;
    }
    
    for (int i = 0; i < auth->user_count; i++) {
        if (strcmp(auth->users[i].username, username) == 0) {
            printf("ERROR: User '%s' already exists\n", username);
            return -1;
        }
    }
    
    User *new_user = &auth->users[auth->user_count];
    strncpy(new_user->username, username, MAX_USERNAME - 1);
    new_user->username[MAX_USERNAME - 1] = '\0';
    
    generate_salt(new_user->salt, SALT_SIZE);
    pbkdf2(password, new_user->salt, SALT_SIZE, PBKDF2_ITERATIONS,
           new_user->password_hash, HASH_SIZE);
    
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
            
            if (auth->users[i].is_locked) {
                printf("ERROR: Account locked. Too many failed attempts.\n");
                return -2;
            }
            
            uint8_t test_hash[HASH_SIZE];
            pbkdf2(password, auth->users[i].salt, SALT_SIZE, PBKDF2_ITERATIONS,
                   test_hash, HASH_SIZE);
            
            if (auth->users[i].is_active &&
                memcmp(auth->users[i].password_hash, test_hash, HASH_SIZE) == 0) {
                auth->users[i].failed_attempts = 0;
                auth->current_user_index = i;
                return 0;
            } else {
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
    
    if (!auth_validate_password(new_password)) {
        printf("ERROR: Password must be at least 8 characters with uppercase, lowercase, and a digit\n");
        return -2;
    }
    
    User *user = &auth->users[auth->current_user_index];
    generate_salt(user->salt, SALT_SIZE);
    pbkdf2(new_password, user->salt, SALT_SIZE, PBKDF2_ITERATIONS,
           user->password_hash, HASH_SIZE);
    user->must_change_password = 0;
    
    return 0;
}

void auth_logout(AuthSystem *auth) {
    if (auth) auth->current_user_index = -1;
}

int auth_is_logged_in(AuthSystem *auth) {
    return auth && auth->current_user_index >= 0;
}

const char *auth_current_user(AuthSystem *auth) {
    if (!auth || auth->current_user_index < 0) return NULL;
    return auth->users[auth->current_user_index].username;
}

void auth_destroy(AuthSystem *auth) {
    if (auth) free(auth);
}