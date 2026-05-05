#ifndef NET_H
#define NET_H

int net_connect(const char *host, int port);
int net_send(int socket_fd, const char *packet);
int net_recv(int socket_fd, char *buffer, int max_size);
void net_close(int socket_fd);

#endif // NET_H
