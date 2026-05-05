#include "db.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Stub MySQL functions - database integration disabled for Phase 0 */

MYSQL *db_connect(void)
{
    fprintf(stderr, "[DB] MySQL not configured - using stub\n");
    return (MYSQL *)malloc(sizeof(int));  /* Return dummy pointer */
}

void db_disconnect(MYSQL *conn)
{
    if (conn) {
        free(conn);
    }
}

int db_prepare(MYSQL *conn, MYSQL_STMT **stmt, const char *query)
{
    (void)conn;
    (void)query;
    fprintf(stderr, "[DB] db_prepare stub called\n");
    *stmt = NULL;
    return 0;
}

int db_exec_insert(MYSQL_STMT *stmt)
{
    (void)stmt;
    fprintf(stderr, "[DB] db_exec_insert stub called\n");
    return 0;
}

int db_query_row(MYSQL_STMT *stmt, MYSQL_BIND *bind, int bind_count)
{
    (void)stmt;
    (void)bind;
    (void)bind_count;
    fprintf(stderr, "[DB] db_query_row stub called\n");
    return 0;
}

int db_query_rows(MYSQL_STMT *stmt, MYSQL_BIND *bind, int bind_count, char *buffer, int buffer_size)
{
    (void)stmt;
    (void)bind;
    (void)bind_count;
    (void)buffer;
    (void)buffer_size;
    fprintf(stderr, "[DB] db_query_rows stub called\n");
    return 0;
}
