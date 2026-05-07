# 인메모리 구조체

## 1. 전역 상수

```c
/* config.h */
#define MAX_CLIENTS              256   /* 최대 동시 접속 세션 (NFR-01) */
#define MAX_ROOMS                100   /* 최대 채팅방 수 */
#define MAX_ROOM_MEMBERS          64   /* 방당 최대 멤버 수 (FR-G01) */
#define MAX_USERS               1000   /* 가입 유저 인메모리 캐시 한도 */
#define MAX_FRIENDS             5000   /* 친구 관계 인메모리 한도 (1000명 × 5명 평균) */
#define MAX_MSG_HISTORY         1000   /* 인메모리 최근 메시지 캐시 수 */
#define MAX_ROOM_MEMBER_RECORDS 6400   /* MAX_ROOMS × MAX_ROOM_MEMBERS */
#define MAX_DM_READS           10000   /* DM 읽음 기록 캐시 한도 */
#define MAX_INVITES             1000   /* 오프라인 초대 대기 한도 */
#define MAX_ROOM_READS          6400   /* room_reads 캐시 한도 (MAX_USERS × 평균 참여방) */
#define MAX_BUF_SIZE           10240   /* 최대 패킷 버퍼 크기 (bytes) */
#define DEFAULT_PORT           55555   /* 기본 포트 번호 */

/* 데이터 파일 경로 */
#define FILE_USERS         "data/users.txt"
#define FILE_ROOMS         "data/rooms.txt"
#define FILE_MESSAGES      "data/messages.txt"
#define FILE_FRIENDS       "data/friends.txt"
#define FILE_ROOM_MEMBERS  "data/room_members.txt"
#define FILE_DM_READS      "data/dm_reads.txt"
#define FILE_ROOM_INVITES  "data/room_invites.txt"
#define FILE_USER_SETTINGS "data/user_settings.txt"
#define FILE_ROOM_READS    "data/room_reads.txt"
```

---

## 2. ClientSession — 접속 중인 클라이언트

```c
/* globals.h */
typedef struct {
    SOCKET   fd;               /* 클라이언트 소켓 핸들 */
    char     user_id[21];      /* 로그인 ID (최대 20자 + null) */
    char     nickname[21];     /* 표시 닉네임 */
    int      online_status;    /* 0=offline, 1=online, 2=busy, 3=invisible */
    int      dnd;              /* 0=알림 켜짐, 1=방해금지 모드 */
    int      current_room_id;  /* 현재 입장한 방 ID (0 = 채팅방 밖) */
    int      muted_rooms[32];  /* 알림 무음 room_id 목록 */
    int      muted_count;      /* muted_rooms 유효 항목 수 */
    int      is_admin;         /* 서버 관리자 여부 (0/1) */
    int      active;           /* 1 = 유효한 세션, 0 = 빈 슬롯 */
    HANDLE   hThread;          /* _beginthreadex 스레드 핸들 */
} ClientSession;
```

**사용 패턴**:
```c
/* 슬롯 할당 — accept 후 */
WaitForSingleObject(g_sessions_mutex, INFINITE);
for (int i = 0; i < MAX_CLIENTS; i++) {
    if (!g_sessions[i].active) {
        g_sessions[i].fd     = client_sock;
        g_sessions[i].active = 1;
        break;
    }
}
ReleaseMutex(g_sessions_mutex);

/* 슬롯 해제 — leftClient() */
WaitForSingleObject(g_sessions_mutex, INFINITE);
g_sessions[idx].active        = 0;
g_sessions[idx].fd            = INVALID_SOCKET;
g_sessions[idx].user_id[0]    = '\0';
g_sessions[idx].current_room_id = 0;
ReleaseMutex(g_sessions_mutex);
```

---

## 3. RoomInfo — 활성 채팅방

```c
typedef struct {
    int   id;                              /* 방 고유 ID */
    char  name[31];                        /* 방 이름 (최대 30자) */
    char  topic[101];                      /* 방 주제 (최대 100자) */
    char  password_hash[65];               /* SHA-256 해시 (64자 + null), 빈 문자열 = 공개 */
    int   max_users;                       /* 최대 인원 */
    char  owner_id[21];                    /* 방장 ID */
    char  notice[256];                     /* 공지사항 */
    int   is_open;                         /* 0=그룹채팅, 1=오픈채팅 */
    int   pinned_msg_id;                   /* 핀 메시지 ID (0 = 없음) */
    int   member_count;                    /* 현재 멤버 수 */
    char  member_ids[MAX_ROOM_MEMBERS][21];/* 멤버 ID 배열 */
    int   admin_flags[MAX_ROOM_MEMBERS];   /* 1 = 공동 방장 */
    int   active;                          /* 1 = 유효한 방, 0 = 빈 슬롯 */
} RoomInfo;
```

