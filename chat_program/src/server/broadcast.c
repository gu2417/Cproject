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
/* printf 형식으로 패킷을 만들어 한 소켓에 보낸다. */
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
/* 로그인한 모든 사용자에게 같은 메시지를 보낸다. */
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
 * broadcast_to_room: room_id 방의 모든 멤버(온라인)에게 msg 를 전송한다.
 * 멤버가 현재 방 뷰에 있든 메인 메뉴에 있든 관계없이 전송하여
 * 클라이언트가 상황에 따라 인라인 표시 또는 알림으로 처리한다.
 * MUTEX: g_sessions_mutex
 * --------------------------------------------------------------- */
/* 해당 방 멤버 중 접속 중인 사용자에게 메시지를 보낸다. */
void broadcast_to_room(int room_id, const char *msg) {
    int i, j;
    int len = (int)strlen(msg);

    /* MUTEX: g_sessions_mutex */
    WaitForSingleObject(g_sessions_mutex, INFINITE);
    int ri = find_room_idx(room_id);
    if (ri < 0) {
        ReleaseMutex(g_sessions_mutex);
        return;
    }
    for (i = 0; i < g_rooms[ri].member_count; i++) {
        const char *mid = g_rooms[ri].member_ids[i];
        for (j = 0; j < MAX_CLIENTS; j++) {
            if (g_sessions[j].active &&
                g_sessions[j].sock != INVALID_SOCKET &&
                strcmp(g_sessions[j].user_id, mid) == 0) {
                send(g_sessions[j].sock, msg, len, 0);
                break;
            }
        }
    }
    ReleaseMutex(g_sessions_mutex);
}

/* ---------------------------------------------------------------
 * send_to_user: 특정 user_id 의 온라인 세션 하나에 msg 를 전송한다.
 * MUTEX: g_sessions_mutex
 * --------------------------------------------------------------- */
/* 특정 사용자에게만 메시지를 보낸다. */
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
