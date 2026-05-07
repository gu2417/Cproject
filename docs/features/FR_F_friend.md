# FR-F: 친구 관리

## FR-F01 — 친구 추가

### 흐름

```
[클라이언트 — 친구 추가 메뉴]
    > 추가할 유저 ID 입력: bob
    |
    | FRIEND_ADD_REQ|bob
    |---------------------------------> [서버]
    |                                      |
    |                          bob 존재 확인 (users.txt)
    |                          차단/중복 여부 확인 (friends.txt)
    |                          pending 레코드 추가 (friends.txt)
    |                                      |
    |                          bob가 온라인이면:
    |                          FRIEND_REQUEST_NOTIFY|alice:홍길동 → bob
    |                                      |
    | FRIEND_ADD_RES|0 (SENT)             |
    |<---------------------------------
```

### 응답 코드

| 코드 | 의미 |
|------|------|
| 0 | 요청 전송 성공 |
| 1 | 대상 유저 없음 |
| 2 | 차단된 유저 |
| 3 | 이미 친구 관계 |

---

## FR-F02 — 친구 요청 수락/거절

### 수락 흐름

```
[클라이언트 — 친구 요청 목록]
  1. alice (홍길동)
  > 1번 수락

    | FRIEND_ACCEPT|alice
    |---------------------------------> [서버]
    |                          friends.txt: status → 1
    |                          alice가 온라인이면:
    |                          FRIEND_ACCEPT_NOTIFY|bob:김철수 → alice
    |
```

### 거절 흐름

```
    | FRIEND_REJECT|alice
    |---------------------------------> [서버]
    |                          friends.txt: 레코드 삭제
    |                          (알림 없음)
```

### 패킷

```
C→S  FRIEND_ACCEPT|<from_id>
S→C  FRIEND_ACCEPT_NOTIFY|<user_id>:<nick>   (요청 송신자에게)

C→S  FRIEND_REJECT|<from_id>
     (응답 없음)
```

---

## FR-F03 — 친구 목록 조회

### 표시 형식

```
============================================================
    [친구 목록] 총 3명
============================================================
  1. [ON ] 김철수  (kimcs)  | 오늘도 열심히!
  2. [OFF] 이영희  (leeya)  | 마지막 접속: 2시간 전
  3. [바쁨] 박민준 (parkmj) | 회의 중
```

### 온라인 상태 표시 규칙

| online_status 값 | 실제 상태 | 화면 표시 |
|-----------------|-----------|-----------|
| 0 | offline | `[OFF]` |
| 1 | online | `[ON ]` |
| 2 | busy | `[바쁨]` |
| 3 | invisible | `[OFF]` (타인에게는 offline으로) |

> invisible(3) 설정자는 서버가 `FRIEND_STATUS_CHANGE` 알림 시 `status=0`(offline)으로 전송.

### 패킷

```
C→S  FRIEND_LIST_REQ|
S→C  FRIEND_LIST_RES|<count>:<id>:<nick>:<status>:<status_msg>;<id>:...
```

---

## FR-F04 — 친구 삭제

```
C→S  FRIEND_DELETE|<target_id>
     (응답 없음 — 목록 새로고침으로 확인)
```

서버 처리:
1. `friends.txt`에서 해당 레코드 삭제 (양방향 모두 삭제)
2. 인메모리 캐시 갱신
3. 상대방에게 별도 알림 없음

---

## FR-F05 — 친구 차단

```
C→S  FRIEND_BLOCK|<target_id>
     (응답 없음)
```

서버 처리:
1. `friends.txt`에서 (user_id=차단자, friend_id=피차단자) 레코드의 `status → 2` (blocked).
   레코드가 없으면 신규 추가 (`status=2`로 직접 생성).
2. 차단 관계는 **단방향 단일 레코드**로 표현. 양쪽이 서로를 차단한 경우 두 개의 레코드가 존재.
3. 차단 유저는 차단자의 친구 목록에서 숨김 처리.

### 차단 효과 매트릭스

차단자 = `BLK`, 피차단자 = `TGT` 관점에서 모든 상호작용을 정의한다. 검사는 항상 **수신자 측 friends.txt**에서 (수신자, 발신자) 페어의 `status==2` 여부를 확인한다.

| 상호작용 | BLK → TGT | TGT → BLK | 검사 위치 |
|----------|-----------|-----------|-----------|
| **DM 송신** | 허용(상대 무수신) | 차단 (`FRIEND_ADD_RES` 패턴 응답 없이 무시) | `handle_dm_send()` |
| **친구 요청 (`FRIEND_ADD`)** | 허용(상대 무수신) | 차단 → `FRIEND_ADD_RES\|2` (BLOCKED) | `handle_friend_add()` |
| **방 초대 (`ROOM_INVITE`)** | 허용 (BLK이 TGT를 초대 가능) | **차단** → `ROOM_INVITE_RES\|1`(NOT_FOUND 위장) | `handle_room_invite()` |
| **같은 방 메시지 (브로드캐스트)** | 그대로 도달 (방 멤버 자격 우선) | 그대로 도달 (방 멤버 자격 우선) | 검사 없음 |
| **귓속말 (`/w`)** | 허용 | 차단 (`WHISPER_RECV` 미전달) | `handle_whisper()` |
| **친구 상태 변경 알림** | 미전송 (TGT가 BLK 친구 목록에서 숨김) | 미전송 | `notify_friend_status_change()` |
| **유저 검색 결과** | 노출 | 노출 | 검사 없음 (검색은 차단과 무관) |

