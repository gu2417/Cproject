#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include "../common/protocol.h"
#include "../common/utils.h"
#include "state.h"
#include "net.h"
#include "menu_initial.h"
#include "menu_main.h"

/* ── 비밀번호 입력 (getch 마스킹) ── */
void read_password(char *buf, int maxlen) {
    int i = 0, c;
    while (i < maxlen - 1) {
        c = _getch();
        if (c == '\r' || c == '\n') break;
        if ((c == '\b' || c == 127) && i > 0) {
            i--;
            printf("\b \b");
            fflush(stdout);
            continue;
        }
        if (c >= 32 && c < 127) {
            buf[i++] = (char)c;
            printf("*");
            fflush(stdout);
        }
    }
    buf[i] = '\0';
    printf("\n");
}

/* ── 한 줄 문자열 입력 (개행 제거) ── */
static void read_line(const char *prompt, char *buf, int maxlen) {
    printf("%s", prompt);
    fflush(stdout);
    if (!fgets(buf, maxlen, stdin)) {
        buf[0] = '\0';
        return;
    }
    int n = (int)strlen(buf);
    if (n > 0 && buf[n - 1] == '\n') buf[--n] = '\0';
    if (n > 0 && buf[n - 1] == '\r') buf[--n] = '\0';
}

/* ── 로그인 ── */
static void do_login(void) {
    char id[21], plain_pw[20], pw_hash[65];

    read_line("아이디: ", id, (int)sizeof(id));
    if (has_forbidden_char(id)) {
        printf("[오류] 아이디에 금지 문자가 포함되어 있습니다.\n");
        return;
    }
    printf("비밀번호: ");
    read_password(plain_pw, (int)sizeof(plain_pw));
    if (has_forbidden_char(plain_pw)) {
        printf("[오류] 비밀번호에 금지 문자가 포함되어 있습니다.\n");
        return;
    }

    sha256_hex(plain_pw, pw_hash);
    if (pw_hash[0] == '\0') {
        printf("[오류] 비밀번호 해시 생성 실패\n");
        return;
    }

    /* 상태 초기화 후 전송 */
    g_state.logged_in = 0;
    strncpy(g_state.user_id, id, sizeof(g_state.user_id) - 1);
    g_state.response_received = 0;
    send_packet(g_state.sock, "%s|%s:%s", LOGIN_REQ, id, pw_hash);
    wait_response(10000);

    if (!g_state.response_received) {
        printf("[오류] 서버 응답 시간 초과\n");
        return;
    }
    if (g_state.logged_in) {
        g_state.online_status = STATUS_ONLINE;
        ShowMainMenu();
        g_state.logged_in     = 0;
        g_state.online_status = STATUS_OFFLINE;
        g_state.user_id[0]    = '\0';
        g_state.nickname[0]   = '\0';
    }
}

/* ── 회원가입 ── */
static void do_register(void) {
    char id[21], nickname[21], plain_pw[20], confirm_pw[20], pw_hash[65];

    read_line("사용할 아이디 (최대 20자): ", id, (int)sizeof(id));
    if (has_forbidden_char(id) || id[0] == '\0') {
        printf("[오류] 유효하지 않은 아이디입니다.\n");
        return;
    }
    read_line("닉네임 (최대 20자): ", nickname, (int)sizeof(nickname));
    if (has_forbidden_char(nickname) || nickname[0] == '\0') {
        printf("[오류] 유효하지 않은 닉네임입니다.\n");
        return;
    }
    printf("비밀번호 (최대 19자): ");
    read_password(plain_pw, (int)sizeof(plain_pw));
    if (has_forbidden_char(plain_pw) || plain_pw[0] == '\0') {
        printf("[오류] 유효하지 않은 비밀번호입니다.\n");
        return;
    }
    printf("비밀번호 확인: ");
    read_password(confirm_pw, (int)sizeof(confirm_pw));
    if (strcmp(plain_pw, confirm_pw) != 0) {
        printf("[오류] 비밀번호가 일치하지 않습니다.\n");
        return;
    }

    sha256_hex(plain_pw, pw_hash);
    if (pw_hash[0] == '\0') {
        printf("[오류] 비밀번호 해시 생성 실패\n");
        return;
    }

    g_state.response_received = 0;
    send_packet(g_state.sock, "%s|%s:%s:%s", REGISTER_REQ, id, pw_hash, nickname);
    wait_response(10000);

    if (!g_state.response_received)
        printf("[오류] 서버 응답 시간 초과\n");
    /* 성공/실패 메시지는 packet_parse에서 출력 */
}

/* ── 초기 메뉴 루프 ── */
void InitialMenu(void) {
    char ch[4];
    while (1) {
        if (!g_state.connected) {
            printf("\n서버와의 연결이 끊어졌습니다. 프로그램을 종료합니다.\n");
            break;
        }
        printf("\n==============================\n");
        printf("  C 채팅 프로그램\n");
        printf("==============================\n");
        printf("  1. 로그인\n");
        printf("  2. 회원가입\n");
        printf("  0. 종료\n");
        printf("선택> ");
        fflush(stdout);

        if (!fgets(ch, sizeof(ch), stdin)) break;
        switch (ch[0]) {
            case '1': do_login();    break;
            case '2': do_register(); break;
            case '0': return;
            default:  printf("잘못된 선택입니다.\n"); break;
        }
    }
}
