#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../common/protocol.h"
#include "../common/utils.h"
#include "state.h"
#include "net.h"
#include "menu_initial.h"
#include "menu_main.h"
#include "menu_chat.h"
#include "menu_friend.h"
#include "menu_dm.h"
#include "menu_mypage.h"
#include "menu_settings.h"

/* ─────────────────────────────────────────────────────────
 * ShowRoomMenu — 그룹채팅(is_open=0) 또는 오픈채팅(is_open=1)
 * ───────────────────────────────────────────────────────── */
void ShowRoomMenu(int is_open) {
    const char *title = is_open ? "오픈채팅" : "채팅방";
    const char *type  = is_open ? "open"    : "group";

    while (1) {
        printf("\n====== %s ======\n", title);

        /* 방 목록 요청 */
        g_state.response_received = 0;
        send_packet(g_state.sock, "%s|%s", ROOM_LIST_REQ, type);
        wait_response(5000);

        printf("\n  1. 방 입장\n");
        printf("  2. 방 생성\n");
        printf("  3. 방 검색\n");
        printf("  0. 돌아가기\n");
        printf("선택> ");
        fflush(stdout);

        char ch[4];
        if (!fgets(ch, sizeof(ch), stdin)) break;

        if (ch[0] == '0') break;

        if (ch[0] == '3') {
            /* ── 방 검색 ── */
            char keyword[51] = "";
            printf("검색어: ");
            fflush(stdout);
            if (!fgets(keyword, (int)sizeof(keyword), stdin)) continue;
            int kn = (int)strlen(keyword);
            if (kn > 0 && keyword[kn-1] == '\n') keyword[--kn] = '\0';
            if (keyword[0] == '\0') continue;
            g_state.response_received = 0;
            send_packet(g_state.sock, "%s|%s", ROOM_SEARCH, keyword);
            wait_response(5000);
        }
        else if (ch[0] == '1') {
            /* ── 방 입장 ── */
            printf("방 ID: ");
            fflush(stdout);
            int room_id = 0;
            scanf("%d", &room_id);
            /* 입력 버퍼 flush */
            int c; while ((c = getchar()) != '\n' && c != EOF);

            char plain_pw[11] = "";
            printf("비밀번호 (없으면 Enter): ");
            fflush(stdout);
            read_password(plain_pw, (int)sizeof(plain_pw));

            if (plain_pw[0] != '\0') {
                char pw_hash[65];
                sha256_hex(plain_pw, pw_hash);
                g_state.response_received = 0;
                send_packet(g_state.sock, "%s|%d:%s", ROOM_JOIN, room_id, pw_hash);
            } else {
                g_state.response_received = 0;
                send_packet(g_state.sock, "%s|%d", ROOM_JOIN, room_id);
            }
            wait_response(5000);

            if (g_last_code == ROOM_JOIN_OK && g_state.current_room_id == room_id) {
                ShowChatRoom(room_id, g_state.current_room_name);
            }
        }
        else if (ch[0] == '2') {
            /* ── 방 생성 ── */
            char name[31] = "", plain_pw[11] = "";
            int  max_users = 0;

            printf("방 이름 (최대 30자): ");
            fflush(stdout);
            if (!fgets(name, (int)sizeof(name), stdin)) continue;
            int n = (int)strlen(name);
            if (n > 0 && name[n-1] == '\n') name[--n] = '\0';
            if (has_forbidden_char(name) || name[0] == '\0') {
                printf("[오류] 유효하지 않은 방 이름입니다.\n");
                continue;
            }

            printf("최대 인원 (2~%d): ", MAX_ROOM_MEMBERS);
            fflush(stdout);
            scanf("%d", &max_users);
            { int c2; while ((c2 = getchar()) != '\n' && c2 != EOF); }
            if (max_users < 2 || max_users > MAX_ROOM_MEMBERS)
                max_users = MAX_ROOM_MEMBERS;

            printf("비밀번호 (없으면 Enter, 최대 10자): ");
            fflush(stdout);
            read_password(plain_pw, (int)sizeof(plain_pw));

            g_state.response_received = 0;
            if (plain_pw[0] != '\0') {
                char pw_hash[65];
                sha256_hex(plain_pw, pw_hash);
                send_packet(g_state.sock, "%s|%s:%d:%d:%s",
                            ROOM_CREATE, name, max_users, is_open, pw_hash);
            } else {
                send_packet(g_state.sock, "%s|%s:%d:%d:",
                            ROOM_CREATE, name, max_users, is_open);
            }
            wait_response(5000);

            /* 생성 성공 시 자동 입장 */
            if (g_last_code == ROOM_CREATE_OK && g_last_room_id > 0) {
                int new_id = g_last_room_id;
                g_state.response_received = 0;
                send_packet(g_state.sock, "%s|%d", ROOM_JOIN, new_id);
                wait_response(5000);
                if (g_last_code == ROOM_JOIN_OK &&
                    g_state.current_room_id == new_id) {
                    ShowChatRoom(new_id, g_state.current_room_name);
                }
            }
        }
    }
}

/* ─────────────────────────────────────────────────────────
 * ShowMainMenu — 메인 로비
 * ───────────────────────────────────────────────────────── */
void ShowMainMenu(void) {
    char ch[4];
    while (1) {
        printf("\n==============================\n");
        printf("  환영합니다, %s 님!\n",
               g_state.nickname[0] ? g_state.nickname : g_state.user_id);
        printf("==============================\n");
        printf("  1. 친구 목록\n");
        printf("  2. 채팅방\n");
        printf("  3. 오픈채팅\n");
        printf("  4. DM\n");
        printf("  5. 마이페이지\n");
        printf("  6. 설정\n");
        printf("  7. 내 방 목록\n");
        printf("  8. 로그아웃\n");
        printf("선택> ");
        fflush(stdout);

        if (!fgets(ch, sizeof(ch), stdin)) break;

        switch (ch[0]) {
            case '1': ShowFriendMenu();   break;
            case '2': ShowRoomMenu(0);    break;  /* 그룹채팅 */
            case '3': ShowRoomMenu(1);    break;  /* 오픈채팅 */
            case '4': ShowDMMenu();       break;
            case '5': ShowMyPageMenu();   break;
            case '6': ShowSettingsMenu(); break;
            case '7':
                g_state.response_received = 0;
                send_packet(g_state.sock, "%s|", MY_ROOMS_REQ);
                wait_response(5000);
                break;
            case '8':
                send_packet(g_state.sock, "%s|", LOGOUT_REQ);
                printf("로그아웃합니다.\n");
                return;
            default:
                printf("잘못된 선택입니다.\n");
                break;
        }

        if (!g_state.logged_in || !g_state.connected) break;
    }
}
