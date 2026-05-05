#include "broadcast.h"
#include "globals.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <windows.h>
    #define SEND_FLAGS 0
    #define MUTEX_LOCK(m)   WaitForSingleObject(m, INFINITE)
    #define MUTEX_UNLOCK(m) ReleaseMutex(m)
#else
    #include <unistd.h>
    #include <pthread.h>
    #define SEND_FLAGS MSG_NOSIGNAL
    #define MUTEX_LOCK(m)   pthread_mutex_lock(&m)
    #define MUTEX_UNLOCK(m) pthread_mutex_unlock(&m)
#endif

void bcast_room(int room_id, const char *packet, int exclude_fd)
{
    if (!packet) {
        return;
    }

    int fds[100];
    int count = sessions_get_room_members(room_id, fds, 100);

    for (int i = 0; i < count; i++) {
        if (fds[i] != exclude_fd && fds[i] > 0) {
            send(fds[i], packet, strlen(packet), SEND_FLAGS);
            send(fds[i], "\n", 1, SEND_FLAGS);
        }
    }
}

void bcast_all(const char *packet, int exclude_fd)
{
    if (!packet) {
        return;
    }

    MUTEX_LOCK(g_sessions_mutex);
    for (int i = 0; i < g_session_count; i++) {
        if (g_sessions[i].socket_fd != exclude_fd && g_sessions[i].socket_fd > 0) {
            send(g_sessions[i].socket_fd, packet, strlen(packet), SEND_FLAGS);
            send(g_sessions[i].socket_fd, "\n", 1, SEND_FLAGS);
        }
    }
    MUTEX_UNLOCK(g_sessions_mutex);
}

void send_packet_to_user(const char *user_id, const char *packet)
{
    if (!user_id || !packet) {
        return;
    }

    User *user = sessions_find_by_id(user_id);
    if (user && user->socket_fd > 0) {
        send(user->socket_fd, packet, strlen(packet), SEND_FLAGS);
        send(user->socket_fd, "\n", 1, SEND_FLAGS);
    }
}
