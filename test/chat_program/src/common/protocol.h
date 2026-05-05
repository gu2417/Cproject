#ifndef PROTOCOL_H
#define PROTOCOL_H

/* Packet type constants */
#define PKT_AUTH_REQ        "AUTH_REQ"
#define PKT_AUTH_RES        "AUTH_RES"
#define PKT_SIGNUP_REQ      "SIGNUP_REQ"
#define PKT_SIGNUP_RES      "SIGNUP_RES"
#define PKT_MESSAGE_SEND    "MESSAGE_SEND"
#define PKT_MESSAGE_RECV    "MESSAGE_RECV"
#define PKT_ROOM_CREATE     "ROOM_CREATE"
#define PKT_ROOM_LIST       "ROOM_LIST"
#define PKT_FRIEND_ADD      "FRIEND_ADD"
#define PKT_FRIEND_LIST     "FRIEND_LIST"
#define PKT_DM_SEND         "DM_SEND"
#define PKT_DM_RECV         "DM_RECV"
#define PKT_KEEPALIVE       "KEEPALIVE"
#define PKT_USER_VIEW       "USER_VIEW"
#define PKT_ROOM_INFO       "ROOM_INFO"
#define PKT_ROOM_EDIT       "ROOM_EDIT"
#define PKT_ROOM_DELETE     "ROOM_DELETE"

/* Separators */
#define FIELD_SEP           '|'
#define MULTI_SEP           ':'
#define LIST_SEP            ';'
#define PKT_TERM            '\n'

/* Max packet size */
#define MAX_PACKET_SIZE     2048

#endif // PROTOCOL_H
