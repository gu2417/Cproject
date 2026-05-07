# 서버 아키텍처

> **타입 변경 메모**: requirements.md §6의 `ClientSession.fd`는 `int`이지만 본 문서/`in_memory_structures.md`는 Windows WinSock2 환경 반영하여 `SOCKET` 타입을 사용한다. WinSock2의 `SOCKET`은 `UINT_PTR` 기반 unsigned 타입이며, `INVALID_SOCKET`(`(SOCKET)~0`) 비교가 `int < 0` 검사 대신 사용된다.  
> **MySQL 연결 제거**: requirements.md `ClientSession.MYSQL *db` 필드는 MySQL → txt 파일 결정에 따라 제거됨. 파일 I/O는 모두 `g_file_mutex` 보호 하에 `file_io.c`에서 처리한다.

## 1. 서버 파일 구조

```
src/server/
├── main.c              # 진입점: WSAStartup, bind, listen, accept 루프, 파일 로드
├── config.h            # 포트, MAX_CLIENTS, MAX_ROOMS, 파일 경로 상수
├── globals.c           # g_sessions[], g_rooms[], g_sessions_mutex 정의
├── globals.h           # globals.c 선언 헤더
├── client_handler.c    # HandleClient 스레드 (per-client)
├── client_handler.h
├── router.c            # 패킷 TYPE → 핸들러 함수 라우팅 (MsgChecker)
├── router.h
├── file_io.c           # txt 파일 읽기/쓰기 헬퍼 (MySQL 대체)
├── file_io.h
├── auth.c              # 회원가입, 로그인 처리
├── auth.h
├── user_store.c        # 유저·설정 CRUD (users.txt, user_settings.txt)
├── user_store.h
├── friend.c            # 친구 요청/수락/거절/삭제/차단
├── friend.h
├── room.c              # 채팅방 생성/참여/퇴장/관리 (rooms.txt, room_members.txt)
├── room.h
├── dm.c                # 1:1 DM 처리 (messages.txt, dm_reads.txt)
├── dm.h
├── message.c           # 메시지 저장·삭제·수정·검색·핀 (messages.txt)
├── message.h
└── broadcast.c         # 브로드캐스트, 알림 전송
    broadcast.h
```

## 2. 모듈별 책임

| 모듈 | 파일 | 주요 책임 |
|------|------|-----------|
| main | main.c | 서버 소켓 초기화, accept 루프, 파일 데이터 로드, 스레드 생성 |
| client_handler | client_handler.c/h | per-client recv 루프, 패킷 수신 후 router 호출, leftClient 처리 |
| router | router.c/h | 수신 패킷의 TYPE 분류 후 적절한 핸들러 함수로 위임 |
| file_io | file_io.c/h | txt 파일 파싱/저장 헬퍼, NULL 체크, mutex 보호 하 쓰기 |
| auth | auth.c/h | 로그인 검증(SHA256 비교), 회원가입(중복 ID 체크, users.txt 저장) |
| user_store | user_store.c/h | 유저 정보 조회/수정, 설정 조회/수정, 마지막 접속 시간 갱신 |
| friend | friend.c/h | 친구 요청 전송/수락/거절/삭제/차단, 친구 목록 조회, 유저 검색 |
| room | room.c/h | 방 생성/입장/퇴장/초대/강퇴/삭제, 공지·핀 설정, 멤버 목록 |
| dm | dm.c/h | DM 전송, DM 히스토리 조회, 읽음 처리 |
| message | message.c/h | 메시지 저장·삭제·수정(5분 이내)·답장·검색·핀·이모티콘 변환 |
| broadcast | broadcast.c/h | 방 브로드캐스트, 전체 브로드캐스트, 특정 유저 전송, 상태 알림 |

## 3. 스레드 모델

```
main() [메인 스레드 — Accept Loop]
  |
  | accept() → clientSock
  |
  +-- _beginthreadex --> HandleClient(clientSock)  [스레드 #0]
  |                           |
  |                           +-- recv() loop
  |                           |      |
  |                           |      v
  |                           |  router() [MsgChecker]
  |                           |      |
  |                           |      +-- handle_login()
  |                           |      +-- handle_register()
  |                           |      +-- handle_room_create()
  |                           |      +-- handle_room_msg()
  |                           |      +-- handle_friend_add()
  |                           |      +-- ... (기타 핸들러)
  |                           |
  |                           +-- recv() == 0 또는 음수 → leftClient()
  |
  +-- _beginthreadex --> HandleClient(clientSock)  [스레드 #1]
  |
  ...
  +-- _beginthreadex --> HandleClient(clientSock)  [스레드 #255]
```

## 4. main.c 흐름

