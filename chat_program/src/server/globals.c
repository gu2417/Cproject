#include <winsock2.h>
#include <windows.h>
#include <string.h>
#include "globals.h"

/* === 세션·방 === */
ClientSession g_sessions[MAX_CLIENTS];
int           g_session_count = 0;
RoomInfo      g_rooms[MAX_ROOMS];
int           g_room_count    = 0;

/* === 파일 영속 캐시 === */
UserRecord   g_users[MAX_USERS];
int          g_user_count   = 0;
FriendRecord g_friends[MAX_FRIENDS];
int          g_friend_count = 0;
MessageRecord      g_messages[MAX_MSG_HISTORY];
int                g_msg_count           = 0;
RoomMemberRecord   g_room_members[MAX_ROOM_MEMBER_RECORDS];
int                g_room_member_count   = 0;
DmReadRecord       g_dm_reads[MAX_DM_READS];
int                g_dm_read_count       = 0;
RoomInviteRecord   g_room_invites[MAX_INVITES];
int                g_room_invite_count   = 0;
UserSettingsRecord g_user_settings[MAX_USERS];
int                g_user_settings_count = 0;
RoomReadRecord     g_room_reads[MAX_ROOM_READS];
int                g_room_read_count     = 0;

/* === 단조 증가 ID === */
int g_next_user_id   = 1;
int g_next_room_id   = 1;
int g_next_msg_id    = 1;
int g_next_friend_id = 1;
int g_next_invite_id = 1;

/* === Mutex 핸들 === */
HANDLE g_sessions_mutex = NULL;
HANDLE g_file_mutex     = NULL;
HANDLE g_console_mutex  = NULL;

/* ---------------------------------------------------------------
 * 파일 로드 후 각 ID 카운터를 max+1 로 복원한다.
 * load_users(), load_rooms(), load_room_members() 호출 후 실행.
 * load_messages() 가 g_next_msg_id 를 직접 갱신하므로 여기서는 제외.
 * --------------------------------------------------------------- */
void restore_next_ids(void) {
    int i;

    /* user: g_users[].id 는 로드 시 순번으로 채워짐 */
    g_next_user_id = g_user_count + 1;

    /* room */
    g_next_room_id = 1;
    for (i = 0; i < g_room_count; i++) {
        if (g_rooms[i].info.id >= g_next_room_id)
            g_next_room_id = g_rooms[i].info.id + 1;
    }

    /* friend */
    g_next_friend_id = 1;
    for (i = 0; i < g_friend_count; i++) {
        if (g_friends[i].id >= g_next_friend_id)
            g_next_friend_id = g_friends[i].id + 1;
    }

    /* invite: g_room_invites 에서 최대값 복원 */
    g_next_invite_id = 1;
    for (i = 0; i < g_room_invite_count; i++) {
        if (g_room_invites[i].id >= g_next_invite_id)
            g_next_invite_id = g_room_invites[i].id + 1;
    }
}

UserRecord *find_user_by_id(const char *user_id) {
    int i;
    for (i = 0; i < g_user_count; i++) {
        if (strcmp(g_users[i].id_str, user_id) == 0)
            return &g_users[i];
    }
    return NULL;
}

/* MUTEX: 호출자가 g_sessions_mutex 를 보유해야 한다. */
ClientSession *find_session_by_id(const char *user_id) {
    int i;
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (g_sessions[i].active && strcmp(g_sessions[i].user_id, user_id) == 0)
            return &g_sessions[i];
    }
    return NULL;
}

/* MUTEX: 호출자가 g_sessions_mutex 를 보유해야 한다. */
int find_room_idx(int room_id) {
    int i;
    for (i = 0; i < g_room_count; i++) {
        if (g_rooms[i].info.id == room_id && !g_rooms[i].info.is_deleted)
            return i;
    }
    return -1;
}

int count_user_messages(const char *user_id) {
    int i, n = 0;
    for (i = 0; i < g_msg_count; i++) {
        if (!g_messages[i].is_deleted &&
            strcmp(g_messages[i].from_id, user_id) == 0)
            n++;
    }
    return n;
}

int count_user_rooms(const char *user_id) {
    int i, j, n = 0;
    for (i = 0; i < g_room_member_count; i++) {
        if (strcmp(g_room_members[i].user_id, user_id) != 0) continue;
        for (j = 0; j < g_room_count; j++) {
            if (g_rooms[j].info.id == g_room_members[i].room_id &&
                !g_rooms[j].info.is_deleted) {
                n++;
                break;
            }
        }
    }
    return n;
}

int count_user_friends(const char *user_id) {
    int i, n = 0;
    for (i = 0; i < g_friend_count; i++) {
        if (g_friends[i].status == FRIEND_ACCEPTED &&
            (strcmp(g_friends[i].user_id,   user_id) == 0 ||
             strcmp(g_friends[i].friend_id, user_id) == 0))
            n++;
    }
    return n;
}
