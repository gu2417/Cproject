#ifndef ROUTER_H
#define ROUTER_H

void router_init(void);
void router_dispatch(int socket_fd, const char *packet);

#endif // ROUTER_H
