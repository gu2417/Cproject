#ifndef ROOM_H
#define ROOM_H

void handle_room_create(int socket_fd, const char *packet);
void handle_room_join(int socket_fd, const char *packet);
void handle_room_leave(int socket_fd, const char *packet);
void handle_room_invite(int socket_fd, const char *packet);
void handle_room_kick(int socket_fd, const char *packet);
void handle_room_set_notice(int socket_fd, const char *packet);
void handle_room_grant_admin(int socket_fd, const char *packet);

#endif // ROOM_H
