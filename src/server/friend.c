#include "friend.h"
#include "globals.h"
#include "broadcast.h"
#include "../common/protocol.h"
#include "../common/types.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/* In-memory friend list storage */
#define MAX_FRIENDSHIPS 1000
static struct {
    char user_a[21];
    char user_b[21];
    int status;  /* 0=pending, 1=accepted, 2=blocked */
    time_t created_at;
} g_friendships[MAX_FRIENDSHIPS];

static int g_friendship_count = 0;

void handle_friend_add(int socket_fd, const char *packet)
{
    if (!packet) return;

    User *requester = sessions_find_by_fd(socket_fd);
    if (!requester) return;

    /* Parse: FRIEND_ADD|target_user_id */
    char target_user[21] = {0};
    const char *pipe1 = strchr(packet, '|');
    if (!pipe1) return;

    int len = strchr(pipe1 + 1, '\n') ? strchr(pipe1 + 1, '\n') - pipe1 - 1 : strlen(pipe1 + 1);
    if (len <= 0 || len >= 20) return;
    strncpy(target_user, pipe1 + 1, len);

    /* Add friendship */
    if (g_friendship_count < MAX_FRIENDSHIPS) {
        strcpy(g_friendships[g_friendship_count].user_a, requester->user_id);
        strcpy(g_friendships[g_friendship_count].user_b, target_user);
        g_friendships[g_friendship_count].status = 0;  /* pending */
        g_friendships[g_friendship_count].created_at = time(NULL);
        g_friendship_count++;

        char response[256];
        snprintf(response, sizeof(response), "FRIEND_ADD_RES|1\n");
        send(socket_fd, response, strlen(response), 0);
    }
}

void handle_friend_accept(int socket_fd, const char *packet)
{
    if (!packet) return;

    User *user = sessions_find_by_fd(socket_fd);
    if (!user) return;

    /* Parse: FRIEND_ACCEPT|requester_user_id */
    char requester[21] = {0};
    const char *pipe1 = strchr(packet, '|');
    if (!pipe1) return;

    int len = strchr(pipe1 + 1, '\n') ? strchr(pipe1 + 1, '\n') - pipe1 - 1 : strlen(pipe1 + 1);
    if (len <= 0 || len >= 20) return;
    strncpy(requester, pipe1 + 1, len);

    /* Find and accept friendship */
    for (int i = 0; i < g_friendship_count; i++) {
        if (strcmp(g_friendships[i].user_a, requester) == 0 &&
            strcmp(g_friendships[i].user_b, user->user_id) == 0 &&
            g_friendships[i].status == 0) {
            g_friendships[i].status = 1;  /* accepted */

            char response[256];
            snprintf(response, sizeof(response), "FRIEND_ACCEPT_RES|1\n");
            send(socket_fd, response, strlen(response), 0);
            return;
        }
    }

    char response[256];
    snprintf(response, sizeof(response), "FRIEND_ACCEPT_RES|0\n");
    send(socket_fd, response, strlen(response), 0);
}

void handle_friend_reject(int socket_fd, const char *packet)
{
    (void)socket_fd;
    (void)packet;
    /* Friend rejection not implemented in Phase 0 */
}

void handle_friend_block(int socket_fd, const char *packet)
{
    if (!packet) return;

    User *user = sessions_find_by_fd(socket_fd);
    if (!user) return;

    /* Parse: FRIEND_BLOCK|target_user_id */
    char target_user[21] = {0};
    const char *pipe1 = strchr(packet, '|');
    if (!pipe1) return;

    int len = strchr(pipe1 + 1, '\n') ? strchr(pipe1 + 1, '\n') - pipe1 - 1 : strlen(pipe1 + 1);
    if (len <= 0 || len >= 20) return;
    strncpy(target_user, pipe1 + 1, len);

    /* Add block */
    if (g_friendship_count < MAX_FRIENDSHIPS) {
        strcpy(g_friendships[g_friendship_count].user_a, user->user_id);
        strcpy(g_friendships[g_friendship_count].user_b, target_user);
        g_friendships[g_friendship_count].status = 2;  /* blocked */
        g_friendships[g_friendship_count].created_at = time(NULL);
        g_friendship_count++;

        char response[256];
        snprintf(response, sizeof(response), "FRIEND_BLOCK_RES|1\n");
        send(socket_fd, response, strlen(response), 0);
    }
}

void handle_friend_list(int socket_fd, const char *packet)
{
    (void)packet;

    User *user = sessions_find_by_fd(socket_fd);
    if (!user) return;

    /* Build friend list */
    char friend_list[1024] = "FRIEND_LIST|";
    int count = 0;

    for (int i = 0; i < g_friendship_count; i++) {
        if (g_friendships[i].status == 1) {  /* accepted only */
            if (strcmp(g_friendships[i].user_a, user->user_id) == 0) {
                if (count > 0) strcat(friend_list, ";");
                strcat(friend_list, g_friendships[i].user_b);
                count++;
            } else if (strcmp(g_friendships[i].user_b, user->user_id) == 0) {
                if (count > 0) strcat(friend_list, ";");
                strcat(friend_list, g_friendships[i].user_a);
                count++;
            }
        }
    }
    strcat(friend_list, "\n");

    send(socket_fd, friend_list, strlen(friend_list), 0);
}

int friend_is_blocked(const char *user_a, const char *user_b)
{
    if (!user_a || !user_b) return 0;

    for (int i = 0; i < g_friendship_count; i++) {
        if (g_friendships[i].status == 2) {  /* blocked */
            if ((strcmp(g_friendships[i].user_a, user_a) == 0 &&
                 strcmp(g_friendships[i].user_b, user_b) == 0) ||
                (strcmp(g_friendships[i].user_a, user_b) == 0 &&
                 strcmp(g_friendships[i].user_b, user_a) == 0)) {
                return 1;
            }
        }
    }
    return 0;
}
