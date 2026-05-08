#pragma once
#include <winsock2.h>
#include <windows.h>

#define TUI_MSG_ROWS  500
#define TUI_MSG_WIDTH 512

/* 1 = 채팅방 TUI 활성 상태 */
extern int g_tui_active;

/* 시작 시 1회 — ANSI VT 처리 활성화 */
void tui_init(void);

/* 채팅방 입장/퇴장 */
void tui_enter(const char *room_name);
void tui_exit(void);

/* 메시지 영역에 한 줄 추가 (g_console_mutex 보유 상태에서 호출) */
void tui_puts(const char *line);

/* 포맷팅 후 표시 — 뮤텍스 내부 처리, TUI/비-TUI 모두 동작 */
void tui_printf(const char *fmt, ...);

/* 블로킹 라인 입력. 반환값: 바이트 수, -1 = 연결 끊김/강퇴 */
int tui_readline(char *buf, int max_bytes);