> **핵심 원칙**: 차단은 **피차단자가 차단자에게 보내는 행동**을 막는다. 차단자가 상대를 보지 못하게 하는 효과는 클라이언트 표시 단계에서 처리한다.  
> **방 멤버 자격 우선**: 같은 방에 이미 들어와 있는 메시지는 차단으로 막지 않는다. 차단 후에도 같은 방의 메시지는 받게 된다 — 방을 나가야 차단 효과가 적용된다 (FR-G05).  
> **검색 노출 정책**: 차단된 유저도 `USER_SEARCH`에 노출된다. 차단 해제 후 다시 친구 추가하기 위해 필요하기 때문 (단, 클라이언트는 차단 표시를 원하면 친구 목록 status=2 검사 추가).

### 차단 검사 헬퍼

```c
/* (수신자 입장에서) 발신자가 차단되었는지 검사 */
int is_blocked_by(const char *receiver_id, const char *sender_id) {
    for (int i = 0; i < g_friend_count; i++) {
        if (strcmp(g_friends[i].user_id,   receiver_id) == 0 &&
            strcmp(g_friends[i].friend_id, sender_id)   == 0 &&
            g_friends[i].status == 2) {
            return 1;
        }
    }
    return 0;
}

/* DM 송신 시 차단 체크 */
void handle_dm_send(const char *to_id, const char *content,
                    ClientSession *sess) {
    if (is_blocked_by(to_id, sess->user_id)) {
        /* 조용히 무시 — 발신자에게 응답 없음 (차단 사실 노출 방지) */
        return;
    }
    /* ... 정상 DM 처리 */
}

/* 방 초대 시 차단 체크 */
void handle_room_invite(int room_id, const char *target_id,
                        ClientSession *sess) {
    if (is_blocked_by(target_id, sess->user_id)) {
        /* NOT_FOUND로 위장 — 차단당했음을 알리지 않음 */
        send_packet(sess->fd, "ROOM_INVITE_RES|1");
        return;
    }
    /* ... 정상 초대 처리 */
}
```

### 차단 해제 정책

차단 해제는 `FRIEND_DELETE`로 통일한다 (`status=2` 레코드 자체를 삭제). `status` 값을 0으로 되돌리는 정책은 사용하지 않는다 (file_schema.md §4 friends.txt status 값 정의 참조).

해제 후 친구 관계는 자동 복원되지 **않는다**. 차단 이전에 accepted 친구였더라도 해제 후에는 일반 사용자(친구 아님)가 된다 — 다시 친구가 되려면 `FRIEND_ADD_REQ` 재요청 필요.

---

## FR-F06 — 온라인 상태 표시

실시간 업데이트: 친구가 로그인/로그아웃/상태 변경 시 `FRIEND_STATUS_CHANGE` 수신.

```c
/* packet_parse()에서 처리 */
else if (strcmp(type, "FRIEND_STATUS_CHANGE") == 0) {
    char *id     = strtok(payload, ":");
    char *nick   = strtok(NULL, ":");
    int   status = atoi(strtok(NULL, ":"));

    const char *label;
    switch (status) {
        case 1:  label = "[ON ]"; break;
        case 2:  label = "[바쁨]"; break;
        default: label = "[OFF]"; break;
    }
    printf("\n[알림] %s %s\n> ", nick, label);
    fflush(stdout);
}
```

---

## FR-F07 — 유저 검색

```
C→S  USER_SEARCH|<keyword>
S→C  USER_SEARCH_RES|<count>:<id>:<nick>:<status_msg>;<id>:...
```

서버는 `users.txt` 인메모리 배열에서 `id` 또는 `nickname`에 keyword가 포함된 유저를 반환한다.

```c
void handle_user_search(const char *keyword, ClientSession *sess) {
    UserRecord results[50];
    int found = 0;

    for (int i = 0; i < g_user_count && found < 50; i++) {
        if (strstr(g_users[i].id,       keyword) ||
            strstr(g_users[i].nickname, keyword)) {
            results[found++] = g_users[i];
        }
    }

    char buf[MAX_BUF_SIZE];
    int  off = 0;
    off += snprintf(buf + off, sizeof(buf) - off,
                    "USER_SEARCH_RES|%d", found);
    for (int i = 0; i < found; i++) {
        off += snprintf(buf + off, sizeof(buf) - off,
                        ":%s:%s:%s",
                        results[i].id,
                        results[i].nickname,
                        results[i].status_msg);
        if (i < found - 1) buf[off++] = ';';
    }
    buf[off++] = '\n';
    buf[off]   = '\0';
    send(sess->fd, buf, off, 0);
}
```

검색 결과 화면에서 번호를 선택해 친구 추가(`FRIEND_ADD_REQ`) 또는 DM 시작(`DM_SEND`)으로 연결.
