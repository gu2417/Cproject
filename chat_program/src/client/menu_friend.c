#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../common/protocol.h"
#include "../common/utils.h"
#include "state.h"
#include "net.h"
#include "packet.h"
#include "menu_friend.h"

static void refresh_friend_list(void) {
    g_state.response_received = 0;
    send_packet(g_state.sock, "%s|", FRIEND_LIST_REQ);
    wait_response(5000);
}

static void print_friend_list(void) {
    if (g_friend_count == 0) {
        printf("  (친구 목록이 비어 있습니다)\n");
        return;
    }
    static const char *stat_labels[] = {"[대기]", "[수락]", "[차단]"};
    printf("\n  %-4s %-20s %-8s %s\n", "번호", "닉네임", "상태", "상태메시지");
    printf("  %-4s %-20s %-8s %s\n", "----", "--------------------", "--------", "----------");
    for (int i = 0; i < g_friend_count; i++) {
        FriendEntry *fe = &g_friend_list[i];
        const char *slabel = (fe->status >= 0 && fe->status <= 2)
                             ? stat_labels[fe->status] : "[?]";
        printf("  %-4d %-20s %-8s %s\n",
               i + 1, fe->nick, slabel,
               fe->status_msg[0] ? fe->status_msg : "");
    }
}

void ShowFriendMenu(void) {
    char ch[8];
    while (1) {
        printf("\n====== 친구 ======\n");
        refresh_friend_list();
        print_friend_list();

        printf("\n  a. 친구 추가\n");
        printf("  b. 요청 수락\n");
        printf("  c. 요청 거절\n");
        printf("  d. 친구 삭제\n");
        printf("  e. 차단\n");
        printf("  0. 돌아가기\n");
        printf("선택> ");
        fflush(stdout);

        if (!fgets(ch, sizeof(ch), stdin)) break;
        int n = (int)strlen(ch);
        if (n > 0 && ch[n - 1] == '\n') ch[--n] = '\0';

        if (ch[0] == '0') break;

        char target_id[21];

        if (ch[0] == 'a') {
            /* 친구 추가 */
            printf("추가할 사용자 ID: ");
            fflush(stdout);
            if (!fgets(target_id, (int)sizeof(target_id), stdin)) continue;
            n = (int)strlen(target_id);
            if (n > 0 && target_id[n - 1] == '\n') target_id[--n] = '\0';
            if (target_id[0] == '\0' || has_forbidden_char(target_id)) {
                printf("[오류] 유효하지 않은 ID입니다.\n");
                continue;
            }
            g_state.response_received = 0;
            send_packet(g_state.sock, "%s|%s", FRIEND_ADD_REQ, target_id);
            wait_response(5000);
        }
        else if (ch[0] == 'b') {
            /* 요청 수락 — 상대방 ID 직접 입력 (pending 목록에서 ID 확인) */
            printf("수락할 사용자 ID: ");
            fflush(stdout);
            if (!fgets(target_id, (int)sizeof(target_id), stdin)) continue;
            n = (int)strlen(target_id);
            if (n > 0 && target_id[n - 1] == '\n') target_id[--n] = '\0';
            if (target_id[0] == '\0') continue;
            /* FRIEND_ACCEPT 응답 없음 — 송신자에게 FRIEND_ACCEPT_NOTIFY 전송됨 */
            send_packet(g_state.sock, "%s|%s", FRIEND_ACCEPT, target_id);
            printf("[친구 요청 수락] %s\n", target_id);
        }
        else if (ch[0] == 'c') {
            /* 요청 거절 */
            printf("거절할 사용자 ID: ");
            fflush(stdout);
            if (!fgets(target_id, (int)sizeof(target_id), stdin)) continue;
            n = (int)strlen(target_id);
            if (n > 0 && target_id[n - 1] == '\n') target_id[--n] = '\0';
            if (target_id[0] == '\0') continue;
            send_packet(g_state.sock, "%s|%s", FRIEND_REJECT, target_id);
            printf("[친구 요청 거절] %s\n", target_id);
        }
        else if (ch[0] == 'd') {
            /* 친구 삭제 */
            if (g_friend_count == 0) { printf("  (삭제할 친구가 없습니다)\n"); continue; }
            printf("삭제할 번호: ");
            fflush(stdout);
            int idx = 0;
            scanf("%d", &idx);
            { int c; while ((c = getchar()) != '\n' && c != EOF); }
            if (idx < 1 || idx > g_friend_count) {
                printf("[오류] 잘못된 번호입니다.\n");
                continue;
            }
            send_packet(g_state.sock, "%s|%s",
                        FRIEND_DELETE, g_friend_list[idx - 1].id);
            printf("[친구 삭제] %s\n", g_friend_list[idx - 1].nick);
        }
        else if (ch[0] == 'e') {
            /* 차단 */
            if (g_friend_count == 0) { printf("  (차단할 친구가 없습니다)\n"); continue; }
            printf("차단할 번호: ");
            fflush(stdout);
            int idx = 0;
            scanf("%d", &idx);
            { int c; while ((c = getchar()) != '\n' && c != EOF); }
            if (idx < 1 || idx > g_friend_count) {
                printf("[오류] 잘못된 번호입니다.\n");
                continue;
            }
            send_packet(g_state.sock, "%s|%s",
                        FRIEND_BLOCK, g_friend_list[idx - 1].id);
            printf("[차단] %s\n", g_friend_list[idx - 1].nick);
        }

        if (!g_state.logged_in || !g_state.connected) break;
    }
}
