#include "globals.h"
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
    #define MUTEX_LOCK(m)   WaitForSingleObject(m, INFINITE)
    #define MUTEX_UNLOCK(m) ReleaseMutex(m)
    #define MUTEX_INIT(m)   (m = CreateMutex(NULL, FALSE, NULL))
#else
    #define MUTEX_LOCK(m)   pthread_mutex_lock(&m)
    #define MUTEX_UNLOCK(m) pthread_mutex_unlock(&m)
    #define MUTEX_INIT(m)   pthread_mutex_init(&m, NULL)
#endif

User g_sessions[100];
int g_session_count = 0;
pthread_mutex_t g_sessions_mutex;

int g_active_rooms = 0;
pthread_mutex_t g_rooms_mutex;

void globals_init(void)
{
    MUTEX_INIT(g_sessions_mutex);
    MUTEX_INIT(g_rooms_mutex);
}

int sessions_add(const User *user)
{
    if (!user || g_session_count >= 100) {
        return -1;
    }

    MUTEX_LOCK(g_sessions_mutex);
    memcpy(&g_sessions[g_session_count], user, sizeof(User));
    g_session_count++;
    MUTEX_UNLOCK(g_sessions_mutex);

    return 0;
}

int sessions_remove(const char *user_id)
{
    if (!user_id) {
        return -1;
    }

    MUTEX_LOCK(g_sessions_mutex);
    for (int i = 0; i < g_session_count; i++) {
        if (strcmp(g_sessions[i].user_id, user_id) == 0) {
            memmove(&g_sessions[i], &g_sessions[i + 1],
                    (g_session_count - i - 1) * sizeof(User));
            g_session_count--;
            MUTEX_UNLOCK(g_sessions_mutex);
            return 0;
        }
    }
    MUTEX_UNLOCK(g_sessions_mutex);

    return -1;
}

User *sessions_find_by_id(const char *user_id)
{
    if (!user_id) {
        return NULL;
    }

    MUTEX_LOCK(g_sessions_mutex);
    for (int i = 0; i < g_session_count; i++) {
        if (strcmp(g_sessions[i].user_id, user_id) == 0) {
            MUTEX_UNLOCK(g_sessions_mutex);
            return &g_sessions[i];
        }
    }
    MUTEX_UNLOCK(g_sessions_mutex);

    return NULL;
}

User *sessions_find_by_fd(int socket_fd)
{
    if (socket_fd < 0) {
        return NULL;
    }

    MUTEX_LOCK(g_sessions_mutex);
    for (int i = 0; i < g_session_count; i++) {
        if (g_sessions[i].socket_fd == socket_fd) {
            MUTEX_UNLOCK(g_sessions_mutex);
            return &g_sessions[i];
        }
    }
    MUTEX_UNLOCK(g_sessions_mutex);

    return NULL;
}

int sessions_get_room_members(int room_id, int *fds, int max_fds)
{
    (void)room_id;

    if (!fds || max_fds <= 0) {
        return 0;
    }

    MUTEX_LOCK(g_sessions_mutex);
    int count = 0;
    for (int i = 0; i < g_session_count && count < max_fds; i++) {
        fds[count++] = g_sessions[i].socket_fd;
    }
    MUTEX_UNLOCK(g_sessions_mutex);

    return count;
}

int sessions_remove_by_fd(int socket_fd)
{
    if (socket_fd < 0) {
        return -1;
    }

    MUTEX_LOCK(g_sessions_mutex);
    for (int i = 0; i < g_session_count; i++) {
        if (g_sessions[i].socket_fd == socket_fd) {
            memmove(&g_sessions[i], &g_sessions[i + 1],
                    (g_session_count - i - 1) * sizeof(User));
            g_session_count--;
            MUTEX_UNLOCK(g_sessions_mutex);
            return 0;
        }
    }
    MUTEX_UNLOCK(g_sessions_mutex);

    return -1;
}
