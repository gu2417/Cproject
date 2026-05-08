#include <winsock2.h>
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "globals.h"
#include "client_handler.h"
#include "file_io.h"
#include "broadcast.h"
#include "router.h"
#include "auth.h"
#include "user_store.h"
#include "room.h"
#include "friend.h"
#include "dm.h"
#include "message.h"

int main(void) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    WSADATA      wsa;
    SOCKET       listen_sock;
    SOCKADDR_IN  addr;
    int          opt = 1;

    /* WinSock2 초기화 */
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "[서버] WSAStartup 실패: %d\n", WSAGetLastError());
        return 1;
    }

    /* Mutex 초기화 */
    g_sessions_mutex = CreateMutex(NULL, FALSE, NULL);
    g_file_mutex     = CreateMutex(NULL, FALSE, NULL);
    g_console_mutex  = CreateMutex(NULL, FALSE, NULL);
    if (!g_sessions_mutex || !g_file_mutex || !g_console_mutex) {
        fprintf(stderr, "[서버] CreateMutex() 실패\n");
        return 1;
    }

    /* 데이터 파일 로드 */
    printf("[서버] 데이터 로드 중...\n");
    load_users(FILE_USERS);
    load_rooms(FILE_ROOMS);
    load_room_members(FILE_ROOM_MEMBERS);
    load_friends(FILE_FRIENDS);
    load_messages(FILE_MESSAGES);
    load_dm_reads(FILE_DM_READS);
    load_room_invites(FILE_ROOM_INVITES);
    load_user_settings(FILE_USER_SETTINGS);
    load_room_reads(FILE_ROOM_READS);
    restore_next_ids();
    printf("[서버] 로드 완료: 유저=%d, 방=%d, 친구=%d, 초대=%d\n",
           g_user_count, g_room_count, g_friend_count, g_room_invite_count);

    /* Listen 소켓 생성 */
    listen_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET) {
        fprintf(stderr, "[서버] socket() 실패: %d\n", WSAGetLastError());
        return 1;
    }

    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR,
               (const char *)&opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(DEFAULT_PORT);

    if (bind(listen_sock, (SOCKADDR *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        fprintf(stderr, "[서버] bind() 실패: %d\n", WSAGetLastError());
        closesocket(listen_sock);
        WSACleanup();
        return 1;
    }

    if (listen(listen_sock, SOMAXCONN) == SOCKET_ERROR) {
        fprintf(stderr, "[서버] listen() 실패: %d\n", WSAGetLastError());
        closesocket(listen_sock);
        WSACleanup();
        return 1;
    }

    printf("[서버] 포트 %d에서 대기 중...\n", DEFAULT_PORT);

    /* 패킷 핸들러 등록 */
    auth_init();
    user_store_init();
    room_init();
    friend_init();
    dm_init();
    message_init();

    /* Accept 루프 */
    while (1) {
        SOCKADDR_IN c_addr;
        int         c_len  = sizeof(c_addr);
        SOCKET      c_sock = accept(listen_sock,
                                    (SOCKADDR *)&c_addr, &c_len);
        int         slot   = -1;
        unsigned    tid;
        HANDLE      h;

        if (c_sock == INVALID_SOCKET) continue;

        /* 세션 슬롯 할당 */
        /* MUTEX: g_sessions_mutex */
        WaitForSingleObject(g_sessions_mutex, INFINITE);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!g_sessions[i].active) {
                slot = i;
                break;
            }
        }
        if (slot >= 0) {
            memset(&g_sessions[slot], 0, sizeof(g_sessions[slot]));
            g_sessions[slot].sock   = c_sock;
            g_sessions[slot].active = 1;
            g_sessions[slot].room_id = 0;
            g_session_count++;
        }
        ReleaseMutex(g_sessions_mutex);

        if (slot < 0) {
            /* 슬롯 부족: 연결 거부 */
            send(c_sock, "ERROR|SERVER_FULL\n", 18, 0);
            closesocket(c_sock);
            continue;
        }

        /* 클라이언트 스레드 생성 */
        h = (HANDLE)_beginthreadex(NULL, 0, HandleClient,
                                    (void *)(intptr_t)slot, 0, &tid);
        if (!h) {
            /* 스레드 생성 실패 → 슬롯 반환 */
            WaitForSingleObject(g_sessions_mutex, INFINITE);
            g_sessions[slot].active = 0;
            g_sessions[slot].sock   = INVALID_SOCKET;
            if (g_session_count > 0) g_session_count--;
            ReleaseMutex(g_sessions_mutex);
            closesocket(c_sock);
        } else {
            CloseHandle(h);   /* detach: 스레드 핸들 불필요 */
        }
    }

    WSACleanup();
    return 0;
}
