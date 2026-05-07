#pragma once

#include <winsock2.h>

void packet_parse(const char *buf, SOCKET sock);
void display_chat_message(const char *from_nick, const char *timestamp,
                          const char *content, int msg_type, int reply_to);

/* ── 친구 목록 캐시 (FRIEND_LIST_RES 수신 시 채워짐) ─────── */
#define MAX_FRIEND_LIST 64
typedef struct {
    char id[21];
    char nick[21];
    int  status;          /* 0=pending 1=accepted 2=blocked */
    char status_msg[101];
} FriendEntry;
extern FriendEntry g_friend_list[MAX_FRIEND_LIST];
extern int         g_friend_count;

/* ── DM 파트너 목록 캐시 (DM_LIST_RES 수신 시 채워짐) ────── */
#define MAX_DM_LIST 64
typedef struct {
    char partner_id[21];
    char partner_nick[21];
    char last_msg[101];
    int  unread;
} DmPartnerEntry;
extern DmPartnerEntry g_dm_list[MAX_DM_LIST];
extern int            g_dm_count;