```c
int main(int argc, char *argv[]) {
    // 1. Mutex 초기화
    g_sessions_mutex = CreateMutex(NULL, FALSE, NULL);
    g_file_mutex     = CreateMutex(NULL, FALSE, NULL);

    // 2. 파일에서 데이터 로드 (파일 없으면 빈 배열로 시작)
    g_user_count = load_users(FILE_USERS, g_users, MAX_USERS);
    g_room_count = load_rooms(FILE_ROOMS, g_rooms, MAX_ROOMS);
    g_friend_count        = load_friends      (FILE_FRIENDS,       g_friends,       MAX_FRIENDS);
    g_msg_count           = load_messages     (FILE_MESSAGES,      g_messages,      MAX_MSG_HISTORY);
    g_room_member_count   = load_room_members (FILE_ROOM_MEMBERS,  g_room_members,  MAX_ROOM_MEMBER_RECORDS);
    g_dm_read_count       = load_dm_reads     (FILE_DM_READS,      g_dm_reads,      MAX_DM_READS);
    g_room_invite_count   = load_room_invites (FILE_ROOM_INVITES,  g_room_invites,  MAX_INVITES);
    g_user_settings_count = load_user_settings(FILE_USER_SETTINGS, g_user_settings, MAX_USERS);
    g_room_read_count     = load_room_reads   (FILE_ROOM_READS,    g_room_reads,    MAX_ROOM_READS);
    restore_next_ids();   /* g_next_room_id, g_next_msg_id, g_next_friend_id, g_next_invite_id */

    // 3. WinSock 초기화
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // 4. 서버 소켓 생성 및 바인딩
    serverSock = socket(PF_INET, SOCK_STREAM, 0);
    serverAddr.sin_family      = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port        = htons(DEFAULT_PORT);
    bind(serverSock, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    listen(serverSock, 10);

    printf("Server listening on port %d...\n", DEFAULT_PORT);

    // 5. Accept 루프
    while (1) {
        clientSock = accept(serverSock, (SOCKADDR*)&clientAddr, &addrSize);

        WaitForSingleObject(g_sessions_mutex, INFINITE);
        // 빈 슬롯에 소켓 등록
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!g_sessions[i].active) {
                g_sessions[i].fd     = clientSock;
                g_sessions[i].active = 1;
                g_client_count++;
                hThread = (HANDLE)_beginthreadex(
                    NULL, 0, HandleClient,
                    (void*)&g_sessions[i], 0, NULL);
                g_sessions[i].hThread = hThread;
                break;
            }
        }
        ReleaseMutex(g_sessions_mutex);

        printf("Connected: %s\n", inet_ntoa(clientAddr.sin_addr));
    }

    closesocket(serverSock);
    WSACleanup();
    return 0;
}
```

## 5. Mutex 보호 범위

| 공유 자원 | Mutex | 위험 연산 |
|-----------|-------|-----------|
| `g_sessions[]` | `g_sessions_mutex` | active 읽기/쓰기, fd 접근, 세션 정보 갱신 |
| `g_rooms[]` (인메모리) | `g_sessions_mutex` | 방 생성/삭제, 멤버 추가/삭제 |
| `g_client_count` | `g_sessions_mutex` | 카운터 증감 |
| txt 파일 쓰기 | `g_file_mutex` | fopen, fprintf, fclose |

## 6. router.c/h — 패킷 디스패치 (MsgChecker)

`client_handler.c`의 `recv()` 루프가 패킷 한 건을 수신할 때마다 `router(buf, sess)`을 호출한다. router는 PAYLOAD 첫 번째 `|` 까지의 TYPE 문자열을 추출하여 적절한 핸들러로 위임한다.

### 6-1. 함수 시그니처

```c
/* router.h */
#ifndef ROUTER_H
#define ROUTER_H
#include "globals.h"

/* 단일 패킷 dispatch — recv 버퍼에는 단일 패킷만 들어왔다고 가정.
 * 다중 패킷이 한 번에 도착할 경우 client_handler.c가 '\n' 단위로 분할 후
 * 패킷별로 router()를 호출한다. */
void router(char *buf, ClientSession *sess);

#endif
```

### 6-2. 디스패치 테이블 패턴

