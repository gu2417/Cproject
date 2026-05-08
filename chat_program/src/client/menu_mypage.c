#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../common/protocol.h"
#include "../common/utils.h"
#include "state.h"
#include "net.h"
#include "menu_initial.h"
#include "menu_mypage.h"

void ShowMyPageMenu(void) {
    char ch[8];
    while (1) {
        /* 마이페이지 정보 요청 — MYPAGE_RES 핸들러가 출력 */
        g_state.response_received = 0;
        send_packet(g_state.sock, "%s|", MYPAGE_REQ);
        wait_response(5000);

        printf("\n  1. 닉네임 / 상태메시지 수정\n");
        printf("  2. 비밀번호 변경\n");
        printf("  3. 온라인 상태 변경\n");
        printf("  4. 내 방 목록\n");
        printf("  0. 돌아가기\n");
        printf("선택> ");
        fflush(stdout);

        if (!fgets(ch, sizeof(ch), stdin)) break;
        int n = (int)strlen(ch);
        if (n > 0 && ch[n - 1] == '\n') ch[--n] = '\0';

        if (ch[0] == '0') break;

        if (ch[0] == '1') {
            /* 닉네임 / 상태메시지 수정 */
            char new_nick[21], status_msg[101];

            printf("새 닉네임 (현재: %s, Enter = 유지): ",
                   g_state.nickname[0] ? g_state.nickname : g_state.user_id);
            fflush(stdout);
            if (!fgets(new_nick, (int)sizeof(new_nick), stdin)) continue;
            n = (int)strlen(new_nick);
            if (n > 0 && new_nick[n - 1] == '\n') new_nick[--n] = '\0';

            if (new_nick[0] == '\0') {
                /* Enter만 입력 → 현재 닉네임 유지 */
                strncpy(new_nick, g_state.nickname, 20);
                new_nick[20] = '\0';
            }
            if (has_forbidden_char(new_nick)) {
                printf("[오류] 닉네임에 금지 문자(: ; | \\n)가 포함되어 있습니다.\n");
                continue;
            }

            printf("상태메시지 (현재 값 지우려면 Enter): ");
            fflush(stdout);
            if (!fgets(status_msg, (int)sizeof(status_msg), stdin)) continue;
            n = (int)strlen(status_msg);
            if (n > 0 && status_msg[n - 1] == '\n') status_msg[--n] = '\0';

            /* PROFILE_UPDATE|<nickname>:<status_msg>  (status_msg content-last) */
            g_state.response_received = 0;
            send_packet(g_state.sock, "%s|%s:%s",
                        PROFILE_UPDATE, new_nick, status_msg);
            wait_response(5000);

            if (g_last_code == 0) {
                strncpy(g_state.nickname, new_nick, 20);
                g_state.nickname[20] = '\0';
            }
        }
        else if (ch[0] == '2') {
            /* 비밀번호 변경 */
            char old_plain[20], new_plain[20];
            char old_hash[65], new_hash[65];

            printf("현재 비밀번호: ");
            fflush(stdout);
            read_password(old_plain, (int)sizeof(old_plain));
            if (old_plain[0] == '\0') continue;

            printf("새 비밀번호 (1~19자): ");
            fflush(stdout);
            read_password(new_plain, (int)sizeof(new_plain));
            n = (int)strlen(new_plain);
            if (n < 1 || n > 19) {
                printf("[오류] 비밀번호는 1~19자여야 합니다.\n");
                continue;
            }

            sha256_hex(old_plain, old_hash);
            sha256_hex(new_plain, new_hash);

            /* PASS_CHANGE|<old_hash>:<new_hash> */
            g_state.response_received = 0;
            send_packet(g_state.sock, "%s|%s:%s",
                        PASS_CHANGE, old_hash, new_hash);
            wait_response(5000);
        }
        else if (ch[0] == '3') {
            /* 온라인 상태 변경 */
            printf("  1. 온라인\n");
            printf("  2. 바쁨\n");
            printf("  3. 숨김(오프라인처럼 보임)\n");
            printf("선택> ");
            fflush(stdout);
            char sc[4];
            if (!fgets(sc, sizeof(sc), stdin)) continue;
            int new_status;
            switch (sc[0]) {
                case '1': new_status = STATUS_ONLINE;    break;
                case '2': new_status = STATUS_BUSY;      break;
                case '3': new_status = STATUS_INVISIBLE; break;
                default:
                    printf("[오류] 잘못된 선택입니다.\n");
                    continue;
            }
            /* STATUS_CHANGE|<status> — 응답 없음 */
            send_packet(g_state.sock, "%s|%d", STATUS_CHANGE, new_status);
            g_state.online_status = new_status;
            printf("[상태 변경 완료]\n");
        }
        else if (ch[0] == '4') {
            /* 내 방 목록 — MY_ROOMS_RES 핸들러가 출력 */
            g_state.response_received = 0;
            send_packet(g_state.sock, "%s|", MY_ROOMS_REQ);
            wait_response(5000);
        }

        if (!g_state.logged_in || !g_state.connected) break;
    }
}
