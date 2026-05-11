#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../common/protocol.h"
#include "../common/utils.h"
#include "state.h"
#include "net.h"
#include "chat_tui.h"
#include "menu_chat.h"

/* ─────────────────────────────────────────────────────────
 * ShowChatRoom
 *   room_id   : 입장할 방 ID
 *   room_name : 방 이름
 * ───────────────────────────────────────────────────────── */
/* 선택한 채팅방에 들어가 메시지 명령을 처리한다. */
void ShowChatRoom(int room_id, const char *room_name) {
    g_state.current_room_id = room_id;
    strncpy(g_state.current_room_name, room_name,
            sizeof(g_state.current_room_name) - 1);
    g_state.current_room_name[sizeof(g_state.current_room_name) - 1] = '\0';

    /* TUI 초기화 후 히스토리 요청 — RecvMsg가 각 메시지를 tui_puts로 출력 */
    tui_enter(room_name);
    tui_set_notice(g_state.current_room_notice);
    g_state.response_received = 0;
    send_packet(g_state.sock, "%s|%d:50", ROOM_HISTORY_REQ, room_id);
    wait_response(5000);

    char input[MAX_PKT_SIZE];

    while (g_state.current_room_id == room_id && g_state.connected) {
        int len = tui_readline(input, (int)sizeof(input));
        if (len < 0) break;   /* 연결 끊김 또는 강퇴 */
        if (len == 0) {
            if (g_state.current_room_id != room_id) break;
            continue;
        }

        if (input[0] == '/') {
            /* ── 슬래시 명령 ── */
            if (strcmp(input, "/leave") == 0) {
                send_packet(g_state.sock, "%s|%d", ROOM_LEAVE, room_id);
                break;
            }
            else if (strcmp(input, "/roomdelete") == 0) {
                send_packet(g_state.sock, "%s|%d", ROOM_DELETE, room_id);
            }
            else if (strcmp(input, "/help") == 0) {
                tui_printf("  사용 가능한 명령어:");
                tui_printf("    /leave            — 채팅방 나가기");
                tui_printf("    /roomdelete       방 삭제 (방장)");
                tui_printf("    /members          — 멤버 목록");
                tui_printf("    /invite <ID>      — 사용자 초대");
                tui_printf("    /kick <ID>        — 사용자 강퇴 (방장)");
                tui_printf("    /notice <내용>    — 공지 설정 (방장/관리자)");
                tui_printf("    /noticeoff        — 공지 해제 (방장/관리자)");
                tui_printf("    /edit <ID> <내용> — 내 메시지 수정 (5분 내)");
                tui_printf("    /delete <ID>      — 내 메시지 삭제");
                tui_printf("    /pin <ID>         — 메시지 핀 (방장/관리자), 0이면 해제");
                tui_printf("    /w <ID> <내용>    — 귓속말");
                tui_printf("    /opennick <닉>    — 오픈채팅 닉네임 변경");
                tui_printf("    /mute             — 알림 토글");
                tui_printf("    /me <동작>        — 동작 메시지");
                tui_printf("    /reply <ID> <내용> — 메시지 인용 답장");
                tui_printf("    /search <키워드>  — 현재 방 메시지 검색");
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
            else if (strcmp(input, "/noticeoff") == 0) {
                send_packet(g_state.sock, "%s|%d:",
                            ROOM_SET_NOTICE, room_id);
            }
            else if (strncmp(input, "/edit ", 6) == 0) {
                char *rest = input + 6;
                char *sp   = strchr(rest, ' ');
                if (sp && *(sp + 1)) {
                    *sp = '\0';
                    int mid = atoi(rest);
                    const char *new_content = sp + 1;
                    if (mid > 0 && *new_content)
                        send_packet(g_state.sock, "%s|%d:%d:%s",
                                    MSG_EDIT, room_id, mid, new_content);
                } else {
                    tui_printf("  사용법: /edit <메시지ID> <새내용>");
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
                send_packet(g_state.sock, "%s|%d:%d", MSG_PIN, room_id, mid);
            }
            else if (strncmp(input, "/w ", 3) == 0) {
                char *rest = input + 3;
                char *sp   = strchr(rest, ' ');
                if (sp && *(sp + 1)) {
                    *sp = '\0';
                    const char *target  = rest;
                    const char *content = sp + 1;
                    if (*target && *content)
                        send_packet(g_state.sock, "%s|%d:%s:%s",
                                    WHISPER, room_id, target, content);
                } else {
                    tui_printf("  사용법: /w <상대ID> <내용>");
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
                if (*(input + 4))
                    send_packet(g_state.sock, "%s|%d:%s", ROOM_MSG, room_id, input);
            }
            else if (strncmp(input, "/reply ", 7) == 0) {
                char *rest = input + 7;
                char *sp   = strchr(rest, ' ');
                if (sp && *(sp + 1)) {
                    *sp = '\0';
                    int mid = atoi(rest);
                    const char *reply_content = sp + 1;
                    if (mid > 0 && *reply_content)
                        send_packet(g_state.sock, "%s|%d:%d:%s",
                                    MSG_REPLY, room_id, mid, reply_content);
                } else {
                    tui_printf("  사용법: /reply <메시지ID> <내용>");
                }
            }
            else if (strncmp(input, "/search ", 8) == 0) {
                const char *keyword = input + 8;
                if (*keyword) {
                    g_state.response_received = 0;
                    send_packet(g_state.sock, "%s|%d:%s",
                                MSG_SEARCH, room_id, keyword);
                    wait_response(5000);
                }
            }
            else {
                tui_printf("  알 수 없는 명령어입니다. /help 참조");
            }
        }
        else {
            /* ── 일반 메시지 전송 ── */
            if (has_forbidden_char(input)) {
                tui_printf("  [오류] 메시지에 금지 문자(: ; | \\n)가 포함되어 있습니다.");
                continue;
            }
            send_packet(g_state.sock, "%s|%d:%s", ROOM_MSG, room_id, input);
        }
    }

    tui_exit();
    g_state.current_room_id       = 0;
    g_state.current_room_name[0]  = '\0';
    g_state.current_room_notice[0] = '\0';
    g_state.current_dm_partner[0] = '\0';
}
