#include "db.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

MYSQL *db_connect(void)
{
    MYSQL *conn = mysql_init(NULL);
    if (!conn) {
        fprintf(stderr, "[DB] mysql_init failed\n");
        return NULL;
    }

    unsigned int timeout = 3;
    mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    mysql_options(conn, MYSQL_OPT_READ_TIMEOUT, &timeout);
    mysql_options(conn, MYSQL_OPT_WRITE_TIMEOUT, &timeout);
    mysql_options(conn, MYSQL_SET_CHARSET_NAME, "utf8mb4");

    if (!mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASSWORD, DB_NAME, DB_PORT, NULL, 0)) {
        fprintf(stderr, "[DB] connect failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return NULL;
    }

    return conn;
}

void db_disconnect(MYSQL *conn)
{
    if (conn) {
        mysql_close(conn);
    }
}

int db_prepare(MYSQL *conn, MYSQL_STMT **stmt, const char *query)
{
    if (!conn || !stmt || !query) {
        return -1;
    }

    *stmt = mysql_stmt_init(conn);
    if (!*stmt) {
        fprintf(stderr, "[DB] mysql_stmt_init failed: %s\n", mysql_error(conn));
        return -1;
    }

    if (mysql_stmt_prepare(*stmt, query, (unsigned long)strlen(query)) != 0) {
        fprintf(stderr, "[DB] prepare failed: %s\n", mysql_stmt_error(*stmt));
        mysql_stmt_close(*stmt);
        *stmt = NULL;
        return -1;
    }

    return 0;
}

int db_exec_insert(MYSQL_STMT *stmt)
{
    if (!stmt) {
        return -1;
    }
    return mysql_stmt_execute(stmt) == 0 ? (int)mysql_stmt_insert_id(stmt) : -1;
}

int db_query_row(MYSQL_STMT *stmt, MYSQL_BIND *bind, int bind_count)
{
    if (!stmt || !bind || bind_count <= 0) {
        return -1;
    }
    if (mysql_stmt_bind_result(stmt, bind) != 0 || mysql_stmt_execute(stmt) != 0) {
        return -1;
    }
    return mysql_stmt_fetch(stmt) == 0 ? 1 : 0;
}

int db_query_rows(MYSQL_STMT *stmt, MYSQL_BIND *bind, int bind_count, char *buffer, int buffer_size)
{
    (void)stmt;
    (void)bind;
    (void)bind_count;
    (void)buffer;
    (void)buffer_size;
    return -1;
}
