#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../common/protocol.h"
#include "../common/types.h"
#include "../common/utils.h"
#include "globals.h"
#include "file_io.h"
#include "broadcast.h"
#include "router.h"
#include "user_store.h"

/* 유저 닉네임 반환. 없으면 user_id를 복사한다. */
/* 사용자 아이디로 닉네임을 찾아 돌려준다. */
void get_nickname(const char *user_id, char out_nick[21]) {
    int i;
    for (i = 0; i < g_user_count; i++) {
        if (strcmp(g_users[i].id_str, user_id) == 0) {
            strncpy(out_nick, g_users[i].nickname, 20);
            out_nick[20] = '\0';
            return;
        }
    }
    strncpy(out_nick, user_id, 20);
    out_nick[20] = '\0';
}

/* last_seen을 현재 시각으로 갱신하고 파일을 저장한다. */
/* 사용자의 마지막 접속 시간을 현재 시간으로 갱신한다. */
void update_last_seen(const char *user_id) {
    UserRecord *u = find_user_by_id(user_id);
    if (!u) return;
    get_current_timestamp(u->last_seen);
    WaitForSingleObject(g_file_mutex, INFINITE);
    save_users(FILE_USERS);
    ReleaseMutex(g_file_mutex);
}

/* user_id의 친구(accepted) 중 온라인인 세션에 FRIEND_STATUS_CHANGE를 전송한다.
 * MUTEX: g_sessions_mutex 내부 획득 */
/* 친구들에게 사용자의 온라인 상태 변경을 알린다. */
void notify_friend_status_change(const char *user_id, int new_status) {
    char nick[21] = {0};
    char msg[256];
    int  i, j;
    int  display_status = (new_status == STATUS_INVISIBLE)
                            ? STATUS_OFFLINE : new_status;

    get_nickname(user_id, nick);
    snprintf(msg, sizeof(msg) - 2,
             FRIEND_STATUS_CHANGE "|%s:%s:%d", user_id, nick, display_status);
    int mlen = (int)strlen(msg);
    msg[mlen++] = '\n'; msg[mlen] = '\0';

    WaitForSingleObject(g_sessions_mutex, INFINITE);
    for (i = 0; i < g_friend_count; i++) {
        FriendRecord *fr = &g_friends[i];
        if (fr->status != FRIEND_ACCEPTED) continue;

        const char *other_id = NULL;
        if      (strcmp(fr->user_id,   user_id) == 0) other_id = fr->friend_id;
        else if (strcmp(fr->friend_id, user_id) == 0) other_id = fr->user_id;
        else continue;

        for (j = 0; j < MAX_CLIENTS; j++) {
            if (g_sessions[j].active &&
                strcmp(g_sessions[j].user_id, other_id) == 0) {
                send(g_sessions[j].sock, msg, mlen, 0);
                break;
            }
        }
    }
    ReleaseMutex(g_sessions_mutex);
}

/* MYPAGE_REQ| → MYPAGE_RES|id:nickname:created_at:last_seen:msg_count:room_count:friend_count:status_msg */
/* 마이페이지에 필요한 사용자 요약 정보를 보낸다. */
static void handle_mypage(ClientSession *sess, char *payload) {
    (void)payload;
    if (sess->user_id[0] == '\0') return;
    UserRecord *u = find_user_by_id(sess->user_id);
    if (!u) return;
    /* content-last 규칙: status_msg 가 마지막 필드 */
    send_packet(sess->sock, MYPAGE_RES "|%s:%s:%s:%s:%d:%d:%d:%s",
                u->id_str, u->nickname, u->created_at, u->last_seen,
                count_user_messages(sess->user_id),
                count_user_rooms(sess->user_id),
                count_user_friends(sess->user_id),
                u->status_msg);
}

