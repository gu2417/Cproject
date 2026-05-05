#include "user_store.h"
#include "db.h"
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
static int g_db_checked = 0;
static int g_db_available = 0;

static void db_escape(MYSQL *conn, const char *src, char *dst, size_t dst_size)
{
    if (!src || !dst || dst_size == 0) {
        return;
    }

    if (!conn) {
        snprintf(dst, dst_size, "%s", src);
        return;
    }

    mysql_real_escape_string(conn, dst, src, (unsigned long)strlen(src));
}

static int db_has_rows(MYSQL *conn, const char *query)
{
    MYSQL_RES *res;
    int found = 0;

    if (mysql_query(conn, query) != 0) {
        fprintf(stderr, "[DB] query failed: %s\n", mysql_error(conn));
        return 0;
    }

    res = mysql_store_result(conn);
    if (!res) {
        fprintf(stderr, "[DB] result failed: %s\n", mysql_error(conn));
        return 0;
    }

    found = mysql_num_rows(res) > 0;
    mysql_free_result(res);
    return found;
}

static void db_check_available(void)
{
    MYSQL *conn;

    if (g_db_checked) {
        return;
    }

    g_db_checked = 1;
    conn = db_connect();
    if (conn) {
        g_db_available = 1;
        db_disconnect(conn);
        fprintf(stderr, "[DB] MySQL connected - using chat_db\n");
    } else {
        g_db_available = 0;
        fprintf(stderr, "[DB] unavailable - using in-memory test users\n");
    }
}

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
    db_check_available();
}

int user_exists(const char *user_id)
{
    if (!user_id) return 0;

    if (g_db_available) {
        MYSQL *conn = db_connect();
        if (conn) {
            char esc_user[43] = {0};
            char query[256];
            int found;

            db_escape(conn, user_id, esc_user, sizeof(esc_user));
            snprintf(query, sizeof(query),
                     "SELECT 1 FROM users WHERE user_id='%s' LIMIT 1", esc_user);
            found = db_has_rows(conn, query);
            db_disconnect(conn);
            return found;
        }
    }
    
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

    if (g_db_available) {
        MYSQL *conn = db_connect();
        if (conn) {
            char esc_user[43] = {0};
            char esc_pass[203] = {0};
            char query[512];
            int verified;

            db_escape(conn, user_id, esc_user, sizeof(esc_user));
            db_escape(conn, password, esc_pass, sizeof(esc_pass));
            snprintf(query, sizeof(query),
                     "SELECT 1 FROM users "
                     "WHERE user_id='%s' "
                     "AND (password_hash=SHA2('%s', 256) OR password_hash='%s') "
                     "LIMIT 1",
                     esc_user, esc_pass, esc_pass);
            verified = db_has_rows(conn, query);
            db_disconnect(conn);
            return verified;
        }
    }
    
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

    if (g_db_available) {
        MYSQL *conn = db_connect();
        if (conn) {
            char esc_user[43] = {0};
            char query[256];
            MYSQL_RES *res;
            MYSQL_ROW row;

            db_escape(conn, user_id, esc_user, sizeof(esc_user));
            snprintf(query, sizeof(query),
                     "SELECT nickname, profile_msg FROM users WHERE user_id='%s' LIMIT 1",
                     esc_user);

            if (mysql_query(conn, query) == 0 && (res = mysql_store_result(conn)) != NULL) {
                row = mysql_fetch_row(res);
                if (row) {
                    snprintf(nickname, 21, "%s", row[0] ? row[0] : "");
                    snprintf(profile_msg, 101, "%s", row[1] ? row[1] : "");
                    mysql_free_result(res);
                    db_disconnect(conn);
                    return 0;
                }
                mysql_free_result(res);
            }
            db_disconnect(conn);
        }
    }
    
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
    if (!theme) return -1;

    if (g_db_available && user_id) {
        MYSQL *conn = db_connect();
        if (conn) {
            char esc_user[43] = {0};
            char query[256];
            MYSQL_RES *res;
            MYSQL_ROW row;

            db_escape(conn, user_id, esc_user, sizeof(esc_user));
            snprintf(query, sizeof(query),
                     "SELECT theme FROM user_settings WHERE user_id='%s' LIMIT 1",
                     esc_user);

            if (mysql_query(conn, query) == 0 && (res = mysql_store_result(conn)) != NULL) {
                row = mysql_fetch_row(res);
                if (row && row[0]) {
                    snprintf(theme, 10, "%s", row[0]);
                    mysql_free_result(res);
                    db_disconnect(conn);
                    return 0;
                }
                mysql_free_result(res);
            }
            db_disconnect(conn);
        }
    }

    strcpy(theme, "light");
    return 0;
}

int settings_update(const char *user_id, const char *theme)
{
    if (!user_id || !theme) return -1;

    if (g_db_available) {
        MYSQL *conn = db_connect();
        if (conn) {
            char esc_user[43] = {0};
            char esc_theme[23] = {0};
            char query[512];
            int ok;

            db_escape(conn, user_id, esc_user, sizeof(esc_user));
            db_escape(conn, theme, esc_theme, sizeof(esc_theme));
            snprintf(query, sizeof(query),
                     "INSERT INTO user_settings (user_id, theme) VALUES ('%s', '%s') "
                     "ON DUPLICATE KEY UPDATE theme=VALUES(theme)",
                     esc_user, esc_theme);
            ok = mysql_query(conn, query) == 0;
            if (!ok) {
                fprintf(stderr, "[DB] settings update failed: %s\n", mysql_error(conn));
            }
            db_disconnect(conn);
            return ok ? 0 : -1;
        }
    }

    return 0;
}

int profile_update(const char *user_id, const char *nickname, const char *profile_msg)
{
    if (!user_id || !nickname || !profile_msg) return -1;

    if (g_db_available) {
        MYSQL *conn = db_connect();
        if (conn) {
            char esc_user[43] = {0};
            char esc_nick[43] = {0};
            char esc_msg[203] = {0};
            char query[512];
            int ok;

            db_escape(conn, user_id, esc_user, sizeof(esc_user));
            db_escape(conn, nickname, esc_nick, sizeof(esc_nick));
            db_escape(conn, profile_msg, esc_msg, sizeof(esc_msg));
            snprintf(query, sizeof(query),
                     "UPDATE users SET nickname='%s', profile_msg='%s' WHERE user_id='%s'",
                     esc_nick, esc_msg, esc_user);
            ok = mysql_query(conn, query) == 0 && mysql_affected_rows(conn) > 0;
            if (!ok) {
                fprintf(stderr, "[DB] profile update failed: %s\n", mysql_error(conn));
            }
            db_disconnect(conn);
            return ok ? 0 : -1;
        }
    }
    
    for (int i = 0; i < g_user_count; i++) {
        if (strcmp(g_user_db[i].user_id, user_id) == 0 && g_user_db[i].is_active) {
            strcpy(g_user_db[i].nickname, nickname);
            strcpy(g_user_db[i].profile_msg, profile_msg);
            return 0;
        }
    }
    return -1;
}
