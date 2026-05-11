#pragma once

#include <winsock2.h>

/* 서버에서 받은 한 줄 패킷을 종류별로 나누어 처리한다. */
void packet_parse(const char *buf, SOCKET sock);

/* 채팅 메시지를 현재 화면 설정에 맞게 출력한다. */
void display_chat_message(const char *from_nick, const char *timestamp,
                          const char *content, int msg_type, int reply_to);

/* 친구 목록 응답을 잠시 저장해 메뉴에서 다시 보여줄 때 사용한다. */
#define MAX_FRIEND_LIST 64
typedef struct {
    char id[21];
    char nick[21];
    int  status;          /* 0=pending 1=accepted 2=blocked */
    char status_msg[101];
} FriendEntry;
extern FriendEntry g_friend_list[MAX_FRIEND_LIST];
extern int         g_friend_count;

/* DM 목록 응답을 잠시 저장해 메뉴에서 다시 보여줄 때 사용한다. */
#define MAX_DM_LIST 64
typedef struct {
    char partner_id[21];
    char partner_nick[21];
    char last_msg[101];
    int  unread;
} DmPartnerEntry;
extern DmPartnerEntry g_dm_list[MAX_DM_LIST];
extern int            g_dm_count;
