#pragma once

#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "protocol.h"

/* =========================================================
 * 서버가 파일과 메모리에 저장하는 자료형
 * ========================================================= */

/* 가입한 사용자 한 명의 정보를 담는다. */
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

/* 채팅방 기본 정보와 방 설정을 담는다. */
typedef struct {
    int    id;
    char   name[31];
    char   pw_hash[65];        /* 빈 문자열이면 공개방 */
    int    is_open;
    char   owner_id[21];
    char   topic[101];
    char   notice[256];
    int    pinned_msg_id;
    char   created_at[20];
    int    max_members;
    int    is_deleted;
} RoomRecord;

/* 채팅방 메시지와 DM 내용을 한 가지 형태로 저장한다. */
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

/* 친구 신청, 친구 관계, 차단 상태를 저장한다. */
typedef struct {
    int    id;
    char   user_id[21];
    char   friend_id[21];
    int    status;             /* 0=pending, 1=accepted, 2=blocked */
    int    status_before_block; /* -1=none, otherwise status restored on unblock */
    char   created_at[20];
} FriendRecord;

/* 사용자가 어느 방에 속해 있는지와 방 안 권한을 저장한다. */
typedef struct {
    int    room_id;
    char   user_id[21];
    char   open_nick[21];      /* 오픈채팅 별명, 빈 문자열이면 users.txt nickname 사용 */
    int    is_admin;
    char   joined_at[20];
    int    is_muted;
} RoomMemberRecord;

/* DM 메시지를 누가 언제 읽었는지 저장한다. */
typedef struct {
    int    msg_id;
    char   reader_id[21];
    char   read_at[20];
} DmReadRecord;

/* 채팅방 초대 요청과 처리 상태를 저장한다. */
typedef struct {
    int    id;
    int    room_id;
    char   inviter_id[21];
    char   invitee_id[21];
    int    status;             /* 0=pending, 1=accepted, 2=rejected */
    char   created_at[20];
} RoomInviteRecord;

/* 사용자별 화면 색상, 테마, 알림 설정을 저장한다. */
typedef struct {
    char   user_id[21];
    char   msg_color[16];      /* "cyan", "yellow", "red", "green", "white" */
    char   nick_color[16];
    char   theme[11];          /* "dark" or "light" */
    int    ts_format;          /* 0=HH:MM, 1=HH:MM:SS, 2=MM-DD HH:MM */
    int    dnd;                /* 0=off, 1=on */
    int    welcome_shown;      /* 0=첫 로그인, 1=가이드 출력 완료 */
} UserSettingsRecord;

/* 방별 마지막 읽은 메시지를 저장한다. */
typedef struct {
    int    room_id;
    char   user_id[21];
    int    last_read_msg_id;
    char   read_at[20];
} RoomReadRecord;

/* =========================================================
 * 서버에 접속한 클라이언트 상태
 * ========================================================= */

/* 서버가 현재 연결된 클라이언트 한 명을 관리할 때 사용한다. */
typedef struct {
    SOCKET sock;
    int    active;
    char   user_id[21];
    char   nickname[21];
    int    online_status;
    int    room_id;
    char   addr_str[46];
    time_t last_recv;          /* PING/PONG 60초 체크 */
    int    dnd;
    int    muted_rooms[32];
    int    muted_count;
    int    is_admin;
    HANDLE hThread;
} ClientSession;

/* =========================================================
 * 서버 방 정보
 * ========================================================= */

/* 방 기본 정보와 현재 참여자 목록을 함께 들고 있는 구조체이다. */
typedef struct {
    RoomRecord  info;
    char        member_ids[MAX_ROOM_MEMBERS][21];
    int         admin_flags[MAX_ROOM_MEMBERS];
    int         member_count;
} RoomInfo;

/* =========================================================
 * 클라이언트 상태
 * ========================================================= */

/* 클라이언트 프로그램이 로그인, 현재 방, 설정 상태를 기억할 때 사용한다. */
typedef struct {
    SOCKET  sock;
    int     logged_in;
    char    user_id[21];
    char    nickname[21];
    char    status_msg[101];
    int     online_status;
    int     current_room_id;
    char    current_room_name[31];
    char    current_room_notice[256];
    int     pending_invite_room_id;
    char    pending_invite_room_name[31];

    /* DM 대화 상대 */
    char    current_dm_partner[21];
    char    current_dm_partner_nick[21];

    /* 알림 설정 */
    int     muted_rooms[32];
    int     muted_count;

    /* 연결 상태 */
    int     connected;
    int     response_received;
    time_t  last_pong;

    /* 화면 설정 */
    char    msg_color[16];
    char    nick_color[16];
    char    theme[11];
    int     ts_format;
    int     dnd;
} ClientState;
