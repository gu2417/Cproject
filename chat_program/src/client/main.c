#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <process.h>
#include "../common/protocol.h"
#include "state.h"
#include "net.h"
#include "chat_tui.h"
#include "menu_initial.h"

int main(void) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    tui_init();

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "[main] WSAStartup 실패: %d\n", WSAGetLastError());
        return 1;
    }

    /* 콘솔 뮤텍스 초기화 */
    g_console_mutex = CreateMutex(NULL, FALSE, NULL);
    if (!g_console_mutex) {
        fprintf(stderr, "[main] 뮤텍스 생성 실패\n");
        WSACleanup();
        return 1;
    }

    /* 서버 연결 */
    g_state.sock = connect_to_server("127.0.0.1", DEFAULT_PORT);
    g_state.connected = 1;

    /* 수신 스레드 시작 */
    HANDLE hRecv = (HANDLE)_beginthreadex(NULL, 0, RecvMsg,
                                           &g_state.sock, 0, NULL);
    if (!hRecv) {
        fprintf(stderr, "[main] RecvMsg 스레드 생성 실패\n");
        closesocket(g_state.sock);
        CloseHandle(g_console_mutex);
        WSACleanup();
        return 1;
    }

    /* 메인 메뉴 루프 */
    InitialMenu();

    /* 정리 */
    closesocket(g_state.sock);
    WaitForSingleObject(hRecv, 2000);
    CloseHandle(hRecv);
    CloseHandle(g_console_mutex);
    WSACleanup();
    return 0;
}
