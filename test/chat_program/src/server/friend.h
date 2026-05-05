#ifndef FRIEND_H
#define FRIEND_H

void handle_friend_add(int socket_fd, const char *packet);
void handle_friend_accept(int socket_fd, const char *packet);
void handle_friend_reject(int socket_fd, const char *packet);
void handle_friend_block(int socket_fd, const char *packet);
void handle_friend_list(int socket_fd, const char *packet);
int friend_is_blocked(const char *user_a, const char *user_b);

#endif // FRIEND_H
