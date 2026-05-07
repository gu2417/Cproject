#include <winsock2.h>
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "../common/protocol.h"
#include "../common/types.h"
#include "globals.h"
#include "broadcast.h"

/* ---------------------------------------------------------------
 * send_packet: 단일 소켓에 printf 스타일로 패킷을 전송한다.
 * 포맷 결과 끝에 '\n' 이 없으면 자동으로 붙인다.
 * --------------------------------------------------------------- */
void send_packet(SOCKET sock, const char *fmt, ...) {
    char    buf[MAX_PKT_SIZE];
    va_list ap;
    int     n;

    va_start(ap, fmt);
    n = vsnprintf(buf, (int)sizeof(buf) - 2, fmt, ap);
    va_end(ap);

    if (n < 0) return;
    if (n > (int)sizeof(buf) - 2) n = (int)sizeof(buf) - 2;

    if (n == 0 || buf[n - 1] != '\n') {
        buf[n++] = '\n';
    }
    buf[n] = '\0';

    send(sock, buf, n, 0);
}

/* ---------------------------------------------------------------
 * broadcast_to_all: 로그인된 모든 활성 세션에 msg 를 전송한다.
 * MUTEX: g_sessions_mutex
 * --------------------------------------------------------------- */
void broadcast_to_all(const char *msg) {
    int     i;
    int     len = (int)strlen(msg);

    /* MUTEX: g_sessions_mutex */
    WaitForSingleObject(g_sessions_mutex, INFINITE);
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (g_sessions[i].active &&
            g_sessions[i].sock != INVALID_SOCKET &&
            g_sessions[i].user_id[0] != '\0') {
            send(g_sessions[i].sock, msg, len, 0);
        }
    }
    ReleaseMutex(g_sessions_mutex);
}

/* ---------------------------------------------------------------
 * broadcast_to_room: 특정 room_id 에 속한 세션에만 msg 를 전송한다.
 * MUTEX: g_sessions_mutex
 * --------------------------------------------------------------- */
void broadcast_to_room(int room_id, const char *msg) {
    int i;
    int len = (int)strlen(msg);

    /* MUTEX: g_sessions_mutex */
    WaitForSingleObject(g_sessions_mutex, INFINITE);
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (g_sessions[i].active &&
            g_sessions[i].sock != INVALID_SOCKET &&
            g_sessions[i].room_id == room_id) {
            send(g_sessions[i].sock, msg, len, 0);
        }
    }
    ReleaseMutex(g_sessions_mutex);
}

/* ---------------------------------------------------------------
 * send_to_user: 특정 user_id 의 온라인 세션 하나에 msg 를 전송한다.
 * MUTEX: g_sessions_mutex
 * --------------------------------------------------------------- */
void send_to_user(const char *user_id, const char *msg) {
    int i;
    int len = (int)strlen(msg);

    /* MUTEX: g_sessions_mutex */
    WaitForSingleObject(g_sessions_mutex, INFINITE);
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (g_sessions[i].active &&
            g_sessions[i].sock != INVALID_SOCKET &&
            strcmp(g_sessions[i].user_id, user_id) == 0) {
            send(g_sessions[i].sock, msg, len, 0);
            break;
        }
    }
    ReleaseMutex(g_sessions_mutex);
}
