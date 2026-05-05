#include "client_handler.h"
#include "router.h"
#include "globals.h"
#include "../common/protocol.h"
#include "../common/net_compat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <process.h>
#else
    #include <unistd.h>
    #include <pthread.h>
#endif

int read_line_packet(int socket_fd, char *buffer, int max_size)
{
    if (socket_fd < 0 || !buffer || max_size <= 0) {
        return -1;
    }

    int pos = 0;
    while (pos < max_size - 1) {
        int ret = recv(socket_fd, &buffer[pos], 1, 0);
        if (ret <= 0) {
            return -1;
        }

        if (buffer[pos] == '\n') {
            buffer[pos] = '\0';
            return pos;
        }

        pos++;
    }

    return -1;
}

#ifdef _WIN32
void client_thread(void *arg)
{
    if (!arg) {
        _endthread();
        return;
    }

    ClientThreadArg *carg = (ClientThreadArg *)arg;
    int socket_fd = carg->socket_fd;
    free(carg);

    char buffer[MAX_PACKET_SIZE];

    while (1) {
        int ret = read_line_packet(socket_fd, buffer, sizeof(buffer));
        if (ret <= 0) {
            break;
        }

        router_dispatch(socket_fd, buffer);
    }

    sock_close(socket_fd);
    sessions_remove_by_fd(socket_fd);

    _endthread();
}
#else
void *client_thread(void *arg)
{
    if (!arg) {
        return NULL;
    }

    ClientThreadArg *carg = (ClientThreadArg *)arg;
    int socket_fd = carg->socket_fd;
    free(carg);

    char buffer[MAX_PACKET_SIZE];

    while (1) {
        int ret = read_line_packet(socket_fd, buffer, sizeof(buffer));
        if (ret <= 0) {
            break;
        }

        router_dispatch(socket_fd, buffer);
    }

    sock_close(socket_fd);
    sessions_remove_by_fd(socket_fd);

    return NULL;
}
#endif
