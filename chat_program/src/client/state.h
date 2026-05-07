#pragma once

#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "../common/types.h"

extern ClientState g_state;
extern HANDLE      g_console_mutex;
extern int         g_last_code;     /* 마지막 서버 응답 코드 */
extern int         g_last_room_id;  /* ROOM_CREATE_RES 반환 방 ID */

/* 서버 응답 대기 (response_received가 1이 될 때까지, timeout_ms 단위: ms) */
void wait_response(int timeout_ms);
