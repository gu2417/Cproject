#include "user_store.h"
#include "../common/utils.h"
#include <string.h>
#include <stdio.h>

/* In-memory user database */
#define MAX_USERS 100
static struct {
    char user_id[21];
    char password_hash[65];
    char nickname[21];
    char profile_msg[101];
    int is_active;
} g_user_db[MAX_USERS];

static int g_user_count = 0;

/* Initialize user database with test data */
void user_store_init(void)
{
    if (g_user_count > 0) return;  /* Already initialized */

    /* Test user 1 */
    strcpy(g_user_db[0].user_id, "alice");
    strcpy(g_user_db[0].password_hash, "alice123");
    strcpy(g_user_db[0].nickname, "Alice");
    strcpy(g_user_db[0].profile_msg, "Hello, I'm Alice");
    g_user_db[0].is_active = 1;

    /* Test user 2 */
    strcpy(g_user_db[1].user_id, "bob");
    strcpy(g_user_db[1].password_hash, "bob123");
    strcpy(g_user_db[1].nickname, "Bob");
    strcpy(g_user_db[1].profile_msg, "Hi, I'm Bob");
    g_user_db[1].is_active = 1;

    /* Test user 3 */
    strcpy(g_user_db[2].user_id, "charlie");
    strcpy(g_user_db[2].password_hash, "charlie123");
    strcpy(g_user_db[2].nickname, "Charlie");
    strcpy(g_user_db[2].profile_msg, "Hey there!");
    g_user_db[2].is_active = 1;

    g_user_count = 3;
}

int user_exists(const char *user_id)
{
    if (!user_id) return 0;
    
    for (int i = 0; i < g_user_count; i++) {
        if (strcmp(g_user_db[i].user_id, user_id) == 0) {
            return g_user_db[i].is_active;
        }
    }
    return 0;
}

int user_verify_password(const char *user_id, const char *password)
{
    if (!user_id || !password) return 0;
    
    for (int i = 0; i < g_user_count; i++) {
        if (strcmp(g_user_db[i].user_id, user_id) == 0 && g_user_db[i].is_active) {
            return strcmp(g_user_db[i].password_hash, password) == 0;
        }
    }
    return 0;
}


int user_get_profile(const char *user_id, char *nickname, char *profile_msg)
{
    if (!user_id || !nickname || !profile_msg) return -1;
    
    for (int i = 0; i < g_user_count; i++) {
        if (strcmp(g_user_db[i].user_id, user_id) == 0 && g_user_db[i].is_active) {
            strcpy(nickname, g_user_db[i].nickname);
            strcpy(profile_msg, g_user_db[i].profile_msg);
            return 0;
        }
    }
    return -1;
}

int settings_get(const char *user_id, char *theme)
{
    (void)user_id;
    if (theme) strcpy(theme, "dark");  /* Default theme */
    return 0;
}

int settings_update(const char *user_id, const char *theme)
{
    (void)user_id;
    (void)theme;
    return 0;  /* Stub implementation */
}

int profile_update(const char *user_id, const char *nickname, const char *profile_msg)
{
    if (!user_id || !nickname || !profile_msg) return -1;
    
    for (int i = 0; i < g_user_count; i++) {
        if (strcmp(g_user_db[i].user_id, user_id) == 0 && g_user_db[i].is_active) {
            strcpy(g_user_db[i].nickname, nickname);
            strcpy(g_user_db[i].profile_msg, profile_msg);
            return 0;
        }
    }
    return -1;
}
