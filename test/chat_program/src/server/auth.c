#include "auth.h"
#include "db.h"
#include "globals.h"
#include "user_store.h"
#include "broadcast.h"
#include "../common/protocol.h"
#include "../common/types.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void handle_auth_req(int socket_fd, const char *packet)
{
    if (!packet) return;

    /* Parse: AUTH_REQ|user_id|password */
    char user_id[21] = {0};
    char password[101] = {0};
    
    const char *pipe1 = strchr(packet, '|');
    if (!pipe1) return;
    
    const char *pipe2 = strchr(pipe1 + 1, '|');
    if (!pipe2) return;

    int len1 = pipe1 - packet - strlen("AUTH_REQ");
    if (len1 <= 0 || len1 >= 20) return;
    strncpy(user_id, pipe1 + 1, pipe2 - pipe1 - 1);

    len1 = strchr(pipe2 + 1, '\n') ? strchr(pipe2 + 1, '\n') - pipe2 - 1 : strlen(pipe2 + 1);
    if (len1 <= 0 || len1 >= 100) return;
    strncpy(password, pipe2 + 1, len1);

    /* Verify credentials */
    if (!user_verify_password(user_id, password)) {
        char response[256];
        snprintf(response, sizeof(response), "AUTH_RES|0\n");
        send(socket_fd, response, strlen(response), 0);
        return;
    }

    /* Get user profile */
    char nickname[21] = {0};
    char profile_msg[101] = {0};
    user_get_profile(user_id, nickname, profile_msg);

    /* Create user session */
    User user = {0};
    strcpy(user.user_id, user_id);
    strcpy(user.nickname, nickname);
    strcpy(user.profile_msg, profile_msg);
    user.socket_fd = socket_fd;
    user.online_status = 1;
    user.dnd_mode = 0;

    if (sessions_add(&user) < 0) {
        char response[256];
        snprintf(response, sizeof(response), "AUTH_RES|0\n");
        send(socket_fd, response, strlen(response), 0);
        return;
    }

    /* Success response */
    char response[512];
    snprintf(response, sizeof(response), "AUTH_RES|1|%s|%s\n", user_id, nickname);
    send(socket_fd, response, strlen(response), 0);
}

void handle_signup_req(int socket_fd, const char *packet)
{
    (void)socket_fd;
    (void)packet;
    /* Signup not implemented in Phase 0 */
}

void handle_logout(int socket_fd, const char *packet)
{
    (void)packet;
    
    User *user = sessions_find_by_fd(socket_fd);
    if (user) {
        sessions_remove(user->user_id);
    }
}

void handle_pass_change(int socket_fd, const char *packet)
{
    (void)socket_fd;
    (void)packet;
    /* Password change not implemented in Phase 0 */
}
