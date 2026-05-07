#pragma once

#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "protocol.h"

/* =========================================================
 * 서버측 구조체
 * ========================================================= */

typedef struct {
    int    id;
    char   id_str[21];
    char   pw_hash[65];
    char   nickname[21];
    int    online_status;      /* 0=offline, 1=online, 2=busy, 3=invisible */
    char   created_at[20];    /* YYYY-MM-DD HH:MM:SS */
    char   last_seen[20];
    char   status_msg[101];
} UserRecord;

typedef struct {
    int    id;
    char   name[31];
    char   pw_hash[65];        /* 빈 문자열이면 공개방 */
    int    is_open;
    char   owner_id[21];
    char   topic[101];
    char   notice[201];
    int    pinned_msg_id;
    char   created_at[20];
    int    max_members;
    int    is_deleted;
} RoomRecord;

typedef struct {
    int    id;
    int    room_id;            /* 0 = DM */
    char   from_id[21];
    char   from_nick[21];
    char   to_id[21];          /* DM 수신자, 일반 메시지는 빈 문자열 */
    char   content[MAX_PKT_SIZE];
    int    msg_type;           /* 0=normal,1=system,2=whisper,3=me */
    char   created_at[20];
    char   edited_at[20];
    int    reply_to_id;
    int    is_deleted;
} MessageRecord;

typedef struct {
    int    id;
    char   user_id[21];
    char   friend_id[21];
    int    status;             /* 0=pending, 1=accepted, 2=blocked */
    char   created_at[20];
} FriendRecord;

typedef struct {
    int    room_id;
    char   user_id[21];
    char   open_nick[21];      /* 오픈채팅 닉네임, 빈 문자열이면 users.txt nickname 사용 */
    int    is_admin;
    char   joined_at[20];
    int    is_muted;
} RoomMemberRecord;

typedef struct {
    int    msg_id;
    char   reader_id[21];
    char   read_at[20];
} DmReadRecord;

typedef struct {
    int    id;
    int    room_id;
    char   inviter_id[21];
    char   invitee_id[21];
    char   created_at[20];
    int    status;             /* 0=pending, 1=accepted, 2=rejected */
} RoomInviteRecord;

typedef struct {
    char   user_id[21];
    char   msg_color[16];      /* "cyan", "yellow", "red", "green", "white" */
    char   nick_color[16];
    char   theme[11];          /* "dark" or "light" */
    int    ts_format;          /* 0=HH:MM, 1=HH:MM:SS, 2=MM-DD HH:MM */
    int    dnd;                /* 0=off, 1=on */
    int    welcome_shown;      /* 0=첫 로그인, 1=가이드 출력 완료 */
} UserSettingsRecord;

typedef struct {
    int    room_id;
    char   user_id[21];
    int    last_read_msg_id;
} RoomReadRecord;

/* =========================================================
 * 서버 세션
 * ========================================================= */

typedef struct {
    SOCKET sock;
    int    active;
    char   user_id[21];
    char   nickname[21];
    int    online_status;
    int    room_id;
    char   addr_str[46];
    time_t last_recv;          /* PING/PONG 60초 체크 */
} ClientSession;

/* =========================================================
 * 서버 방 정보 (인메모리)
 * ========================================================= */

typedef struct {
    RoomRecord  info;
    char        member_ids[MAX_ROOM_MEMBERS][21];
    int         member_count;
} RoomInfo;

/* =========================================================
 * 클라이언트 상태 (client/state.h에서 extern 선언)
 * ========================================================= */

typedef struct {
    SOCKET  sock;
    int     logged_in;
    char    user_id[21];
    char    nickname[21];
    int     online_status;
    int     current_room_id;
    char    current_room_name[31];

    /* DM 컨텍스트 */
    char    current_dm_partner[21];
    char    current_dm_partner_nick[21];

    /* 알림 설정 */
    int     muted_rooms[32];
    int     muted_count;

    /* 연결 상태 */
    int     connected;
    int     response_received;
    time_t  last_pong;

    /* 설정 */
    char    msg_color[16];
    char    nick_color[16];
    char    theme[11];
    int     ts_format;
    int     dnd;
} ClientState;
