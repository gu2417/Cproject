#ifdef _WIN32
    #error "This file should not be compiled on Windows"
#endif

#include "net_compat.h"
#include <fcntl.h>

int sock_init(void)
{
    return 0;
}

void sock_cleanup(void)
{
}

int sock_close(SOCKET s)
{
    if (s == INVALID_SOCKET) {
        return SOCKET_ERROR;
    }
    return close(s);
}

int sock_set_nonblock(SOCKET s)
{
    if (s == INVALID_SOCKET) {
        return SOCKET_ERROR;
    }

    int flags = fcntl(s, F_GETFL, 0);
    if (flags == -1) {
        return SOCKET_ERROR;
    }

    return fcntl(s, F_SETFL, flags | O_NONBLOCK);
}