```c
/* router.c */
typedef void (*PacketHandler)(char *payload, ClientSession *sess);

typedef struct {
    const char    *type;
    PacketHandler  handler;
} RouteEntry;

static const RouteEntry g_routes[] = {
    /* 인증 (FR-A) */
    { LOGIN_REQ,         handle_login         },
    { REGISTER_REQ,      handle_register      },
    { LOGOUT_REQ,        handle_logout        },   /* P1+ */

    /* 친구 (FR-F) */
    { FRIEND_ADD_REQ,    handle_friend_add    },
    { FRIEND_ACCEPT,     handle_friend_accept },
    { FRIEND_REJECT,     handle_friend_reject },
    { FRIEND_DELETE,     handle_friend_delete },
    { FRIEND_BLOCK,      handle_friend_block  },
    { FRIEND_LIST_REQ,   handle_friend_list   },
    { USER_SEARCH,       handle_user_search   },

    /* DM (FR-D) */
    { DM_SEND,           handle_dm_send       },
    { DM_HISTORY_REQ,    handle_dm_history    },
    { DM_LIST_REQ,       handle_dm_list       },

    /* 방 (FR-G, FR-O) */
    { ROOM_CREATE,       handle_room_create   },
    { ROOM_LIST_REQ,     handle_room_list     },
    { ROOM_SEARCH,       handle_room_search   },
    { ROOM_JOIN,         handle_room_join     },
    { ROOM_MSG,          handle_room_msg      },
    { ROOM_HISTORY_REQ,  handle_room_history  },
    { ROOM_LEAVE,        handle_room_leave    },
    { ROOM_INVITE,       handle_room_invite   },
    { ROOM_KICK,         handle_room_kick     },
    { ROOM_DELETE,       handle_room_delete   },
    { ROOM_SET_NOTICE,   handle_room_set_notice },
    { ROOM_GRANT_ADMIN,  handle_room_grant_admin },
    { ROOM_REVOKE_ADMIN, handle_room_revoke_admin },
    { ROOM_MEMBERS_REQ,  handle_room_members  },

    /* 메시지 부가 (FR-M) */
    { WHISPER,           handle_whisper       },
    { MSG_DELETE,        handle_msg_delete    },
    { MSG_EDIT,          handle_msg_edit      },
    { MSG_REPLY,         handle_msg_reply     },
    { MSG_SEARCH,        handle_msg_search    },
    { MSG_PIN,           handle_msg_pin       },

    /* 설정 (FR-C) */
    { SETTINGS_REQ,         handle_settings_req     },
    { SETTINGS_UPDATE,      handle_settings_update  },
    { STATUS_CHANGE,        handle_status_change    },
    { PROFILE_UPDATE,       handle_profile_update   },
    { PASS_CHANGE,          handle_pass_change      },
    { ROOM_SET_OPEN_NICK,   handle_room_set_open_nick },
    { ROOM_MUTE_TOGGLE,     handle_room_mute_toggle },   /* P3+ */

    /* 마이페이지 (FR-P) */
    { MYPAGE_REQ,    handle_mypage_req   },
    { MY_ROOMS_REQ,  handle_my_rooms_req },

    /* 타이핑 (FR-N05) */
    { TYPING_START,  handle_typing_start },
    { TYPING_STOP,   handle_typing_stop  },

    /* 연결 유지 (P2+) */
    { PING,          handle_ping         },
};
#define G_ROUTES_COUNT (sizeof(g_routes) / sizeof(g_routes[0]))

void router(char *buf, ClientSession *sess) {
    /* '\n' 제거 */
    char *nl = strchr(buf, '\n');
    if (nl) *nl = '\0';

    /* TYPE | PAYLOAD 분리 — 첫 번째 '|' 만 사용 */
    char *bar = strchr(buf, '|');
    char *type, *payload;
    if (bar) {
        *bar = '\0';
        type    = buf;
        payload = bar + 1;
    } else {
        type    = buf;
        payload = "";
    }

    /* 인증 패킷 외에는 로그인 상태 검증 */
    if (!sess->active) return;
    if (strcmp(type, LOGIN_REQ) != 0 &&
        strcmp(type, REGISTER_REQ) != 0 &&
        sess->user_id[0] == '\0') {
        /* 로그인 전 다른 패킷 수신 — 표준 NOTIFY|SERVER 로 안내 */
        send_packet(sess->fd, "NOTIFY|SERVER:로그인이 필요합니다.");
        return;
    }

    for (size_t i = 0; i < G_ROUTES_COUNT; i++) {
        if (strcmp(type, g_routes[i].type) == 0) {
            g_routes[i].handler(payload, sess);
            return;
        }
    }
    /* 알 수 없는 패킷은 무시 (로그만 남김) */
    server_log("Unknown packet type: %s\n", type);
}
```

### 6-3. P0 단계 router

P0에서는 `handle_login`, `handle_register`, `handle_room_create`, `handle_room_join`, `handle_room_msg`, `handle_room_leave`, `handle_room_list` 만 등록하고 나머지는 미구현 (Phase 별 구현은 `development_phases.md` 참조).

---

## 7. leftClient 처리 흐름

```c
void leftClient(ClientSession *sess) {
    printf("Client disconnected: %s\n", sess->user_id);

    // 1. 참여 중이던 방에서 퇴장 처리
    if (sess->current_room_id > 0) {
        handle_room_leave_internal(sess->current_room_id, sess->user_id);
    }

    // 2. 온라인 상태 → offline 갱신 (users.txt)
    update_user_online_status(sess->user_id, 0);
    update_user_last_seen(sess->user_id);

    // 3. 친구들에게 오프라인 상태 변경 알림
    notify_friend_status_change(sess->user_id, 0);

    // 4. 세션 슬롯 해제
    WaitForSingleObject(g_sessions_mutex, INFINITE);
    closesocket(sess->fd);
    memset(sess, 0, sizeof(ClientSession));
    sess->fd = INVALID_SOCKET;
    g_client_count--;
    ReleaseMutex(g_sessions_mutex);
}
```
