#ifndef CONFIG_H
#define CONFIG_H

/* Server configuration */
#define DEFAULT_PORT            8080
#define MAX_CLIENTS             100
#define MAX_ROOMS               100
#define LISTEN_BACKLOG          5

/* Database configuration */
#define DB_HOST                 "localhost"
#define DB_USER                 "root"
#define DB_PASSWORD             ""
#define DB_NAME                 "chat_db"
#define DB_PORT                 3306

/* Protocol configuration */
#define RECV_BUFFER_SIZE        4096
#define SEND_BUFFER_SIZE        4096

/* Timeout configuration (seconds) */
#define KEEPALIVE_INTERVAL      30
#define SOCKET_READ_TIMEOUT     300
#define SOCKET_WRITE_TIMEOUT    60

#endif // CONFIG_H
