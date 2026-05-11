#pragma once

#include <winsock2.h>
#include <windows.h>
#include "../common/types.h"

/* 현재 접속한 클라이언트 세션 목록이다. */
extern ClientSession g_sessions[MAX_CLIENTS];
extern int           g_session_count;

/* 서버가 메모리에 들고 있는 채팅방 목록이다. */
extern RoomInfo      g_rooms[MAX_ROOMS];
extern int           g_room_count;

/* 파일에서 읽어 온 사용자와 친구 정보이다. */
extern UserRecord   g_users[MAX_USERS];
extern int          g_user_count;
extern FriendRecord g_friends[MAX_FRIENDS];
extern int          g_friend_count;

/* 메시지와 방 멤버 기록이다. */
extern MessageRecord      g_messages[MAX_MSG_HISTORY];
extern int                g_msg_count;
extern RoomMemberRecord   g_room_members[MAX_ROOM_MEMBER_RECORDS];
extern int                g_room_member_count;

/* 읽음 처리와 초대, 설정 정보이다. */
extern DmReadRecord       g_dm_reads[MAX_DM_READS];
extern int                g_dm_read_count;
extern RoomInviteRecord   g_room_invites[MAX_INVITES];
extern int                g_room_invite_count;
extern UserSettingsRecord g_user_settings[MAX_USERS];
extern int                g_user_settings_count;
extern RoomReadRecord     g_room_reads[MAX_ROOM_READS];
extern int                g_room_read_count;

/* 새 자료를 만들 때 사용할 다음 번호이다. */
extern int g_next_user_id;
extern int g_next_room_id;
extern int g_next_msg_id;
extern int g_next_friend_id;
extern int g_next_invite_id;

/* 여러 스레드가 같이 쓰는 자료를 보호하는 뮤텍스이다. */
extern HANDLE g_sessions_mutex;  /* g_sessions, g_rooms, g_session_count */
extern HANDLE g_file_mutex;      /* 파일 입출력과 파일 캐시 */
extern HANDLE g_console_mutex;   /* 콘솔 출력 */

/* 파일을 다 읽은 뒤 다음에 쓸 번호들을 맞춘다. */
void restore_next_ids(void);

/* 아이디로 사용자 정보를 찾는다. */
UserRecord    *find_user_by_id(const char *user_id);

/* 아이디로 현재 접속 세션을 찾는다. */
ClientSession *find_session_by_id(const char *user_id);

/* 방 번호로 g_rooms 배열의 위치를 찾는다. */
int            find_room_idx(int room_id);

/* 사용자가 보낸 메시지 수를 센다. */
int count_user_messages(const char *user_id);

/* 사용자가 들어가 있는 방 수를 센다. */
int count_user_rooms(const char *user_id);

/* 사용자의 친구 수를 센다. */
int count_user_friends(const char *user_id);
