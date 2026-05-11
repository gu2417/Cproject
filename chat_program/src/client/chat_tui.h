#pragma once
#include <winsock2.h>
#include <windows.h>

#define TUI_MSG_ROWS  500
#define TUI_MSG_WIDTH 512

/* 채팅 화면이 켜져 있는지 저장하는 값이다. */
extern int g_tui_active;

/* 콘솔에서 ANSI 색상 처리를 쓸 수 있게 준비한다. */
void tui_init(void);

/* 설정된 테마 색을 콘솔에 적용한다. */
void tui_apply_theme(void);

/* 채팅방 화면에 들어간다. */
void tui_enter(const char *room_name);

/* 채팅방 화면에서 나온다. */
void tui_exit(void);

/* 채팅방 상단 공지 문구를 바꾼다. */
void tui_set_notice(const char *notice);

/* 채팅 화면 메시지 영역에 한 줄을 추가한다. */
void tui_puts(const char *line);

/* printf처럼 문자열을 만들어 채팅 화면에 출력한다. */
void tui_printf(const char *fmt, ...);

/* 채팅 화면 아래 입력줄에서 한 줄을 입력받는다. */
int tui_readline(char *buf, int max_bytes);
