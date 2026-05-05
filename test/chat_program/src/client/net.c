#include "net.h"
#include "../common/net_compat.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif

int net_connect(const char *host, int port)
{
    if (!host || port <= 0) {
        return -1;
    }

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host);

    if (connect(socket_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        sock_close(socket_fd);
        return -1;
    }

    return socket_fd;
}

int net_send(int socket_fd, const char *packet)
{
    if (socket_fd < 0 || !packet) {
        return -1;
    }

    int len = strlen(packet);
    int ret = send(socket_fd, packet, len, 0);
    if (ret > 0) {
        send(socket_fd, "\n", 1, 0);
    }

    return ret;
}

int net_recv(int socket_fd, char *buffer, int max_size)
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

void net_close(int socket_fd)
{
    if (socket_fd >= 0) {
        sock_close(socket_fd);
    }
}
