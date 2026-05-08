#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/protocol.h"
#include "../common/utils.h"
#include "state.h"
#include "net.h"
#include "packet.h"
#include "chat_tui.h"
#include "menu_dm.h"

/* ─────────────────────────────────────────────────────────
 * show_dm_chat
 *   채팅룸 스타일 DM 루프.
 *   진입 전 current_dm_partner 를 설정하여 RecvMsg 스레드가
 *   해당 상대의 DM_RECV를 인라인으로 표시하도록 한다.
 * ───────────────────────────────────────────────────────── */
static void show_dm_chat(const char *partner_id, const char *partner_nick) {
    /* DM 컨텍스트 설정 */
    strncpy(g_state.current_dm_partner, partner_id,
            sizeof(g_state.current_dm_partner) - 1);
    g_state.current_dm_partner[sizeof(g_state.current_dm_partner) - 1] = '\0';
    strncpy(g_state.current_dm_partner_nick, partner_nick,
            sizeof(g_state.current_dm_partner_nick) - 1);
    g_state.current_dm_partner_nick[sizeof(g_state.current_dm_partner_nick) - 1] = '\0';

    /* TUI 초기화 후 히스토리 요청 */
    char tui_title[64];
    snprintf(tui_title, sizeof(tui_title), "DM: %s", partner_nick);
    tui_enter(tui_title);

    g_state.response_received = 0;
    send_packet(g_state.sock, "%s|%s:50", DM_HISTORY_REQ, partner_id);
    wait_response(5000);

    char input[MAX_PKT_SIZE];
    for (;;) {
        int len = tui_readline(input, (int)sizeof(input));
        if (len < 0) break;   /* 연결 끊김 */
        if (len == 0) {
            if (g_state.current_dm_partner[0] == '\0') break;
            continue;
        }

        if (strcmp(input, "/back") == 0) break;

        if (has_forbidden_char(input)) {
            tui_printf("  [오류] 메시지에 금지 문자(: ; | \\n)가 포함되어 있습니다.");
            continue;
        }

        /* DM_SEND|<to_id>:<content>  (content-last) */
        send_packet(g_state.sock, "%s|%s:%s", DM_SEND, partner_id, input);
        tui_printf("[나] %s", input);
    }

    tui_exit();

    /* DM 컨텍스트 초기화 */
    g_state.current_dm_partner[0]      = '\0';
    g_state.current_dm_partner_nick[0] = '\0';
}

void ShowDMMenu(void) {
    while (1) {
        printf("\n====== DM ======\n");

        /* DM 목록 요청 */
        g_state.response_received = 0;
        send_packet(g_state.sock, "%s|", DM_LIST_REQ);
        wait_response(5000);

        if (g_dm_count == 0) {
            printf("  (DM 대화 상대가 없습니다)\n");
        } else {
            printf("\n  %-4s %-20s %-8s %s\n", "번호", "닉네임", "안읽음", "마지막 메시지");
            printf("  %-4s %-20s %-8s %s\n", "----", "--------------------", "--------", "------------");
            for (int i = 0; i < g_dm_count; i++) {
                DmPartnerEntry *de = &g_dm_list[i];
                printf("  %-4d %-20s %-8d %s\n",
                       i + 1, de->partner_nick, de->unread,
                       de->last_msg[0] ? de->last_msg : "");
            }
        }

        printf("\n  n. 새 DM 시작\n");
        printf("  0. 돌아가기\n");
        printf("선택> ");
        fflush(stdout);

        char ch[8];
        if (!fgets(ch, sizeof(ch), stdin)) break;
        int n = (int)strlen(ch);
        if (n > 0 && ch[n - 1] == '\n') ch[--n] = '\0';

        if (ch[0] == '0') break;

        if (ch[0] == 'n') {
            /* 새 DM — 상대방 ID 입력 */
            printf("DM 보낼 사용자 ID: ");
            fflush(stdout);
            char target_id[21];
            if (!fgets(target_id, (int)sizeof(target_id), stdin)) continue;
            n = (int)strlen(target_id);
            if (n > 0 && target_id[n - 1] == '\n') target_id[--n] = '\0';
            if (target_id[0] == '\0' || has_forbidden_char(target_id)) {
                printf("[오류] 유효하지 않은 ID입니다.\n");
                continue;
            }
            show_dm_chat(target_id, target_id);
        }
        else {
            /* 번호로 기존 대화 상대 선택 */
            int idx = atoi(ch);
            if (idx < 1 || idx > g_dm_count) {
                printf("[오류] 잘못된 번호입니다.\n");
                continue;
            }
            DmPartnerEntry *de = &g_dm_list[idx - 1];
            show_dm_chat(de->partner_id, de->partner_nick);
        }

        if (!g_state.logged_in || !g_state.connected) break;
    }
}
