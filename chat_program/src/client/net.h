#pragma once

#include <winsock2.h>
#include <windows.h>

/* 지정한 IP와 포트로 서버에 접속한다. */
SOCKET   connect_to_server(const char *ip, int port);

/* 서버에 정해진 형식의 패킷을 보낸다. */
void     send_packet(SOCKET sock, const char *fmt, ...);

/* 서버에서 오는 메시지를 계속 받아서 처리한다. */
unsigned WINAPI RecvMsg(void *arg);
