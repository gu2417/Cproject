#pragma once

#include <winsock2.h>
#include <windows.h>

SOCKET   connect_to_server(const char *ip, int port);
void     send_packet(SOCKET sock, const char *fmt, ...);
unsigned WINAPI RecvMsg(void *arg);
