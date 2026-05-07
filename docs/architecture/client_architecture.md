# 클라이언트 아키텍처

## 1. 클라이언트 파일 구조

```
src/client/
├── main.c              # 진입점: WSAStartup, connect, RecvMsg 스레드 시작, 메뉴 루프
├── state.c             # 전역 클라이언트 상태 (소켓, 로그인 정보, 현재 방, 설정)
├── state.h
├── net.c               # 소켓 연결, send_packet(), RecvMsg 스레드
├── net.h
├── packet.c            # packet_build() / packet_parse() 직렬화/역직렬화
├── packet.h
├── menu_initial.c      # 초기 메뉴: 로그인 / 회원가입 / 종료
├── menu_initial.h
├── menu_main.c         # 메인 로비 메뉴: 친구/채팅/오픈채팅/DM/마이페이지/설정/로그아웃
├── menu_main.h
├── menu_chat.c         # 채팅방 메뉴: 메시지 입력, 슬래시 명령어 처리
├── menu_chat.h
├── menu_friend.c       # 친구 관리 메뉴: 목록/추가/수락/삭제/차단/검색
├── menu_friend.h
├── menu_dm.c           # DM 메뉴: 대화 목록, DM 전송, 히스토리
├── menu_dm.h
├── menu_mypage.c       # 마이페이지: 프로필/통계/참여방목록/비번변경
├── menu_mypage.h
└── menu_settings.c     # 설정 메뉴: 색상/테마/타임스탬프/DND/상태/오픈닉
    menu_settings.h
```

---

## 2. 전역 클라이언트 상태 (state.h)

```c
/* state.h */
typedef struct {
    SOCKET  sock;                       /* 서버 소켓 */
    int     logged_in;                  /* 0=미로그인, 1=로그인 */
    char    user_id[21];
    char    nickname[21];
    int     online_status;              /* 1=online, 2=busy, 3=invisible */
    int     current_room_id;            /* 0=로비, >0=채팅방 */
    char    current_room_name[31];

    /* DM 화면 컨텍스트 (FR-D01~D05) */
    char    current_dm_partner[21];     /* 현재 DM 대화 상대 ID, 빈 값 = DM 화면 밖 */
    char    current_dm_partner_nick[21];/* 상대 닉네임 캐시 (히스토리 출력용) */

    /* 알림 설정 (FR-N04, FR-N06) */
    int     muted_rooms[32];            /* 알림 무음 room_id 목록 */
    int     muted_count;                /* muted_rooms 유효 항목 수 */

    /* 연결 상태 (NFR-03, P2 PING/PONG) */
    int     connected;                  /* 1=서버 연결 정상, 0=재연결 중 */
    int     response_received;          /* 마지막 요청 응답 수신 여부 (재시도 판정) */
    time_t  last_pong;                  /* 마지막 PONG 수신 시각 (P2+) */

    /* 설정 (FR-C01~C06) */
    char    msg_color[16];              /* 기본: "cyan" */
    char    nick_color[16];             /* 기본: "yellow" */
    char    theme[11];                  /* "dark" / "light" */
    int     ts_format;                  /* 0=HH:MM, 1=HH:MM:SS, 2=MM-DD HH:MM */
    int     dnd;                        /* 0=알림 켜짐, 1=방해금지 */
} ClientState;

extern ClientState g_state;
```

> **메모**: `current_dm_partner`는 빈 문자열 비교(`g_state.current_dm_partner[0] == '\0'`)로 DM 화면 진입 여부를 판정한다. 메인 로비/채팅방 진입 시 반드시 `g_state.current_dm_partner[0] = '\0'`로 클리어한다.

---

## 3. 스레드 구조

```
main() [메인 스레드]
  |
  +-- WSAStartup() → socket() → connect()
  |
  +-- _beginthreadex --> RecvMsg()  [수신 전용 스레드]
  |                          |
  |                          +-- recv() loop
  |                          +-- packet_parse() → 콘솔 출력
  |
  +-- InitialMenu()   [로그인/회원가입 메뉴 루프]
  |
  +-- (로그인 성공 시)
  |
  +-- ShowMainMenu()  [메인 로비 루프]
        |
        +-- 1. 친구 목록 → ShowFriendMenu()
        +-- 2. 채팅방 목록 → ShowRoomMenu() → ShowChatRoom()
        +-- 3. 오픈채팅 목록 → ShowOpenChatMenu()
        +-- 4. DM → ShowDMMenu()
        +-- 5. 마이페이지 → ShowMyPageMenu()
        +-- 6. 설정 → ShowSettingsMenu()
        +-- 7. 로그아웃
```

---

## 4. RecvMsg 스레드 — 비동기 수신 패턴

