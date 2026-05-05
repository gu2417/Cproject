#ifndef BROADCAST_H
#define BROADCAST_H

/* Broadcast functions */
void bcast_room(int room_id, const char *packet, int exclude_fd);
void bcast_all(const char *packet, int exclude_fd);
void send_packet_to_user(const char *user_id, const char *packet);

#endif // BROADCAST_H
