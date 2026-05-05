#include "message.h"
#include "globals.h"
#include "broadcast.h"
#include "../common/protocol.h"
#include "../common/types.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

/* In-memory message storage */
#define MAX_MESSAGES 1000
static Message g_messages[MAX_MESSAGES];
static int g_message_count = 0;

void handle_message_send(int socket_fd, const char *packet)
{
    if (!packet) return;

    /* Parse: MESSAGE_SEND|room_id|content */
    User *sender = sessions_find_by_fd(socket_fd);
    if (!sender) return;

    char room_id_str[21] = {0};
    char content[501] = {0};

    const char *pipe1 = strchr(packet, '|');
    if (!pipe1) return;

    const char *pipe2 = strchr(pipe1 + 1, '|');
    if (!pipe2) return;

    int len = pipe2 - pipe1 - 1;
    if (len <= 0 || len >= 20) return;
    strncpy(room_id_str, pipe1 + 1, len);

    int room_id = atoi(room_id_str);

    len = strchr(pipe2 + 1, '\n') ? strchr(pipe2 + 1, '\n') - pipe2 - 1 : strlen(pipe2 + 1);
    if (len <= 0 || len >= 500) return;
    strncpy(content, pipe2 + 1, len);

    /* Store message */
    if (g_message_count < MAX_MESSAGES) {
        Message *msg = &g_messages[g_message_count];
        msg->msg_id = g_message_count + 1;
        msg->room_id = room_id;
        strcpy(msg->from_id, sender->user_id);
        strcpy(msg->to_id, "");
        strcpy(msg->content, content);
        msg->is_deleted = 0;
        msg->read_count = 0;
        msg->timestamp = time(NULL);
        g_message_count++;
    }

    /* Broadcast to room members */
    char response[1024];
    snprintf(response, sizeof(response), "MESSAGE_RECV|%d|%s|%s\n",
             room_id, sender->user_id, content);

    int *member_fds = malloc(100 * sizeof(int));
    if (member_fds) {
        int count = sessions_get_room_members(room_id, member_fds, 100);
        for (int i = 0; i < count; i++) {
            send(member_fds[i], response, strlen(response), 0);
        }
        free(member_fds);
    }
}

void handle_message_delete(int socket_fd, const char *packet)
{
    (void)socket_fd;
    (void)packet;
    /* Message deletion not implemented in Phase 0 */
}

void handle_message_edit(int socket_fd, const char *packet)
{
    (void)socket_fd;
    (void)packet;
    /* Message editing not implemented in Phase 0 */
}

void handle_message_reply(int socket_fd, const char *packet)
{
    (void)socket_fd;
    (void)packet;
    /* Message reply not implemented in Phase 0 */
}

void handle_message_search(int socket_fd, const char *packet)
{
    (void)socket_fd;
    (void)packet;
    /* Message search not implemented in Phase 0 */
}

void handle_whisper(int socket_fd, const char *packet)
{
    (void)socket_fd;
    (void)packet;
    /* Whisper not implemented in Phase 0 */
}