---

## 4. MessageRecord — 메시지 (인메모리 캐시)

```c
typedef struct {
    int   id;            /* 메시지 고유 ID */
    int   room_id;       /* 채팅방 ID (0 = DM) */
    char  from_id[21];   /* 발신자 ID */
    char  to_id[21];     /* 수신자 ID (DM 전용, 빈 값 = 그룹) */
    char  content[501];  /* 메시지 본문 (최대 500자) */
    int   reply_to;      /* 답장 원본 msg_id (0 = 없음) */
    int   msg_type;      /* 0=normal, 1=system, 2=whisper, 3=me-action */
    int   is_deleted;    /* 0=정상, 1=삭제됨 */
    char  created_at[20];/* YYYY-MM-DD HH:MM:SS */
    char  edited_at[20]; /* YYYY-MM-DD HH:MM:SS, 빈 문자열 = 수정 없음 */
} MessageRecord;
```

---

## 5. 파일 I/O용 레코드 구조체

```c
/* file_io.h에서 선언 */

typedef struct {
    char  id[21];
    char  pw_hash[65];
    char  nickname[21];
    char  status_msg[101];
    int   online_status;
    int   is_admin;
    char  last_seen[20];
    char  created_at[20];
} UserRecord;

typedef struct {
    int   id;
    char  user_id[21];
    char  friend_id[21];
    int   status;          /* 0=pending, 1=accepted, 2=blocked */
    char  created_at[20];
} FriendRecord;

typedef struct {
    int   room_id;
    char  user_id[21];
    char  open_nick[21];
    int   is_admin;
    int   is_muted;
    char  joined_at[20];
} RoomMemberRecord;

typedef struct {
    int   msg_id;
    char  reader_id[21];
    char  read_at[20];
} DmReadRecord;

typedef struct {
    int   id;
    int   room_id;
    char  inviter_id[21];
    char  invitee_id[21];
    int   status;          /* 0=pending, 1=수락, 2=거절 */
    char  created_at[20];
} RoomInviteRecord;

typedef struct {
    char  user_id[21];
    char  msg_color[16];
    char  nick_color[16];
    char  theme[11];
    int   ts_format;       /* 0=HH:MM, 1=HH:MM:SS, 2=MM-DD HH:MM */
    int   dnd;
    int   welcome_shown;   /* 첫 로그인 가이드 표시 여부 (0/1) */
} UserSettingsRecord;

/* 그룹 채팅방 읽음 기록 — FR-D05·FR-G09 안읽음 카운트 산출용 */
typedef struct {
    int   room_id;
    char  user_id[21];
    int   last_read_msg_id;  /* 사용자가 마지막으로 읽은 messages.txt 의 msg_id */
    char  read_at[20];       /* YYYY-MM-DD HH:MM:SS */
} RoomReadRecord;

/* rooms.txt 파일 직렬화용 평탄 구조체 (RoomInfo와 다른 점: member_ids/admin_flags 미포함 — 멤버는 room_members.txt가 source-of-truth) */
typedef struct {
    int   id;
    char  name[31];
    char  topic[101];
    char  pw_hash[65];
    int   max_users;
    char  owner_id[21];
    char  notice[256];
    int   is_open;          /* 0=그룹채팅, 1=오픈채팅 */
    int   pinned_msg_id;
    char  created_at[20];
} RoomRecord;
```

---

## 6. 전역 변수 선언 (globals.h / globals.c)

서버는 모든 영속 데이터를 시작 시 파일에서 로드하여 인메모리 배열로 캐시하고, 변경 시 배열 갱신 + 파일 동기화를 동시에 수행한다. 모든 전역은 `g_` 접두사를 사용하며, 접근은 §7의 mutex 패턴을 따른다.

```c
/* globals.h */
#ifndef GLOBALS_H
#define GLOBALS_H

#include <winsock2.h>
#include <Windows.h>
#include "config.h"

/* === 세션·방 (RAM only, mutex: g_sessions_mutex) === */
extern ClientSession      g_sessions[MAX_CLIENTS];
extern RoomInfo           g_rooms[MAX_ROOMS];          /* in-memory 활성 방 캐시 */
extern int                g_client_count;
extern int                g_room_count;

/* === 파일 영속 캐시 (mutex: g_file_mutex 쓰기 시) === */
extern UserRecord         g_users[MAX_USERS];
extern int                g_user_count;
extern FriendRecord       g_friends[MAX_FRIENDS];
extern int                g_friend_count;
extern MessageRecord      g_messages[MAX_MSG_HISTORY];
extern int                g_msg_count;
extern RoomMemberRecord   g_room_members[MAX_ROOM_MEMBER_RECORDS];
extern int                g_room_member_count;
extern DmReadRecord       g_dm_reads[MAX_DM_READS];
extern int                g_dm_read_count;
extern RoomInviteRecord   g_room_invites[MAX_INVITES];
extern int                g_room_invite_count;
extern UserSettingsRecord g_user_settings[MAX_USERS];
extern int                g_user_settings_count;
extern RoomReadRecord     g_room_reads[MAX_ROOM_READS];
extern int                g_room_read_count;

/* === 단조 증가 ID (파일 로드 후 max+1로 복원) === */
extern int                g_next_room_id;
extern int                g_next_msg_id;
extern int                g_next_friend_id;
extern int                g_next_invite_id;

/* === Mutex 핸들 === */
extern HANDLE             g_sessions_mutex;   /* g_sessions, g_rooms, g_client_count */
extern HANDLE             g_file_mutex;       /* 모든 txt 파일 쓰기 + 파일 영속 캐시 변경 */
extern HANDLE             g_console_mutex;    /* RecvMsg ↔ 메인 스레드 콘솔 출력 충돌 방지 */

#endif /* GLOBALS_H */
```

