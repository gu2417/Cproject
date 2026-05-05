#ifndef _WIN32
    #error "This file should not be compiled on non-Windows"
#endif

#include "net_compat.h"

int sock_init(void)
{
    WSADATA wsa_data;
    int ret = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (ret != 0) {
        return SOCKET_ERROR;
    }
    return 0;
}

void sock_cleanup(void)
{
    WSACleanup();
}

int sock_close(SOCKET s)
{
    if (s == INVALID_SOCKET) {
        return SOCKET_ERROR;
    }
    return closesocket(s);
}

int sock_set_nonblock(SOCKET s)
{
    if (s == INVALID_SOCKET) {
        return SOCKET_ERROR;
    }

    unsigned long mode = 1;
    return ioctlsocket(s, FIONBIO, &mode);
}
