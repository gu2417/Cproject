#include <winsock2.h>
#include <windows.h>
#include "state.h"

ClientState g_state         = {0};
HANDLE      g_console_mutex = NULL;
int         g_last_code     = -1;
int         g_last_room_id  = 0;

/* 서버 응답이 오거나 시간이 끝날 때까지 잠시 기다린다. */
void wait_response(int timeout_ms) {
    int elapsed = 0;
    while (!g_state.response_received && elapsed < timeout_ms) {
        Sleep(100);
        elapsed += 100;
    }
}
