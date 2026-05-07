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

/* MYPAGE_REQ| → MYPAGE_RES|nickname:status_msg:online_status:created_at */
static void handle_mypage(ClientSession *sess, char *payload) {
    (void)payload;
    if (sess->user_id[0] == '\0') return;
    UserRecord *u = find_user_by_id(sess->user_id);
    if (!u) return;
    send_packet(sess->sock, MYPAGE_RES "|%s:%s:%d:%s",
                u->nickname, u->status_msg, u->online_status, u->created_at);
}

/* MY_ROOMS_REQ| → MY_ROOMS_RES|room_id:name;room_id:name;... */
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
static void handle_profile_update(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;
    char *new_nick   = strtok(payload, ":");
    char *status_msg = strtok(NULL, "");
    int   n, i;

    if (!new_nick || strlen(new_nick) == 0 || strlen(new_nick) > 20) return;
    if (status_msg) {
        n = (int)strlen(status_msg);
        while (n > 0 && (status_msg[n-1] == '\r' || status_msg[n-1] == '\n'))
            status_msg[--n] = '\0';
    }

    /* 닉네임 중복 검사 */
    for (i = 0; i < g_user_count; i++) {
        if (strcmp(g_users[i].id_str, sess->user_id) == 0) continue;
        if (strcmp(g_users[i].nickname, new_nick) == 0) {
            send_packet(sess->sock, PROFILE_UPDATE_RES "|1");
            return;
        }
    }

    UserRecord *u = find_user_by_id(sess->user_id);
    if (!u) return;
    strncpy(u->nickname, new_nick, 20);
    if (status_msg) strncpy(u->status_msg, status_msg, 100);

    WaitForSingleObject(g_sessions_mutex, INFINITE);
    strncpy(sess->nickname, u->nickname, 20);
    ReleaseMutex(g_sessions_mutex);

    WaitForSingleObject(g_file_mutex, INFINITE);
    save_users(FILE_USERS);
    ReleaseMutex(g_file_mutex);

    send_packet(sess->sock, PROFILE_UPDATE_RES "|0");
}

/* STATUS_CHANGE|status_code (0=offline,1=online,2=busy,3=invisible) */
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
static void handle_settings_req(ClientSession *sess, char *payload) {
    (void)payload;
    if (sess->user_id[0] == '\0') return;
    /* P0: 기본값 반환 */
    send_packet(sess->sock, SETTINGS_RES "|cyan:yellow:dark:0:0");
}

/* SETTINGS_UPDATE|msg_color:nick_color:theme:ts_format:dnd → SETTINGS_UPDATE_RES|0 */
static void handle_settings_update(ClientSession *sess, char *payload) {
    (void)payload;
    if (sess->user_id[0] == '\0') return;
    /* P0: 수신만 하고 확인 응답 */
    send_packet(sess->sock, SETTINGS_UPDATE_RES "|0");
}

/* PING| → PONG| */
static void handle_ping(ClientSession *sess, char *payload) {
    (void)payload;
    send_packet(sess->sock, PONG "|");
}

void user_store_init(void) {
    register_handler(MYPAGE_REQ,      handle_mypage);
    register_handler(MY_ROOMS_REQ,    handle_my_rooms);
    register_handler(PROFILE_UPDATE,  handle_profile_update);
    register_handler(STATUS_CHANGE,   handle_status_change);
    register_handler(SETTINGS_REQ,    handle_settings_req);
    register_handler(SETTINGS_UPDATE, handle_settings_update);
    register_handler(PING,            handle_ping);
}
