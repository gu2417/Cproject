#include "room.h"
#include "globals.h"
#include "broadcast.h"
#include "../common/protocol.h"
#include "../common/types.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/* In-memory room storage */
#define MAX_ROOMS 100
static ChatRoom g_rooms[MAX_ROOMS];
static int g_room_count = 0;

void room_store_init(void)
{
    if (g_room_count > 0) return;

    /* Create default rooms */
    for (int i = 0; i < 3; i++) {
        ChatRoom *room = &g_rooms[i];
        room->room_id = i + 1;
        snprintf(room->room_name, sizeof(room->room_name), "Room %d", i + 1);
        strcpy(room->owner_id, "alice");
        room->is_open = 1;
        room->member_count = 0;
    }
    g_room_count = 3;
}

void handle_room_create(int socket_fd, const char *packet)
{
    if (!packet) return;

    User *creator = sessions_find_by_fd(socket_fd);
    if (!creator) return;

    /* Parse: ROOM_CREATE|room_name */
    char room_name[51] = {0};
    const char *pipe1 = strchr(packet, '|');
    if (!pipe1) return;

    int len = strchr(pipe1 + 1, '\n') ? strchr(pipe1 + 1, '\n') - pipe1 - 1 : strlen(pipe1 + 1);
    if (len <= 0 || len >= 50) return;
    strncpy(room_name, pipe1 + 1, len);

    if (g_room_count < MAX_ROOMS) {
        ChatRoom *room = &g_rooms[g_room_count];
        room->room_id = g_room_count + 1;
        strcpy(room->room_name, room_name);
        strcpy(room->owner_id, creator->user_id);
        room->is_open = 1;
        room->member_count = 1;
        room->member_fds[0] = socket_fd;
        g_room_count++;

        char response[256];
        snprintf(response, sizeof(response), "ROOM_CREATE_RES|1|%d\n", room->room_id);
        notify_user(socket_fd, response);
    }
}

void handle_room_join(int socket_fd, const char *packet)
{
    if (!packet) return;

    User *user = sessions_find_by_fd(socket_fd);
    if (!user) return;

    /* Parse: ROOM_JOIN|room_id */
    char room_id_str[21] = {0};
    const char *pipe1 = strchr(packet, '|');
    if (!pipe1) return;

    int len = strchr(pipe1 + 1, '\n') ? strchr(pipe1 + 1, '\n') - pipe1 - 1 : strlen(pipe1 + 1);
    if (len <= 0 || len >= 20) return;
    strncpy(room_id_str, pipe1 + 1, len);

    int room_id = atoi(room_id_str);

    for (int i = 0; i < g_room_count; i++) {
        if (g_rooms[i].room_id == room_id && g_rooms[i].is_open) {
            if (g_rooms[i].member_count < 64) {
                g_rooms[i].member_fds[g_rooms[i].member_count++] = socket_fd;

                char response[256];
                snprintf(response, sizeof(response), "ROOM_JOIN_RES|1|%d\n", room_id);
                send(socket_fd, response, strlen(response), 0);

                /* Notify other members */
                char notify[256];
                snprintf(notify, sizeof(notify), "USER_JOINED|%d|%s\n", room_id, user->user_id);
                for (int j = 0; j < g_rooms[i].member_count - 1; j++) {
                    send(g_rooms[i].member_fds[j], notify, strlen(notify), 0);
                }
                return;
            }
        }
    }

    char response[256];
    snprintf(response, sizeof(response), "ROOM_JOIN_RES|0\n");
    send(socket_fd, response, strlen(response), 0);
}

void handle_room_leave(int socket_fd, const char *packet)
{
    (void)packet;

    User *user = sessions_find_by_fd(socket_fd);
    if (!user) return;

    for (int i = 0; i < g_room_count; i++) {
        for (int j = 0; j < g_rooms[i].member_count; j++) {
            if (g_rooms[i].member_fds[j] == socket_fd) {
                /* Remove member */
                for (int k = j; k < g_rooms[i].member_count - 1; k++) {
                    g_rooms[i].member_fds[k] = g_rooms[i].member_fds[k + 1];
                }
                g_rooms[i].member_count--;

                /* Notify other members */
                char notify[256];
                snprintf(notify, sizeof(notify), "USER_LEFT|%d|%s\n", g_rooms[i].room_id, user->user_id);
                for (int k = 0; k < g_rooms[i].member_count; k++) {
                    notify_user(g_rooms[i].member_fds[k], notify);
                }
                return;
            }
        }
    }
}

void handle_room_invite(int socket_fd, const char *packet)
{
    (void)socket_fd;
    (void)packet;
    /* Room invitation not implemented in Phase 0 */
}

void handle_room_kick(int socket_fd, const char *packet)
{
    (void)socket_fd;
    (void)packet;
    /* Room kick not implemented in Phase 0 */
}

void handle_room_set_notice(int socket_fd, const char *packet)
{
    (void)socket_fd;
    (void)packet;
    /* Room notice not implemented in Phase 0 */
}

void handle_room_grant_admin(int socket_fd, const char *packet)
{
    (void)socket_fd;
    (void)packet;
    /* Admin grant not implemented in Phase 0 */
}
