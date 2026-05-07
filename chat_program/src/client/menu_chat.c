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
                printf("    /leave  — 채팅방 나가기\n");
                printf("    /help   — 이 도움말 출력\n");
                fflush(stdout);
                ReleaseMutex(g_console_mutex);
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
