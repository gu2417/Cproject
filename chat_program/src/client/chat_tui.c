#include <winsock2.h>
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#include <conio.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <wchar.h>
#include "../common/protocol.h"
#include "state.h"
#include "chat_tui.h"

/* ── 콘솔 출력 핸들 ──────────────────────────────────── */
static HANDLE s_hout = INVALID_HANDLE_VALUE;

static void hout_init(void) {
    if (s_hout == INVALID_HANDLE_VALUE)
        s_hout = GetStdHandle(STD_OUTPUT_HANDLE);
}

/* WriteFile로 직접 출력 — fputs/fflush 보다 확실하게 반영됨 */
static void con_write(const char *s) {
    DWORD written;
    WriteFile(s_hout, s, (DWORD)strlen(s), &written, NULL);
}


/* ── 뷰포트 기준 커서 이동 ───────────────────────────── */
static void get_viewport(int *top, int *left) {
    CONSOLE_SCREEN_BUFFER_INFO ci;
    if (GetConsoleScreenBufferInfo(s_hout, &ci)) {
        *top  = ci.srWindow.Top;
        *left = ci.srWindow.Left;
    } else { *top = 0; *left = 0; }
}

static void get_size(int *rows, int *cols) {
    CONSOLE_SCREEN_BUFFER_INFO ci;
    if (GetConsoleScreenBufferInfo(s_hout, &ci)) {
        *cols = ci.srWindow.Right  - ci.srWindow.Left + 1;
        *rows = ci.srWindow.Bottom - ci.srWindow.Top  + 1;
    } else { *cols = 80; *rows = 24; }
}

/* row, col: 1-based, 뷰포트 기준 */
static void win_move(int row, int col) {
    int vtop, vleft;
    get_viewport(&vtop, &vleft);
    COORD pos = { (SHORT)(vleft + col - 1), (SHORT)(vtop + row - 1) };
    SetConsoleCursorPosition(s_hout, pos);
}

static void clear_line_at(int row, int cols) {
    int vtop, vleft;
    get_viewport(&vtop, &vleft);
    COORD pos = { (SHORT)vleft, (SHORT)(vtop + row - 1) };
    DWORD w;
    FillConsoleOutputCharacterA(s_hout, ' ', cols, pos, &w);
    FillConsoleOutputAttribute(s_hout,
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE, cols, pos, &w);
}

static void cursor_visible(BOOL show) {
    CONSOLE_CURSOR_INFO ci;
    if (GetConsoleCursorInfo(s_hout, &ci)) {
        ci.bVisible = show;
        SetConsoleCursorInfo(s_hout, &ci);
    }
}

/* ── 메시지 링버퍼 ───────────────────────────────────── */
static char s_msgs[TUI_MSG_ROWS][TUI_MSG_WIDTH];
static int  s_head  = 0;
static int  s_count = 0;
static char s_room_name[64];

/* ── 입력 버퍼 (g_console_mutex 보호) ────────────────── */
static char s_input[MAX_PKT_SIZE];
static int  s_input_len = 0;

int g_tui_active = 0;

/* ── 전체 화면 재그리기 (g_console_mutex 보유 상태) ───── */
static void do_redraw(void) {
    int rows, cols;
    get_size(&rows, &cols);
    int msg_rows = rows - 3;
    if (msg_rows < 1) msg_rows = 1;

    cursor_visible(FALSE);

    /* 헤더 (1행) */
    clear_line_at(1, cols);
    win_move(1, 1);
    char hdr[TUI_MSG_WIDTH];
    snprintf(hdr, sizeof(hdr), " [%s]  /help 명령어 | /leave 나가기",
             s_room_name);
    if ((int)strlen(hdr) >= cols) hdr[cols - 1] = '\0';
    con_write(hdr);

    /* 메시지 영역 (2행 ~ msg_rows+1행) */
    int start = (s_count > msg_rows) ? (s_count - msg_rows) : 0;
    for (int r = 0; r < msg_rows; r++) {
        clear_line_at(r + 2, cols);
        int idx = start + r;
        if (idx < s_count) {
            win_move(r + 2, 1);
            int bi = (s_head - s_count + idx + TUI_MSG_ROWS) % TUI_MSG_ROWS;
            con_write(s_msgs[bi]);
        }
    }

    /* 구분선 */
    clear_line_at(rows - 1, cols);
    win_move(rows - 1, 1);
    char sep[256];
    int slen = (cols - 1 < 255) ? cols - 1 : 255;
    memset(sep, '-', slen);
    sep[slen] = '\0';
    con_write(sep);

    /* 입력줄 */
    clear_line_at(rows, cols);
    win_move(rows, 1);
    con_write("> ");
    con_write(s_input);

    cursor_visible(TRUE);
}

/* ── 입력줄만 갱신 (g_console_mutex 보유 상태) ─────────── */
static void redraw_input(void) {
    int rows, cols;
    get_size(&rows, &cols);
    cursor_visible(FALSE);
    clear_line_at(rows, cols);
    win_move(rows, 1);
    con_write("> ");
    con_write(s_input);
    cursor_visible(TRUE);
}

/* ── 마지막 UTF-8 문자 제거 ──────────────────────────── */
static void rm_last_char(void) {
    if (s_input_len <= 0) return;
    int i = s_input_len - 1;
    while (i > 0 && ((unsigned char)s_input[i] & 0xC0) == 0x80) i--;
    s_input_len = i;
    s_input[i]  = '\0';
}

