#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

typedef struct
{
    int socket_fd;
} ClientThreadArg;

#ifdef _WIN32
    void client_thread(void *arg);
#else
    void *client_thread(void *arg);
#endif

int read_line_packet(int socket_fd, char *buffer, int max_size);

#endif // CLIENT_HANDLER_H
