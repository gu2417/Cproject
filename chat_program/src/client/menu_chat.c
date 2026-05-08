#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../common/protocol.h"
#include "../common/utils.h"
#include "state.h"
#include "net.h"
#include "menu_chat.h"

/* ─────────────────────────────────────────────────────────
 * ShowChatRoom
 *   room_id   : 입장할 방 ID
 *   room_name : 방 이름 (배너 표시용)
 *
 *  슬래시 명령어 (P0):
 *    /leave  — ROOM_LEAVE 전송 후 루프 탈출
 *    /help   — 사용 가능 명령어 출력
 * ───────────────────────────────────────────────────────── */
void ShowChatRoom(int room_id, const char *room_name) {
    g_state.current_room_id = room_id;
    strncpy(g_state.current_room_name, room_name,
            sizeof(g_state.current_room_name) - 1);
    g_state.current_room_name[sizeof(g_state.current_room_name) - 1] = '\0';

    WaitForSingleObject(g_console_mutex, INFINITE);
    printf("\n══════════════════════════════\n");
    printf("  [%s]\n", room_name);
    printf("  /help 명령어 목록 | /leave 나가기\n");
    printf("══════════════════════════════\n");
    fflush(stdout);
    ReleaseMutex(g_console_mutex);

    /* 입장 시 최근 50개 메시지 히스토리 요청 */
    g_state.response_received = 0;
    send_packet(g_state.sock, "%s|%d:50", ROOM_HISTORY_REQ, room_id);
    wait_response(5000);

    char input[MAX_PKT_SIZE];

    while (g_state.current_room_id == room_id && g_state.connected) {
        printf("> ");
        fflush(stdout);

        if (!fgets(input, (int)sizeof(input), stdin)) break;

        /* 개행 제거 */
        int len = (int)strlen(input);
        if (len > 0 && input[len - 1] == '\n') input[--len] = '\0';
        if (len > 0 && input[len - 1] == '\r') input[--len] = '\0';
        if (len == 0) {
            /* 강퇴 후 빈 Enter로 루프 탈출 */
            if (g_state.current_room_id != room_id) break;
            continue;
        }

        if (input[0] == '/') {
            /* ── 슬래시 명령 ── */
            if (strcmp(input, "/leave") == 0) {
                send_packet(g_state.sock, "%s|%d", ROOM_LEAVE, room_id);
                break;
            }
            else if (strcmp(input, "/help") == 0) {
                WaitForSingleObject(g_console_mutex, INFINITE);
                printf("  사용 가능한 명령어:\n");
                printf("    /leave            — 채팅방 나가기\n");
                printf("    /members          — 멤버 목록\n");
                printf("    /invite <ID>      — 사용자 초대\n");
                printf("    /kick <ID>        — 사용자 강퇴 (방장)\n");
                printf("    /notice <내용>    — 공지 설정 (방장/관리자)\n");
                printf("    /edit <ID> <내용> — 내 메시지 수정 (5분 내)\n");
                printf("    /delete <ID>      — 내 메시지 삭제\n");
                printf("    /pin <ID>         — 메시지 핀 (방장/관리자), 0이면 해제\n");
                printf("    /w <ID> <내용>    — 귓속말\n");
                printf("    /opennick <닉>    — 오픈채팅 닉네임 변경\n");
                printf("    /mute             — 알림 토글\n");
                printf("    /me <동작>        — 동작 메시지 (/me 가 달려가다)\n");
                printf("    /reply <ID> <내용> — 메시지 인용 답장\n");
                printf("    /search <키워드>  — 현재 방 메시지 검색\n");
                printf("    /help             — 이 도움말 출력\n");
                fflush(stdout);
                ReleaseMutex(g_console_mutex);
            }
            else if (strcmp(input, "/members") == 0) {
                g_state.response_received = 0;
                send_packet(g_state.sock, "%s|%d", ROOM_MEMBERS_REQ, room_id);
                wait_response(5000);
            }
            else if (strncmp(input, "/invite ", 8) == 0) {
                const char *target = input + 8;
                if (*target)
                    send_packet(g_state.sock, "%s|%d:%s",
                                ROOM_INVITE, room_id, target);
            }
            else if (strncmp(input, "/kick ", 6) == 0) {
                const char *target = input + 6;
                if (*target)
                    send_packet(g_state.sock, "%s|%d:%s",
                                ROOM_KICK, room_id, target);
            }
            else if (strncmp(input, "/notice ", 8) == 0) {
                const char *notice = input + 8;
                if (*notice)
                    send_packet(g_state.sock, "%s|%d:%s",
                                ROOM_SET_NOTICE, room_id, notice);
            }
            else if (strncmp(input, "/edit ", 6) == 0) {
                /* /edit <msg_id> <new_content> */
                char *rest = input + 6;
                char *sp = strchr(rest, ' ');
                if (sp && *(sp + 1)) {
                    *sp = '\0';
                    int mid = atoi(rest);
                    const char *new_content = sp + 1;
                    if (mid > 0 && *new_content)
                        send_packet(g_state.sock, "%s|%d:%d:%s",
                                    MSG_EDIT, room_id, mid, new_content);
                } else {
                    WaitForSingleObject(g_console_mutex, INFINITE);
                    printf("  사용법: /edit <메시지ID> <새내용>\n");
                    fflush(stdout);
                    ReleaseMutex(g_console_mutex);
                }
            }
            else if (strncmp(input, "/delete ", 8) == 0) {
                int mid = atoi(input + 8);
                if (mid > 0)
                    send_packet(g_state.sock, "%s|%d:%d",
                                MSG_DELETE, room_id, mid);
            }
            else if (strncmp(input, "/pin ", 5) == 0) {
                int mid = atoi(input + 5);
                send_packet(g_state.sock, "%s|%d:%d",
                            MSG_PIN, room_id, mid);
            }
            else if (strncmp(input, "/w ", 3) == 0) {
                /* /w <target_id> <content> */
                char *rest = input + 3;
                char *sp = strchr(rest, ' ');
                if (sp && *(sp + 1)) {
                    *sp = '\0';
                    const char *target  = rest;
                    const char *content = sp + 1;
                    if (*target && *content)
                        send_packet(g_state.sock, "%s|%d:%s:%s",
                                    WHISPER, room_id, target, content);
                } else {
                    WaitForSingleObject(g_console_mutex, INFINITE);
                    printf("  사용법: /w <상대ID> <내용>\n");
                    fflush(stdout);
                    ReleaseMutex(g_console_mutex);
                }
            }
            else if (strncmp(input, "/opennick ", 10) == 0) {
                const char *new_nick = input + 10;
                if (*new_nick) {
                    g_state.response_received = 0;
                    send_packet(g_state.sock, "%s|%d:%s",
                                ROOM_SET_OPEN_NICK, room_id, new_nick);
                    wait_response(3000);
                }
            }
            else if (strcmp(input, "/mute") == 0) {
                g_state.response_received = 0;
                send_packet(g_state.sock, "%s|%d", ROOM_MUTE_TOGGLE, room_id);
                wait_response(3000);
            }
            else if (strncmp(input, "/me ", 4) == 0) {
                /* /me <동작> — 서버가 MSG_TYPE_ME로 처리 후 브로드캐스트 */
                const char *me_content = input + 4;
                if (*me_content)
                    send_packet(g_state.sock, "%s|%d:%s",
                                ROOM_MSG, room_id, input);
            }
            else if (strncmp(input, "/reply ", 7) == 0) {
                /* /reply <msg_id> <내용> */
                char *rest = input + 7;
                char *sp = strchr(rest, ' ');
                if (sp && *(sp + 1)) {
                    *sp = '\0';
                    int mid = atoi(rest);
                    const char *reply_content = sp + 1;
                    if (mid > 0 && *reply_content)
                        send_packet(g_state.sock, "%s|%d:%d:%s",
                                    MSG_REPLY, room_id, mid, reply_content);
                } else {
                    WaitForSingleObject(g_console_mutex, INFINITE);
                    printf("  사용법: /reply <메시지ID> <내용>\n");
                    fflush(stdout);
                    ReleaseMutex(g_console_mutex);
                }
            }
            else if (strncmp(input, "/search ", 8) == 0) {
                /* /search <키워드> — MSG_SEARCH_RES 핸들러가 출력 */
                const char *keyword = input + 8;
                if (*keyword) {
                    g_state.response_received = 0;
                    send_packet(g_state.sock, "%s|%d:%s",
                                MSG_SEARCH, room_id, keyword);
                    wait_response(5000);
                }
            }
            else {
                WaitForSingleObject(g_console_mutex, INFINITE);
                printf("  알 수 없는 명령어입니다. /help 참조\n");
                fflush(stdout);
                ReleaseMutex(g_console_mutex);
            }
        }
        else {
            /* ── 일반 메시지 전송 ── */
            if (has_forbidden_char(input)) {
                WaitForSingleObject(g_console_mutex, INFINITE);
                printf("  [오류] 메시지에 금지 문자(: ; | \\n)가 포함되어 있습니다.\n");
                fflush(stdout);
                ReleaseMutex(g_console_mutex);
                continue;
            }
            /* ROOM_MSG|<room_id>:<content>  (content-last) */
            send_packet(g_state.sock, "%s|%d:%s", ROOM_MSG, room_id, input);
        }
    }

    /* 채팅방 컨텍스트 초기화 */
    g_state.current_room_id      = 0;
    g_state.current_room_name[0] = '\0';
    g_state.current_dm_partner[0] = '\0';
}