RecvMsg 스레드는 메인 스레드의 메뉴 루프와 독립적으로 동작하며, 서버에서 수신된 패킷을 즉시 콘솔에 출력한다.

```c
unsigned WINAPI RecvMsg(void *arg) {
    SOCKET sock = *((SOCKET*)arg);
    char   buf[MAX_BUF_SIZE];
    int    len;

    while (1) {
        len = recv(sock, buf, sizeof(buf) - 1, 0);
        if (len <= 0) {
            printf("\n[서버 연결이 끊어졌습니다.]\n");
            break;
        }
        buf[len] = '\0';
        packet_parse(buf, sock);
    }
    return 0;
}
```

### packet_parse() — 수신 패킷 처리

```c
void packet_parse(const char *buf, SOCKET sock) {
    char tmp[MAX_BUF_SIZE];
    strncpy(tmp, buf, MAX_BUF_SIZE - 1);

    char *type    = strtok(tmp, "|");
    char *payload = strtok(NULL, "");
    if (!type) return;

    if (strcmp(type, "ROOM_MSG_RECV") == 0) {
        /* room_id:from_nick:timestamp:msg_id:reply_to_id:msg_type:content */
        int  room_id     = atoi(strtok(payload, ":"));
        char *from_nick  = strtok(NULL, ":");
        char *timestamp  = strtok(NULL, ":");
        int  msg_id      = atoi(strtok(NULL, ":"));
        int  reply_to    = atoi(strtok(NULL, ":"));
        int  msg_type    = atoi(strtok(NULL, ":"));
        char *content    = strtok(NULL, "");  /* content-last */

        if (room_id == g_state.current_room_id) {
            display_chat_message(from_nick, timestamp,
                                 content, msg_type, reply_to);
        } else if (!g_state.dnd) {
            printf("\n[알림] 채팅방 #%d: %s\n> ", room_id, content);
        }
    }
    else if (strcmp(type, "DM_RECV") == 0) {
        char *from_id   = strtok(payload, ":");
        char *from_nick = strtok(NULL, ":");
        char *timestamp = strtok(NULL, ":");
        /* msg_id 건너뜀 */    strtok(NULL, ":");
        char *content   = strtok(NULL, "");
        if (!g_state.dnd) {
            printf("\n[DM] %s: %s\n> ", from_nick, content);
        }
    }
    else if (strcmp(type, "FRIEND_REQUEST_NOTIFY") == 0) {
        char *from_id   = strtok(payload, ":");
        char *from_nick = strtok(NULL, ":");
        printf("\n[친구 요청] %s(%s)님이 친구 요청을 보냈습니다.\n> ",
               from_nick, from_id);
    }
    else if (strcmp(type, "FRIEND_STATUS_CHANGE") == 0) {
        char *id     = strtok(payload, ":");
        char *nick   = strtok(NULL, ":");
        int   status = atoi(strtok(NULL, ":"));
        const char *status_str[] = {"오프라인", "온라인", "바쁨", "오프라인"};
        printf("\n[알림] %s(%s) → %s\n> ",
               nick, id, status_str[status]);
    }
    else if (strcmp(type, "TYPING_NOTIFY") == 0) {
        /* room_id:nick:is_typing */
        int  room_id  = atoi(strtok(payload, ":"));
        char *nick    = strtok(NULL, ":");
        int  is_typing = atoi(strtok(NULL, ":"));
        if (room_id == g_state.current_room_id && is_typing)
            printf("\n[%s 님이 입력 중...]\n> ", nick);
    }
    /* ... 기타 패킷 처리 */
}
```

---

## 5. 메시지 표시 형식

```c
void display_chat_message(const char *from_nick, const char *timestamp,
                           const char *content, int msg_type, int reply_to) {
    switch (msg_type) {
        case 0:  /* 일반 */
            printf("[%s] %s: %s\n", timestamp, from_nick, content);
            break;
        case 1:  /* 시스템 */
            printf("--- %s ---\n", content);
            break;
        case 2:  /* 귓속말 */
            printf("[귓속말] %s: %s\n", from_nick, content);
            break;
        case 3:  /* /me 액션 — 서버는 raw 동작만 송신, 클라이언트가 닉네임 합성 */
            printf("* %s %s\n", from_nick, content);
            break;
    }
}
```

---

## 6. net.c — 소켓 연결

```c
/* net.h */
SOCKET connect_to_server(const char *ip, int port);
void   send_packet(SOCKET sock, const char *fmt, ...);
void   start_recv_thread(SOCKET sock);
```

```c
SOCKET connect_to_server(const char *ip, int port) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET sock = socket(PF_INET, SOCK_STREAM, 0);

    SOCKADDR_IN addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port        = htons(port);

    if (connect(sock, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        fprintf(stderr, "서버 연결 실패\n");
        exit(1);
    }
    return sock;
}
```
