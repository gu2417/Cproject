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
1. `friends.txt`에서 해당 레코드의 `status → 2` (blocked)
2. 차단된 유저로부터의 DM 및 메시지 수신 차단
3. 차단 유저는 친구 목록에서 숨김 처리

차단 시 메시지 수신 차단 구현:
```c
/* handle_dm_send() 내부 */
FriendRecord *fr = find_friend_record(to_id, from_id);
if (fr && fr->status == 2) {
    /* 차단 — 조용히 무시 (발신자에게 알림 없음) */
    return;
}
```

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
