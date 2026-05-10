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
#include "auth.h"
#include "user_store.h"

/* LOGIN_REQ|user_id:pw_hash
 * 클라이언트가 SHA-256 해시 후 전송하므로 서버는 단순 문자열 비교만 한다. */
static void handle_login(ClientSession *sess, char *payload) {
    char *user_id = strtok(payload, ":");
    char *pw_hash = strtok(NULL, "");
    int   n, i;

    if (!user_id || !pw_hash) {
        send_packet(sess->sock, LOGIN_RES "|%d", LOGIN_WRONG_ID);
        return;
    }
    /* 금지문자 검사 */
    if (has_forbidden_char(user_id)) {
        send_packet(sess->sock, LOGIN_RES "|%d", LOGIN_WRONG_ID);
        return;
    }
    /* 트레일링 공백 제거 */
    n = (int)strlen(pw_hash);
    while (n > 0 && (pw_hash[n-1] == '\r' || pw_hash[n-1] == '\n'))
        pw_hash[--n] = '\0';

    UserRecord *u = find_user_by_id(user_id);
    if (!u) {
        send_packet(sess->sock, LOGIN_RES "|%d", LOGIN_WRONG_ID);
        return;
    }
    if (strcmp(u->pw_hash, pw_hash) != 0) {
        send_packet(sess->sock, LOGIN_RES "|%d", LOGIN_WRONG_PW);
        return;
    }

    /* 중복 로그인 확인 — MUTEX: g_sessions_mutex */
    WaitForSingleObject(g_sessions_mutex, INFINITE);
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (&g_sessions[i] != sess &&
            g_sessions[i].active &&
            strcmp(g_sessions[i].user_id, user_id) == 0) {
            ReleaseMutex(g_sessions_mutex);
            send_packet(sess->sock, LOGIN_RES "|%d", LOGIN_ALREADY_ONLINE);
            return;
        }
    }
    strncpy(sess->user_id,  u->id_str,   20);
    strncpy(sess->nickname, u->nickname, 20);
    sess->online_status = STATUS_ONLINE;
    ReleaseMutex(g_sessions_mutex);

    u->online_status = STATUS_ONLINE;
    send_packet(sess->sock, LOGIN_RES "|%d", LOGIN_OK);
    notify_friend_status_change(user_id, STATUS_ONLINE);

    /* 대기 중인 친구 요청 알림 전송 */
    {
        int pi;
        for (pi = 0; pi < g_friend_count; pi++) {
            if (g_friends[pi].status == FRIEND_PENDING &&
                strcmp(g_friends[pi].friend_id, user_id) == 0) {
                char req_nick[21];
                char fbuf[256];
                int  flen;
                get_nickname(g_friends[pi].user_id, req_nick);
                flen = snprintf(fbuf, sizeof(fbuf) - 2,
                                FRIEND_REQUEST_NOTIFY "|%s:%s",
                                g_friends[pi].user_id, req_nick);
                fbuf[flen++] = '\n'; fbuf[flen] = '\0';
                send(sess->sock, fbuf, flen, 0);
            }
        }
    }

    printf("[서버] 로그인: %s (%s)\n", u->nickname, user_id);
}

