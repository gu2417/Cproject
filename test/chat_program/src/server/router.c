#include "router.h"
#include "../common/protocol.h"
#include "auth.h"
#include "friend.h"
#include "room.h"
#include "dm.h"
#include "message.h"
#include <stdio.h>
#include <string.h>

void router_init(void)
{
}

void router_dispatch(int socket_fd, const char *packet)
{
    if (!packet || strlen(packet) == 0) {
        return;
    }

    char type_buf[32];
    sscanf(packet, "%31[^|]", type_buf);

    if (strcmp(type_buf, PKT_AUTH_REQ) == 0) {
        handle_auth_req(socket_fd, packet);
    } else if (strcmp(type_buf, PKT_SIGNUP_REQ) == 0) {
        handle_signup_req(socket_fd, packet);
    } else if (strcmp(type_buf, PKT_FRIEND_ADD) == 0) {
        handle_friend_add(socket_fd, packet);
    } else if (strcmp(type_buf, PKT_ROOM_CREATE) == 0) {
        handle_room_create(socket_fd, packet);
    } else if (strcmp(type_buf, PKT_MESSAGE_SEND) == 0) {
        handle_message_send(socket_fd, packet);
    } else if (strcmp(type_buf, PKT_DM_SEND) == 0) {
        handle_dm_send(socket_fd, packet);
    } else {
        fprintf(stderr, "Unknown packet type: %s\n", type_buf);
    }
}
