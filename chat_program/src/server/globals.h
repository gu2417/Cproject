#pragma once

#include <winsock2.h>
#include <windows.h>
#include "../common/types.h"

/* === 세션·방 (mutex: g_sessions_mutex) === */
extern ClientSession g_sessions[MAX_CLIENTS];
extern int           g_session_count;
extern RoomInfo      g_rooms[MAX_ROOMS];
extern int           g_room_count;

/* === 파일 영속 캐시 (mutex: g_file_mutex 쓰기 시) === */
extern UserRecord   g_users[MAX_CLIENTS];   /* 동시 접속 한도 내 유저 캐시 (P0) */
extern int          g_user_count;
extern FriendRecord g_friends[MAX_CLIENTS * 4];
extern int          g_friend_count;

/* === 단조 증가 ID (파일 로드 후 max+1 복원) === */
extern int g_next_user_id;
extern int g_next_room_id;
extern int g_next_msg_id;
extern int g_next_friend_id;
extern int g_next_invite_id;

/* === Mutex 핸들 === */
extern HANDLE g_sessions_mutex;  /* g_sessions, g_rooms, g_session_count */
extern HANDLE g_file_mutex;      /* 모든 fopen/fprintf/fgets 호출 */
extern HANDLE g_console_mutex;   /* RecvMsg ↔ 메인 스레드 콘솔 출력 */

/* 파일 로드 후 ID 카운터를 max+1 로 복원한다. */
void restore_next_ids(void);

/* g_users 에서 id_str 검색. 없으면 NULL. */
UserRecord    *find_user_by_id(const char *user_id);

/* g_sessions 에서 user_id 검색. 없으면 NULL.
 * MUTEX: 호출자가 g_sessions_mutex 를 보유해야 한다. */
ClientSession *find_session_by_id(const char *user_id);

/* g_rooms 에서 room_id 검색, 인덱스 반환. 없으면 -1.
 * MUTEX: 호출자가 g_sessions_mutex 를 보유해야 한다. */
int            find_room_idx(int room_id);