```c
/* globals.c — 모든 전역 정의(소유자) */
#include "globals.h"

ClientSession      g_sessions[MAX_CLIENTS];
RoomInfo           g_rooms[MAX_ROOMS];
int                g_client_count        = 0;
int                g_room_count          = 0;

UserRecord         g_users[MAX_USERS];
int                g_user_count          = 0;
FriendRecord       g_friends[MAX_FRIENDS];
int                g_friend_count        = 0;
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

int                g_next_room_id        = 1;
int                g_next_msg_id         = 1;
int                g_next_friend_id      = 1;
int                g_next_invite_id      = 1;

HANDLE             g_sessions_mutex      = NULL;
HANDLE             g_file_mutex          = NULL;
HANDLE             g_console_mutex       = NULL;
```

### 6-1. 단조 증가 ID 복원

서버 시작 시 `load_*` 호출 후 다음 ID 카운터를 max+1로 복원해야 재시작 후 ID 충돌을 막는다.

```c
/* main.c — 파일 로드 직후 호출 */
void restore_next_ids(void) {
    g_next_room_id = 1;
    for (int i = 0; i < g_room_count; i++)
        if (g_rooms[i].id >= g_next_room_id)
            g_next_room_id = g_rooms[i].id + 1;

    g_next_msg_id = 1;
    for (int i = 0; i < g_msg_count; i++)
        if (g_messages[i].id >= g_next_msg_id)
            g_next_msg_id = g_messages[i].id + 1;

    g_next_friend_id = 1;
    for (int i = 0; i < g_friend_count; i++)
        if (g_friends[i].id >= g_next_friend_id)
            g_next_friend_id = g_friends[i].id + 1;

    g_next_invite_id = 1;
    for (int i = 0; i < g_room_invite_count; i++)
        if (g_room_invites[i].id >= g_next_invite_id)
            g_next_invite_id = g_room_invites[i].id + 1;
}
```

---

## 7. Mutex 초기화 및 사용 패턴

```c
/* main.c — 서버 시작 시 */
g_sessions_mutex = CreateMutex(NULL, FALSE, NULL);
g_file_mutex     = CreateMutex(NULL, FALSE, NULL);
g_console_mutex  = CreateMutex(NULL, FALSE, NULL);
if (!g_sessions_mutex || !g_file_mutex || !g_console_mutex) {
    fprintf(stderr, "CreateMutex() failed\n");
    exit(1);
}

/* 세션 배열 접근 패턴 */
WaitForSingleObject(g_sessions_mutex, INFINITE);
/* --- 임계 구역 시작 --- */
// g_sessions[] 또는 g_rooms[] 읽기/쓰기
/* --- 임계 구역 끝 --- */
ReleaseMutex(g_sessions_mutex);

/* 파일 쓰기 패턴 */
WaitForSingleObject(g_file_mutex, INFINITE);
/* --- 임계 구역 시작 --- */
FILE *fp = fopen(FILE_USERS, "a");
if (fp) {
    fprintf(fp, "%s//%s//...\n", u->id, u->pw_hash);
    fclose(fp);
}
/* --- 임계 구역 끝 --- */
ReleaseMutex(g_file_mutex);
```

---

## 8. 인메모리 ↔ 파일 동기화 전략

| 연산 | 인메모리 | 파일 |
|------|----------|------|
| 서버 시작 | 파일 전체 로드 | 읽기 전용 |
| 신규 레코드 추가 | 배열에 추가 | `fopen("a")` append |
| 레코드 수정 | 배열에서 수정 | 전체 재작성 (`fopen("w")`) |
| 레코드 삭제 | 배열에서 제거 | 전체 재작성 (`fopen("w")`) |
| 서버 종료 | 메모리 해제 | 이미 반영되어 있음 |
