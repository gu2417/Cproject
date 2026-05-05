#include "dm.h"
#include "globals.h"
#include "broadcast.h"
#include "friend.h"
#include "../common/protocol.h"
#include "../common/types.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

/* In-memory DM storage */
#define MAX_DMS 5000
static struct {
    int msg_id;
    char from_id[21];
    char to_id[21];
    char content[501];
    int is_read;
    time_t timestamp;
} g_dms[MAX_DMS];

static int g_dm_count = 0;

void handle_dm_send(int socket_fd, const char *packet)
{
    if (!packet) return;

    User *sender = sessions_find_by_fd(socket_fd);
    if (!sender) return;

    /* Parse: DM_SEND|target_user_id|content */
    char target_user[21] = {0};
    char content[501] = {0};

    const char *pipe1 = strchr(packet, '|');
    if (!pipe1) return;

    const char *pipe2 = strchr(pipe1 + 1, '|');
    if (!pipe2) return;

    int len = pipe2 - pipe1 - 1;
    if (len <= 0 || len >= 20) return;
    strncpy(target_user, pipe1 + 1, len);

    /* Check if blocked */
    if (friend_is_blocked(sender->user_id, target_user)) {
        char response[256];
        snprintf(response, sizeof(response), "DM_SEND_RES|0\n");
        send(socket_fd, response, strlen(response), 0);
        return;
    }

    len = strchr(pipe2 + 1, '\n') ? strchr(pipe2 + 1, '\n') - pipe2 - 1 : strlen(pipe2 + 1);
    if (len <= 0 || len >= 500) return;
    strncpy(content, pipe2 + 1, len);

    /* Store DM */
    if (g_dm_count < MAX_DMS) {
        g_dms[g_dm_count].msg_id = g_dm_count + 1;
        strcpy(g_dms[g_dm_count].from_id, sender->user_id);
        strcpy(g_dms[g_dm_count].to_id, target_user);
        strcpy(g_dms[g_dm_count].content, content);
        g_dms[g_dm_count].is_read = 0;
        g_dms[g_dm_count].timestamp = time(NULL);
        g_dm_count++;
    }

    /* Send to recipient if online */
    User *recipient = sessions_find_by_id(target_user);
    if (recipient) {
        char response[1024];
        snprintf(response, sizeof(response), "DM_RECV|%s|%s\n",
                 sender->user_id, content);
        send(recipient->socket_fd, response, strlen(response), 0);
    }

    char ack[256];
    snprintf(ack, sizeof(ack), "DM_SEND_RES|1\n");
    send(socket_fd, ack, strlen(ack), 0);
}

void handle_dm_history(int socket_fd, const char *packet)
{
    if (!packet) return;

    User *user = sessions_find_by_fd(socket_fd);
    if (!user) return;

    /* Parse: DM_HISTORY|other_user_id */
    char other_user[21] = {0};
    const char *pipe1 = strchr(packet, '|');
    if (!pipe1) return;

    int len = strchr(pipe1 + 1, '\n') ? strchr(pipe1 + 1, '\n') - pipe1 - 1 : strlen(pipe1 + 1);
    if (len <= 0 || len >= 20) return;
    strncpy(other_user, pipe1 + 1, len);

    /* Build history */
    char history[4096] = "DM_HISTORY|";
    int count = 0;

    for (int i = 0; i < g_dm_count; i++) {
        if ((strcmp(g_dms[i].from_id, user->user_id) == 0 && strcmp(g_dms[i].to_id, other_user) == 0) ||
            (strcmp(g_dms[i].from_id, other_user) == 0 && strcmp(g_dms[i].to_id, user->user_id) == 0)) {
            if (count > 0) strcat(history, ";");
            char entry[512];
            snprintf(entry, sizeof(entry), "%s:%s", g_dms[i].from_id, g_dms[i].content);
            strcat(history, entry);
            count++;
        }
    }
    strcat(history, "\n");

    send(socket_fd, history, strlen(history), 0);
}

void mark_dm_read(const char *msg_id, const char *reader_id)
{
    if (!msg_id || !reader_id) return;

    int id = atoi(msg_id);
    for (int i = 0; i < g_dm_count; i++) {
        if (g_dms[i].msg_id == id && strcmp(g_dms[i].to_id, reader_id) == 0) {
            g_dms[i].is_read = 1;
            return;
        }
    }
}
