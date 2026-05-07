#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "../common/protocol.h"
#include "../common/types.h"
#include "globals.h"
#include "broadcast.h"
#include "router.h"
#include "client_handler.h"

/* ---------------------------------------------------------------
 * leftClient: 세션을 정리하고 소켓을 닫는다.
 * 1. 로그인 + 방 안에 있으면 퇴장 메시지를 방에 broadcast
 * 2. g_sessions 슬롯 초기화 (active=0, closesocket)
 * MUTEX: g_sessions_mutex 를 내부에서 획득·해제
 * --------------------------------------------------------------- */
void leftClient(ClientSession *sess) {
    char msg[256];

    if (!sess->active) return;

    /* 로그인 상태이고 방 안에 있으면 퇴장 알림 */
    if (sess->user_id[0] != '\0' && sess->room_id > 0) {
        snprintf(msg, sizeof(msg),
                 "NOTIFY|SERVER:%s님이 퇴장했습니다.\n",
                 sess->nickname[0] ? sess->nickname : sess->user_id);
        broadcast_to_room(sess->room_id, msg);
    }

    /* MUTEX: g_sessions_mutex */
    WaitForSingleObject(g_sessions_mutex, INFINITE);
    closesocket(sess->sock);
    sess->sock          = INVALID_SOCKET;
    sess->active        = 0;
    sess->user_id[0]    = '\0';
    sess->nickname[0]   = '\0';
    sess->room_id       = 0;
    sess->online_status = 0;
    if (g_session_count > 0) g_session_count--;
    ReleaseMutex(g_sessions_mutex);
}

/* ---------------------------------------------------------------
 * HandleClient: 클라이언트 1개를 담당하는 스레드 함수.
 * arg = (void*)(intptr_t)slot — g_sessions[] 의 슬롯 인덱스
 *
 * recv 루프:
 *   - '\n' 단위로 패킷을 조립하여 route_packet() 에 전달
 *   - recv <= 0 시 반드시 leftClient() 호출 (break 만 하면 안 됨)
 * --------------------------------------------------------------- */
unsigned WINAPI HandleClient(void *arg) {
    int            slot = (int)(intptr_t)arg;
    ClientSession *sess = &g_sessions[slot];
    char           buf[MAX_PKT_SIZE];
    char           line[MAX_PKT_SIZE];
    int            line_len = 0;
    int            n;
    int            i;

    sess->last_recv = time(NULL);

    while (1) {
        n = recv(sess->sock, buf, (int)sizeof(buf) - 1, 0);
        if (n <= 0) {
            leftClient(sess);   /* recv 0/-1: 반드시 leftClient 호출 */
            return 0;
        }
        buf[n] = '\0';
        sess->last_recv = time(NULL);

        /* '\n' 단위로 패킷 조립 */
        for (i = 0; i < n; i++) {
            if (buf[i] == '\n') {
                line[line_len] = '\0';
                if (line_len > 0) {
                    /* '\r' 제거 */
                    if (line_len > 0 && line[line_len - 1] == '\r')
                        line[--line_len] = '\0';
                    if (line_len > 0)
                        route_packet(sess, line);
                }
                line_len = 0;
            } else if (line_len < (int)sizeof(line) - 1) {
                line[line_len++] = buf[i];
            }
            /* 버퍼 초과 시 현재 라인 버림: line_len 리셋하지 않음으로써
             * 다음 '\n' 까지 계속 수신 (패킷 드롭) */
        }
    }

    return 0;
}
