#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "../common/protocol.h"
#include "state.h"
#include "net.h"
#include "packet.h"

/* 입력한 IP와 포트로 서버에 접속한다. */
SOCKET connect_to_server(const char *ip, int port) {
    SOCKADDR_IN addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port        = htons((u_short)port);

    SOCKET sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "[net] socket() 실패: %d\n", WSAGetLastError());
        exit(1);
    }
    if (connect(sock, (SOCKADDR *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        fprintf(stderr, "[net] 서버 연결 실패 (%s:%d): %d\n",
                ip, port, WSAGetLastError());
        closesocket(sock);
        exit(1);
    }
    return sock;
}

/* 서버로 보낼 패킷 문자열을 만들어 전송한다. */
void send_packet(SOCKET sock, const char *fmt, ...) {
    char    buf[MAX_PKT_SIZE];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, (int)sizeof(buf) - 2, fmt, ap);
    va_end(ap);
    if (n < 0) return;
    /* '\n' 보장 */
    if (n == 0 || buf[n - 1] != '\n') {
        buf[n]     = '\n';
        buf[n + 1] = '\0';
        n++;
    }
    send(sock, buf, n, 0);
}

/* 서버에서 오는 패킷을 계속 받아 처리한다. */
unsigned WINAPI RecvMsg(void *arg) {
    SOCKET sock = *((SOCKET *)arg);
    char   buf[MAX_BUF_SIZE];
    int    len;

    while (1) {
        len = recv(sock, buf, (int)sizeof(buf) - 1, 0);
        if (len <= 0) {
            WaitForSingleObject(g_console_mutex, INFINITE);
            printf("\n[서버 연결이 끊어졌습니다.]\n");
            ReleaseMutex(g_console_mutex);
            g_state.connected        = 0;
            g_state.response_received = 1;  /* 대기 중인 메뉴 깨우기 */
            break;
        }
        buf[len] = '\0';

        /* 한 recv에 여러 줄(\n 구분)이 올 수 있으므로 줄 단위 처리 */
        char *line = buf;
        char *nl;
        while ((nl = strchr(line, '\n')) != NULL) {
            *nl = '\0';
            if (line[0] != '\0')
                packet_parse(line, sock);
            line = nl + 1;
        }
        if (line[0] != '\0')
            packet_parse(line, sock);
    }
    return 0;
}
