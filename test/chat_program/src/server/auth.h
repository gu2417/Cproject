#ifndef AUTH_H
#define AUTH_H

void handle_auth_req(int socket_fd, const char *packet);
void handle_signup_req(int socket_fd, const char *packet);
void handle_logout(int socket_fd, const char *packet);
void handle_pass_change(int socket_fd, const char *packet);

#endif // AUTH_H
