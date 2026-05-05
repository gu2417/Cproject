#ifndef TYPES_H
#define TYPES_H

#include <time.h>

/* User structure */
typedef struct
{
    char user_id[21];
    char password_hash[65];
    char nickname[21];
    char profile_msg[101];
    int online_status;
    int socket_fd;
    int dnd_mode;
} User;

/* Chat room structure */
typedef struct
{
    int room_id;
    char room_name[51];
    char owner_id[21];
    int is_open;
    int member_count;
    int member_fds[64];
} ChatRoom;

/* Message structure */
typedef struct
{
    int msg_id;
    int room_id;
    char from_id[21];
    char to_id[21];
    char content[501];
    int is_deleted;
    int read_count;
    time_t timestamp;
} Message;

/* Friend entry structure */
typedef struct
{
    char user_id_1[21];
    char user_id_2[21];
    int status;
    time_t created_at;
} FriendEntry;

/* Notification structure */
typedef struct
{
    int notif_id;
    int type;
    char from_id[21];
    int room_id;
    char content[101];
    int is_read;
    time_t created_at;
} Notification;

#endif // TYPES_H
