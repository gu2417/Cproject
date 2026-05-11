#pragma once

#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "../common/types.h"

/* 클라이언트가 실행 중에 들고 있는 현재 상태이다. */
extern ClientState g_state;

/* 수신 스레드와 메뉴 출력이 동시에 콘솔을 쓰지 않게 막는다. */
extern HANDLE      g_console_mutex;

/* 마지막으로 받은 서버 응답 코드이다. */
extern int         g_last_code;

/* 방 생성 뒤 서버가 알려 준 방 번호이다. */
extern int         g_last_room_id;

/* 서버 응답이 올 때까지 잠깐 기다린다. */
void wait_response(int timeout_ms);
