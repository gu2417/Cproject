# FR-N: 알림

## FR-N01 — 메시지 알림

현재 보고 있지 않은 채팅방/DM의 메시지 수신 시 콘솔에 알림을 출력한다.

### 처리 로직

```c
/* packet_parse()에서 ROOM_MSG_RECV 처리 */
int room_id = atoi(strtok(payload, ":"));
/* ... 필드 파싱 ... */

if (room_id == g_state.current_room_id) {
    /* 현재 보는 방 → 즉시 화면에 출력 */
    display_chat_message(from_nick, timestamp, content, msg_type, reply_to);
} else {
    /* 다른 방 → 알림 출력 (DND 모드 확인) */
    if (!g_state.dnd) {
        /* muted_rooms 확인 */
        int muted = is_room_muted(room_id);
        if (!muted) {
            printf("\n[알림] 채팅방 #%d (%s): %s\n> ",
                   room_id, room_name_cache, content);
            fflush(stdout);
        }
    }
}
```

콘솔 알림 형식:
```
[알림] 채팅방 #3 (개발팀): 홍길동: 회의 시작합니다!
```

---

## FR-N02 — 친구 요청 알림

### 실시간 알림 (온라인 시)

```
S→C  FRIEND_REQUEST_NOTIFY|<from_id>:<from_nick>
```

```c
else if (strcmp(type, "FRIEND_REQUEST_NOTIFY") == 0) {
    char *from_id   = strtok(payload, ":");
    char *from_nick = strtok(NULL, ":");
    printf("\n[친구 요청] %s(%s) 님이 친구 요청을 보냈습니다.\n> ",
           from_nick, from_id);
    fflush(stdout);
}
```

### 로그인 시 대기 중인 요청 알림

로그인 성공 후 서버가 `friends.txt`에서 `status=0`(pending)이고 `friend_id=user_id`인 레코드를 확인하여 알림 전송:

```c
/* handle_login() 완료 후 서버에서 */
void notify_pending_friend_requests(const char *user_id, ClientSession *sess) {
    for (int i = 0; i < g_friend_count; i++) {
        FriendRecord *fr = &g_friends[i];
        if (fr->status != 0) continue;
        if (strcmp(fr->friend_id, user_id) != 0) continue;

        char from_nick[21] = {0};
        get_nickname(fr->user_id, from_nick);
        send_packet(sess->fd, "FRIEND_REQUEST_NOTIFY|%s:%s",
                    fr->user_id, from_nick);
    }
}
```

---

## FR-N03 — 멘션 알림 (P3)

`@닉네임` 포함 메시지 수신 시 강조 알림을 표시한다. **DND 모드에서도 표시**한다.

```c
/* packet_parse()에서 ROOM_MSG_RECV 처리 시 멘션 감지 */
char mention_tag[25];
snprintf(mention_tag, sizeof(mention_tag), "@%s", g_state.nickname);

if (strstr(content, mention_tag) != NULL) {
    /* DND 여부와 무관하게 항상 강조 표시 */
    printf("\n[멘션] %s: %s\n> ", from_nick, content);
    fflush(stdout);
} else if (room_id != g_state.current_room_id && !g_state.dnd) {
    printf("\n[알림] 채팅방 #%d: %s\n> ", room_id, content);
    fflush(stdout);
}
```

콘솔 표시:
```
[멘션] 김철수: @홍길동 오늘 회의 자료 확인하셨나요?
```

---

## FR-N04 — DND 모드 (P3)

방해금지 모드 활성화 시 멘션을 제외한 모든 알림을 무시한다.

### 설정 변경

```
C→S  SETTINGS_UPDATE|<msg_color>:<nick_color>:<theme>:<ts_format>:1
                                                                   ^
                                                               dnd=1
S→C  SETTINGS_UPDATE_RES|0
```

### 클라이언트 상태

```c
g_state.dnd = 1;  /* DND 활성화 */
```

### 알림 필터링 로직

```c
/* 알림 출력 전 공통 검사 */
int should_notify(int room_id, int is_mention) {
    if (is_mention) return 1;          /* 멘션은 DND에서도 표시 */
    if (g_state.dnd) return 0;         /* DND 모드 → 차단 */
    if (is_room_muted(room_id)) return 0; /* 방 무음 → 차단 */
    return 1;
}
```

설정 화면 표시:
```
  5. 방해금지 모드   : [OFF] → 1 입력 시 [ON ]으로 변경
```

---

## FR-N05 — 타이핑 표시 (P3)

상대방이 입력 중일 때 채팅방 화면 하단에 표시한다.

```
C→S  TYPING_START|<room_id>   (메시지 입력 시작 시)
C→S  TYPING_STOP|<room_id>    (전송 또는 입력 취소 시)
S→C  TYPING_NOTIFY|<room_id>:<nick>:<is_typing>
```

클라이언트 전송 시점:
- 입력창에 첫 문자 입력 시 `TYPING_START` 전송
- 메시지 전송 또는 입력 내용 삭제 시 `TYPING_STOP` 전송

클라이언트 수신 처리:
```c
else if (strcmp(type, "TYPING_NOTIFY") == 0) {
    int  room_id   = atoi(strtok(payload, ":"));
    char *nick     = strtok(NULL, ":");
    int  is_typing = atoi(strtok(NULL, ":"));

    if (room_id == g_state.current_room_id) {
        if (is_typing) {
            printf("\r[%s 님이 입력 중...]        \n> 메시지 입력: ",
                   nick);
        } else {
            printf("\r                              \n> 메시지 입력: ");
        }
        fflush(stdout);
    }
}
```

콘솔 표시:
```
[홍길동 님이 입력 중...]
> 메시지 입력:
```

> **구현 참고**: 콘솔에서 타이핑 표시는 `\r`(캐리지 리턴)로 같은 줄을 덮어쓰거나, 별도 줄에 출력 후 지우는 방식으로 구현한다. 완벽한 커서 제어를 위해 Windows Console API(`SetConsoleCursorPosition`) 사용을 고려할 수 있다.

---

## FR-N06 — 방 알림 무음 (P3)

특정 채팅방의 알림만 무음 처리한다.

```
C→S  ROOM_MUTE_TOGGLE|<room_id>
S→C  ROOM_MUTE_TOGGLE_RES|<room_id>:<is_muted>
```

서버 처리:
- `room_members.txt`의 `is_muted` 필드 토글
- 세션의 `muted_rooms[]` 배열 갱신

클라이언트:
- `ClientSession.muted_rooms[]`에 room_id 추가/제거
- `is_room_muted(room_id)` 함수로 알림 출력 전 검사

```c
int is_room_muted(int room_id) {
    for (int i = 0; i < g_state.muted_count; i++) {
        if (g_state.muted_rooms[i] == room_id) return 1;
    }
    return 0;
}
```

채팅방 목록 표시:
```
  1. 개발팀    [2/10] [무음]
  2. 스터디    [5/20]
```
