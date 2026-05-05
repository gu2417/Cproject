#ifndef DB_H
#define DB_H

#include <stdio.h>
#ifdef _WIN32
    #include <winsock2.h>
#endif
#include <mysql.h>

/* Database connection */
MYSQL *db_connect(void);
void db_disconnect(MYSQL *conn);

/* Query execution */
int db_prepare(MYSQL *conn, MYSQL_STMT **stmt, const char *query);
int db_exec_insert(MYSQL_STMT *stmt);
int db_query_row(MYSQL_STMT *stmt, MYSQL_BIND *bind, int bind_count);
int db_query_rows(MYSQL_STMT *stmt, MYSQL_BIND *bind, int bind_count, char *buffer, int buffer_size);

#endif // DB_H