/* REGISTER_REQ|user_id:pw_hash:nickname */
static void handle_register(ClientSession *sess, char *payload) {
    char *user_id  = strtok(payload, ":");
    char *pw_hash  = strtok(NULL, ":");
    char *nickname = strtok(NULL, "");   /* content-last */
    int   n, i;

    if (!user_id || !pw_hash || !nickname) {
        send_packet(sess->sock, REGISTER_RES "|%d", REGISTER_ERROR);
        return;
    }
    n = (int)strlen(nickname);
    while (n > 0 && (nickname[n-1] == '\r' || nickname[n-1] == '\n'))
        nickname[--n] = '\0';

    if (strlen(user_id) == 0 || strlen(user_id) > 20 ||
        strlen(nickname) == 0 || strlen(nickname) > 20) {
        send_packet(sess->sock, REGISTER_RES "|%d", REGISTER_ERROR);
        return;
    }

    /* 금지문자 검사 (40-security.md 준수) */
    if (has_forbidden_char(user_id) || has_forbidden_char(nickname)) {
        send_packet(sess->sock, REGISTER_RES "|%d", REGISTER_ERROR);
        return;
    }
    /* pw_hash는 64자 hex여야 함 */
    if (strlen(pw_hash) != 64) {
        send_packet(sess->sock, REGISTER_RES "|%d", REGISTER_ERROR);
        return;
    }

    /* 중복 검사 (ID 및 닉네임) */
    for (i = 0; i < g_user_count; i++) {
        if (strcmp(g_users[i].id_str,   user_id)  == 0 ||
            strcmp(g_users[i].nickname, nickname) == 0) {
            send_packet(sess->sock, REGISTER_RES "|%d", REGISTER_DUPLICATE);
            return;
        }
    }

    /* MUTEX: g_sessions_mutex */
    WaitForSingleObject(g_sessions_mutex, INFINITE);
    if (g_user_count >= MAX_USERS) {
        ReleaseMutex(g_sessions_mutex);
        send_packet(sess->sock, REGISTER_RES "|%d", REGISTER_ERROR);
        return;
    }
    UserRecord *u = &g_users[g_user_count];
    memset(u, 0, sizeof(*u));
    u->id = g_user_count + 1;
    strncpy(u->id_str,   user_id,  20);
    strncpy(u->pw_hash,  pw_hash,  64);
    strncpy(u->nickname, nickname, 20);
    u->online_status = STATUS_OFFLINE;
    get_current_timestamp(u->created_at);
    get_current_timestamp(u->last_seen);
    g_user_count++;
    ReleaseMutex(g_sessions_mutex);

    /* MUTEX: g_file_mutex */
    WaitForSingleObject(g_file_mutex, INFINITE);
    append_user(FILE_USERS, u);
    ReleaseMutex(g_file_mutex);

    send_packet(sess->sock, REGISTER_RES "|%d", REGISTER_OK);
    printf("[서버] 회원가입: %s (%s)\n", nickname, user_id);
}

/* LOGOUT_REQ| */
static void handle_logout(ClientSession *sess, char *payload) {
    (void)payload;
    if (sess->user_id[0] == '\0') return;

    char user_id[21];
    char nickname[21];
    strncpy(user_id, sess->user_id, 20);
    user_id[20] = '\0';
    strncpy(nickname, sess->nickname, 20);
    nickname[20] = '\0';

    UserRecord *u = find_user_by_id(sess->user_id);
    if (u) u->online_status = STATUS_OFFLINE;

    notify_friend_status_change(sess->user_id, STATUS_OFFLINE);
    update_last_seen(sess->user_id);

    send_packet(sess->sock, LOGOUT_RES "|0");

    WaitForSingleObject(g_sessions_mutex, INFINITE);
    sess->user_id[0]    = '\0';
    sess->nickname[0]   = '\0';
    sess->room_id       = 0;
    sess->online_status = STATUS_OFFLINE;
    ReleaseMutex(g_sessions_mutex);

    printf("[서버] 로그아웃: %s (%s)\n",
           nickname[0] ? nickname : user_id, user_id);
}

/* PASS_CHANGE|old_pw_hash:new_pw_hash */
static void handle_pass_change(ClientSession *sess, char *payload) {
    if (sess->user_id[0] == '\0') return;

    char *old_hash = strtok(payload, ":");
    char *new_hash = strtok(NULL, "");
    int   n;

    if (!old_hash || !new_hash) {
        send_packet(sess->sock, PASS_CHANGE_RES "|1");
        return;
    }
    n = (int)strlen(new_hash);
    while (n > 0 && (new_hash[n-1] == '\r' || new_hash[n-1] == '\n'))
        new_hash[--n] = '\0';

    UserRecord *u = find_user_by_id(sess->user_id);
    if (!u || strcmp(u->pw_hash, old_hash) != 0) {
        send_packet(sess->sock, PASS_CHANGE_RES "|1");
        return;
    }

    strncpy(u->pw_hash, new_hash, 64);

    WaitForSingleObject(g_file_mutex, INFINITE);
    save_users(FILE_USERS);
    ReleaseMutex(g_file_mutex);

    send_packet(sess->sock, PASS_CHANGE_RES "|0");
}

void auth_init(void) {
    register_handler(LOGIN_REQ,    handle_login);
    register_handler(REGISTER_REQ, handle_register);
    register_handler(LOGOUT_REQ,   handle_logout);
    register_handler(PASS_CHANGE,  handle_pass_change);
}
