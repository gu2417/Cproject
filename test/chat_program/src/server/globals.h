#ifndef GLOBALS_H
#define GLOBALS_H

#include "../common/types.h"

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <windows.h>
    typedef HANDLE pthread_mutex_t;
#else
    #include <pthread.h>
#endif

/* Global session storage */
extern User g_sessions[100];
extern int g_session_count;
extern pthread_mutex_t g_sessions_mutex;

extern int g_active_rooms;
extern pthread_mutex_t g_rooms_mutex;

/* Session management functions */
int sessions_add(const User *user);
int sessions_remove(const char *user_id);
int sessions_remove_by_fd(int socket_fd);
User *sessions_find_by_id(const char *user_id);
User *sessions_find_by_fd(int socket_fd);
int sessions_get_room_members(int room_id, int *fds, int max_fds);
void globals_init(void);

#endif // GLOBALS_H