/* MY_ROOMS_REQ| → MY_ROOMS_RES|room_id:name;room_id:name;... */
/* 사용자가 참여한 방 목록을 만들어 보낸다. */
static void handle_my_rooms(ClientSession *sess, char *payload) {
    (void)payload;
    if (sess->user_id[0] == '\0') return;

    char buf[MAX_PKT_SIZE];
    int  off = 0;
    int  cnt = 0;
    int  i, j;

    off += snprintf(buf + off, sizeof(buf) - off, MY_ROOMS_RES "|");

    WaitForSingleObject(g_sessions_mutex, INFINITE);
    for (i = 0; i < g_room_count && off < (int)sizeof(buf) - 64; i++) {
        if (g_rooms[i].info.is_deleted) continue;
        for (j = 0; j < g_rooms[i].member_count; j++) {
            if (strcmp(g_rooms[i].member_ids[j], sess->user_id) == 0) {
                if (cnt > 0) buf[off++] = ';';
                off += snprintf(buf + off, sizeof(buf) - off,
                                "%d:%s",
                                g_rooms[i].info.id, g_rooms[i].info.name);
                cnt++;
                break;
            }
        }
    }
    ReleaseMutex(g_sessions_mutex);

    buf[off++] = '\n';
    buf[off]   = '\0';
    send(sess->sock, buf, off, 0);
}

/* PROFILE_UPDATE|new_nick:status_msg → PROFILE_UPDATE_RES|0 (or 1) */
/* 닉네임과 상태 메시지 변경 요청을 처리한다. */
static void handle_profile_update(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;
    char *new_nick = payload;
    char *status_msg = "";
    int   n, i;

    if (!payload) {
        send_packet(sess->sock, PROFILE_UPDATE_RES "|1");
        return;
    }
    status_msg = strchr(payload, ':');
    if (status_msg) {
        *status_msg++ = '\0';
    } else {
        status_msg = "";
    }

    if (!new_nick || strlen(new_nick) == 0 || strlen(new_nick) > 20) {
        send_packet(sess->sock, PROFILE_UPDATE_RES "|1");
        return;
    }
    n = (int)strlen(status_msg);
    while (n > 0 && (status_msg[n-1] == '\r' || status_msg[n-1] == '\n'))
        status_msg[--n] = '\0';

    /* 닉네임 중복 검사 */
    for (i = 0; i < g_user_count; i++) {
        if (strcmp(g_users[i].id_str, sess->user_id) == 0) continue;
        if (strcmp(g_users[i].nickname, new_nick) == 0) {
            send_packet(sess->sock, PROFILE_UPDATE_RES "|1");
            return;
        }
    }

    UserRecord *u = find_user_by_id(sess->user_id);
    if (!u) {
        send_packet(sess->sock, PROFILE_UPDATE_RES "|1");
        return;
    }
    strncpy(u->nickname, new_nick, 20);
    u->nickname[20] = '\0';
    strncpy(u->status_msg, status_msg, 100);
    u->status_msg[100] = '\0';

    WaitForSingleObject(g_sessions_mutex, INFINITE);
    strncpy(sess->nickname, u->nickname, 20);
    sess->nickname[20] = '\0';
    ReleaseMutex(g_sessions_mutex);

    WaitForSingleObject(g_file_mutex, INFINITE);
    save_users(FILE_USERS);
    ReleaseMutex(g_file_mutex);

    send_packet(sess->sock, PROFILE_UPDATE_RES "|0");
}

/* STATUS_CHANGE|status_code (0=offline,1=online,2=busy,3=invisible) */
/* 사용자의 온라인 상태 변경 요청을 처리한다. */
static void handle_status_change(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;
    int status = payload ? atoi(payload) : STATUS_ONLINE;
    if (status < 0 || status > 3) status = STATUS_ONLINE;

    UserRecord *u = find_user_by_id(sess->user_id);
    if (u) u->online_status = status;

    WaitForSingleObject(g_sessions_mutex, INFINITE);
    sess->online_status = status;
    ReleaseMutex(g_sessions_mutex);

    notify_friend_status_change(sess->user_id, status);
}