/* ── 화면 지우기 (뷰포트 기준) ───────────────────────── */
static void clear_viewport(void) {
    int rows, cols;
    get_size(&rows, &cols);
    int vtop, vleft;
    get_viewport(&vtop, &vleft);
    COORD origin = { (SHORT)vleft, (SHORT)vtop };
    DWORD w;
    FillConsoleOutputCharacterA(s_hout, ' ', rows * cols, origin, &w);
    FillConsoleOutputAttribute(s_hout,
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
        rows * cols, origin, &w);
    win_move(1, 1);
}

/* ════════════════════════════════════════════════════════
 * 공개 API
 * ════════════════════════════════════════════════════════ */

void tui_init(void) {
    hout_init();
    DWORD mode = 0;
    GetConsoleMode(s_hout, &mode);
    SetConsoleMode(s_hout, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

void tui_enter(const char *room_name) {
    hout_init();
    WaitForSingleObject(g_console_mutex, INFINITE);
    strncpy(s_room_name, room_name ? room_name : "", 63);
    s_room_name[63] = '\0';
    s_input[0]  = '\0';
    s_input_len = 0;
    s_head      = 0;
    s_count     = 0;
    g_tui_active = 1;
    clear_viewport();
    ReleaseMutex(g_console_mutex);
}

void tui_exit(void) {
    WaitForSingleObject(g_console_mutex, INFINITE);
    g_tui_active = 0;
    clear_viewport();
    ReleaseMutex(g_console_mutex);
}

/* g_console_mutex 보유 상태에서 호출 */
void tui_puts(const char *line) {
    if (g_tui_active) {
        strncpy(s_msgs[s_head], line, TUI_MSG_WIDTH - 1);
        s_msgs[s_head][TUI_MSG_WIDTH - 1] = '\0';
        s_head  = (s_head + 1) % TUI_MSG_ROWS;
        if (s_count < TUI_MSG_ROWS) s_count++;
        do_redraw();
    } else {
        printf("%s\n", line);
        fflush(stdout);
    }
}

void tui_printf(const char *fmt, ...) {
    char buf[TUI_MSG_WIDTH];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, TUI_MSG_WIDTH - 1, fmt, ap);
    va_end(ap);
    buf[TUI_MSG_WIDTH - 1] = '\0';
    int n = (int)strlen(buf);
    while (n > 0 && (buf[n-1] == '\n' || buf[n-1] == '\r')) buf[--n] = '\0';

    WaitForSingleObject(g_console_mutex, INFINITE);
    tui_puts(buf);
    ReleaseMutex(g_console_mutex);
}

int tui_readline(char *buf, int max_bytes) {
    /* 시작 시 강제 재그리기 — 이전 처리 중 도착한 메시지 즉시 표시 */
    WaitForSingleObject(g_console_mutex, INFINITE);
    if (g_tui_active) do_redraw();
    ReleaseMutex(g_console_mutex);

    for (;;) {
        /* 50ms 단위 폴링 — 연결 끊김·강퇴 감지 */
        while (!_kbhit()) {
            if (!g_state.connected) return -1;
            if (g_state.current_room_id == 0 &&
                g_state.current_dm_partner[0] == '\0') return -1;
            Sleep(50);
        }

        wint_t wc = _getwch();

        /* 특수키 — 두 번째 코드 소비 후 무시 */
        if (wc == 0x0000 || wc == 0x00E0) {
            if (_kbhit()) _getwch();
            continue;
        }

        /* Enter (CR) */
        if (wc == L'\r') {
            /* Windows CR+LF: 뒤따라오는 LF 소비 */
            Sleep(5);
            if (_kbhit()) {
                wint_t lf = _getwch();
                if (lf != L'\n') {
                    /* LF가 아닌 다른 문자 — 처리할 방법 없어 버림 */
                    (void)lf;
                }
            }
            WaitForSingleObject(g_console_mutex, INFINITE);
            strncpy(buf, s_input, max_bytes - 1);
            buf[max_bytes - 1] = '\0';
            int ret = s_input_len;
            s_input[0]  = '\0';
            s_input_len = 0;
            redraw_input();
            ReleaseMutex(g_console_mutex);
            return ret;
        }

        /* LF 단독 — 무시 (CR+LF 잔여분) */
        if (wc == L'\n') continue;

        /* Backspace */
        if (wc == L'\b') {
            WaitForSingleObject(g_console_mutex, INFINITE);
            rm_last_char();
            redraw_input();
            ReleaseMutex(g_console_mutex);
            continue;
        }

        /* 기타 제어 문자 무시 */
        if (wc < 32) continue;

        /* 일반 문자: wide char → UTF-8 변환 후 입력 버퍼 추가 */
        wchar_t ws[2] = { (wchar_t)wc, 0 };
        char mb[5]    = {0};
        int mb_len = WideCharToMultiByte(CP_UTF8, 0, ws, 1, mb, 4, NULL, NULL);
        if (mb_len <= 0) continue;

        WaitForSingleObject(g_console_mutex, INFINITE);
        if (s_input_len + mb_len < max_bytes - 1) {
            memcpy(s_input + s_input_len, mb, mb_len);
            s_input_len += mb_len;
            s_input[s_input_len] = '\0';
            redraw_input();
        }
        ReleaseMutex(g_console_mutex);
    }
}
