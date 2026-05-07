#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../common/protocol.h"
#include "../common/utils.h"
#include "state.h"
#include "net.h"
#include "menu_settings.h"

static void print_current_settings(void) {
    static const char *ts_labels[] = {"HH:MM", "HH:MM:SS", "MM-DD HH:MM"};
    int tf = g_state.ts_format;
    if (tf < 0 || tf > 2) tf = 0;
    printf("\n  ─────────────────────────────\n");
    printf("  메시지 색상 : %s\n", g_state.msg_color[0]  ? g_state.msg_color  : "white");
    printf("  닉네임 색상 : %s\n", g_state.nick_color[0] ? g_state.nick_color : "cyan");
    printf("  테마         : %s\n", g_state.theme[0]      ? g_state.theme      : "dark");
    printf("  시간 형식    : %s\n", ts_labels[tf]);
    printf("  방해금지(DND): %s\n", g_state.dnd ? "켜짐" : "꺼짐");
    printf("  ─────────────────────────────\n");
}

void ShowSettingsMenu(void) {
    /* 현재 설정 로드 — SETTINGS_RES 핸들러가 g_state에 저장 */
    g_state.response_received = 0;
    send_packet(g_state.sock, "%s|", SETTINGS_REQ);
    wait_response(5000);
    print_current_settings();

    static const char *color_opts[] = {"white", "cyan", "yellow", "red", "green"};
    char ch[8];

    while (1) {
        printf("\n  1. 메시지 색상 변경\n");
        printf("  2. 닉네임 색상 변경\n");
        printf("  3. 테마 변경\n");
        printf("  4. 시간 형식 변경\n");
        printf("  5. 방해금지(DND) 토글\n");
        printf("  s. 저장 및 적용\n");
        printf("  0. 돌아가기\n");
        printf("선택> ");
        fflush(stdout);

        if (!fgets(ch, sizeof(ch), stdin)) break;
        int n = (int)strlen(ch);
        if (n > 0 && ch[n - 1] == '\n') ch[--n] = '\0';

        if (ch[0] == '0') break;

        if (ch[0] == '1' || ch[0] == '2') {
            /* 색상 변경 */
            printf("  색상 선택 (white/cyan/yellow/red/green): ");
            fflush(stdout);
            char col[16];
            if (!fgets(col, (int)sizeof(col), stdin)) continue;
            n = (int)strlen(col);
            if (n > 0 && col[n - 1] == '\n') col[--n] = '\0';

            int valid = 0;
            for (int i = 0; i < 5; i++)
                if (strcmp(col, color_opts[i]) == 0) { valid = 1; break; }
            if (!valid) {
                printf("[오류] white/cyan/yellow/red/green 중 하나를 입력하세요.\n");
                continue;
            }
            if (ch[0] == '1') {
                strncpy(g_state.msg_color,  col, 15); g_state.msg_color[15]  = '\0';
            } else {
                strncpy(g_state.nick_color, col, 15); g_state.nick_color[15] = '\0';
            }
            print_current_settings();
        }
        else if (ch[0] == '3') {
            /* 테마 변경 */
            printf("  테마 선택 (dark/light): ");
            fflush(stdout);
            char th[12];
            if (!fgets(th, (int)sizeof(th), stdin)) continue;
            n = (int)strlen(th);
            if (n > 0 && th[n - 1] == '\n') th[--n] = '\0';
            if (strcmp(th, "dark") != 0 && strcmp(th, "light") != 0) {
                printf("[오류] dark 또는 light만 가능합니다.\n");
                continue;
            }
            strncpy(g_state.theme, th, 10);
            g_state.theme[10] = '\0';
            print_current_settings();
        }
        else if (ch[0] == '4') {
            /* 시간 형식 변경 */
            printf("  형식 선택 (0=HH:MM  1=HH:MM:SS  2=MM-DD HH:MM): ");
            fflush(stdout);
            char tf[4];
            if (!fgets(tf, (int)sizeof(tf), stdin)) continue;
            int fmt = tf[0] - '0';
            if (fmt < 0 || fmt > 2) {
                printf("[오류] 0, 1, 2 중 하나를 입력하세요.\n");
                continue;
            }
            g_state.ts_format = fmt;
            print_current_settings();
        }
        else if (ch[0] == '5') {
            /* DND 토글 */
            g_state.dnd = g_state.dnd ? 0 : 1;
            printf("  방해금지: %s\n", g_state.dnd ? "켜짐" : "꺼짐");
        }
        else if (ch[0] == 's') {
            /* 저장 — SETTINGS_UPDATE|<msg_color>:<nick_color>:<theme>:<ts_format>:<dnd> */
            g_state.response_received = 0;
            send_packet(g_state.sock, "%s|%s:%s:%s:%d:%d",
                        SETTINGS_UPDATE,
                        g_state.msg_color[0]  ? g_state.msg_color  : "white",
                        g_state.nick_color[0] ? g_state.nick_color : "cyan",
                        g_state.theme[0]      ? g_state.theme      : "dark",
                        g_state.ts_format,
                        g_state.dnd);
            wait_response(5000);
        }

        if (!g_state.logged_in || !g_state.connected) break;
    }
}
