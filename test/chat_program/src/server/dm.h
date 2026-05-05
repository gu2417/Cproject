#ifndef DM_H
#define DM_H

void handle_dm_send(int socket_fd, const char *packet);
void handle_dm_history(int socket_fd, const char *packet);
void mark_dm_read(const char *msg_id, const char *reader_id);

#endif // DM_H
