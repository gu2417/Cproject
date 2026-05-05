#ifndef NET_COMPAT_H
#define NET_COMPAT_H

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef int socklen_t;
    #define SHUT_RDWR SD_BOTH
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define SOCKET int
    #define INVALID_SOCKET (-1)
    #define SOCKET_ERROR (-1)
    #define closesocket(s) close(s)
#endif

/* Socket compatibility functions */
int sock_init(void);
void sock_cleanup(void);
int sock_close(SOCKET s);
int sock_set_nonblock(SOCKET s);

#endif // NET_COMPAT_H