/* SETTINGS_REQ| → SETTINGS_RES|msg_color:nick_color:theme:ts_format:dnd */
/* 현재 저장된 사용자 설정을 클라이언트에 보낸다. */
static void handle_settings_req(ClientSession *sess, char *payload) {
    (void)payload;
    if (sess->user_id[0] == '\0') return;

    int i;
    for (i = 0; i < g_user_settings_count; i++) {
        if (strcmp(g_user_settings[i].user_id, sess->user_id) == 0) {
            send_packet(sess->sock, SETTINGS_RES "|%s:%s:%s:%d:%d",
                        g_user_settings[i].msg_color,
                        g_user_settings[i].nick_color,
                        g_user_settings[i].theme,
                        g_user_settings[i].ts_format,
                        g_user_settings[i].dnd);
            return;
        }
    }
    /* 저장된 설정 없음: 기본값 */
    send_packet(sess->sock, SETTINGS_RES "|cyan:yellow:dark:0:0");
}

/* SETTINGS_UPDATE|msg_color:nick_color:theme:ts_format:dnd → SETTINGS_UPDATE_RES|0 */
/* 사용자 설정 변경 내용을 저장하고 세션에도 반영한다. */
static void handle_settings_update(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    char *msg_color  = strtok(payload, ":");
    char *nick_color = strtok(NULL,    ":");
    char *theme      = strtok(NULL,    ":");
    char *ts_fmt_s   = strtok(NULL,    ":");
    char *dnd_s      = strtok(NULL,    "");
    if (!msg_color || !nick_color || !theme || !ts_fmt_s || !dnd_s) {
        send_packet(sess->sock, SETTINGS_UPDATE_RES "|1");
        return;
    }

    int n = (int)strlen(dnd_s);
    while (n > 0 && (dnd_s[n-1] == '\r' || dnd_s[n-1] == '\n'))
        dnd_s[--n] = '\0';

    UserSettingsRecord s;
    memset(&s, 0, sizeof(s));
    strncpy(s.user_id,    sess->user_id, 20);
    strncpy(s.msg_color,  msg_color,     15);
    strncpy(s.nick_color, nick_color,    15);
    strncpy(s.theme,      theme,         10);
    s.ts_format = atoi(ts_fmt_s);
    s.dnd       = atoi(dnd_s);

    /* welcome_shown 은 기존 값 유지 */
    int i;
    for (i = 0; i < g_user_settings_count; i++) {
        if (strcmp(g_user_settings[i].user_id, sess->user_id) == 0) {
            s.welcome_shown = g_user_settings[i].welcome_shown;
            break;
        }
    }

    /* MUTEX: g_file_mutex */
    WaitForSingleObject(g_file_mutex, INFINITE);
    upsert_user_settings(FILE_USER_SETTINGS, &s);
    ReleaseMutex(g_file_mutex);

    /* 세션의 dnd 상태 갱신 — MUTEX: g_sessions_mutex */
    WaitForSingleObject(g_sessions_mutex, INFINITE);
    sess->dnd = s.dnd;
    ReleaseMutex(g_sessions_mutex);

    send_packet(sess->sock, SETTINGS_UPDATE_RES "|0");
}

/* PING| → PONG| */
/* 클라이언트의 연결 확인 요청에 응답한다. */
static void handle_ping(ClientSession *sess, char *payload) {
    (void)payload;
    send_packet(sess->sock, PONG "|");
}

/* 사용자 정보와 설정 관련 패킷 처리 함수를 등록한다. */
void user_store_init(void) {
    register_handler(MYPAGE_REQ,      handle_mypage);
    register_handler(MY_ROOMS_REQ,    handle_my_rooms);
    register_handler(PROFILE_UPDATE,  handle_profile_update);
    register_handler(STATUS_CHANGE,   handle_status_change);
    register_handler(SETTINGS_REQ,    handle_settings_req);
    register_handler(SETTINGS_UPDATE, handle_settings_update);
    register_handler(PING,            handle_ping);
}
