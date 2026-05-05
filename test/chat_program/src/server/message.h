#ifndef MESSAGE_H
#define MESSAGE_H

void handle_message_send(int socket_fd, const char *packet);
void handle_message_delete(int socket_fd, const char *packet);
void handle_message_edit(int socket_fd, const char *packet);
void handle_message_reply(int socket_fd, const char *packet);
void handle_message_search(int socket_fd, const char *packet);
void handle_whisper(int socket_fd, const char *packet);

#endif // MESSAGE_H
