# FR-O: 오픈채팅방

## 오픈채팅 vs 그룹채팅 구분

`rooms.txt`의 `is_open` 필드로 구분한다.

| is_open | 유형 | 참여 방식 |
|---------|------|-----------|
| 0 | 그룹채팅 | 초대 전용 |
| 1 | 오픈채팅 | 목록 조회 후 자유 참여 |

---

## FR-O01 — 오픈채팅방 생성

그룹채팅 생성(`FR-G01`)과 동일한 패킷이지만 `is_open=1`로 설정한다.

```
C→S  ROOM_CREATE|<name>:<max_users>:1[:<password>[:<topic>]]
S→C  ROOM_CREATE_RES|1:<room_id>
```

### 콘솔 입력 화면

```
============================================================
    Create New Open Chat Room
============================================================
> 방 이름 (최대 30자): 코딩 질문방
> 최대 인원 (기본 10, 최대 64): 50
> 비밀번호 (없으면 Enter):
> 방 주제 (최대 100자): 자유롭게 질문하세요

> [성공] 오픈채팅방 생성 완료 (ID: 5)
```

---

## FR-O02 — 목록 조회

`is_open=1`인 방만 필터링하여 반환한다.

```
C→S  ROOM_LIST_REQ|open
S→C  ROOM_LIST_RES|<id>:<name>:<cur>:<max>:<has_pw>:<is_open>:<topic>;<id>:...
```

서버 필터링:
```c
void handle_room_list_req(const char *type, ClientSession *sess) {
    char buf[MAX_BUF_SIZE];
    int  off = 0;
    int  count = 0;
    char items[MAX_ROOMS][256];

    for (int i = 0; i < MAX_ROOMS; i++) {
        RoomInfo *r = &g_rooms[i];
        if (!r->active) continue;

        /* type 필터 */
        if (type && strcmp(type, "open")  == 0 && !r->is_open)  continue;
        if (type && strcmp(type, "group") == 0 &&  r->is_open)  continue;

        int has_pw = (strlen(r->password_hash) > 0) ? 1 : 0;
        snprintf(items[count++], 256,
                 "%d:%s:%d:%d:%d:%d:%s",
                 r->id, r->name, r->member_count,
                 r->max_users, has_pw, r->is_open, r->topic);
    }

    off += snprintf(buf, sizeof(buf), "ROOM_LIST_RES|");
    for (int i = 0; i < count; i++) {
        off += snprintf(buf + off, sizeof(buf) - off, "%s", items[i]);
        if (i < count - 1) buf[off++] = ';';
    }
    buf[off++] = '\n';
    buf[off]   = '\0';
    send(sess->fd, buf, off, 0);
}
```

### 목록 화면

```
============================================================
    [OPEN CHATTING ROOMS]
============================================================
  번호  방이름              인원    주제
  ----  ------------------  ------  --------------------
  1.    코딩 질문방         12/50   자유롭게 질문하세요
  2.    취업 정보            7/30   취준생 모여라
  3.    게임 파티  [비번]    3/10   같이 게임해요
------------------------------------------------------------
c. 방 만들기   s. 방 검색   0. 뒤로 가기
> Enter Command:
```

---

## FR-O03 — 방 검색 (P3)

```
C→S  ROOM_SEARCH|open:<keyword>
S→C  ROOM_SEARCH_RES|<id>:<name>:<cur>:<max>:<has_pw>:<is_open>:<topic>;<id>:...
```

서버에서 `name` 또는 `topic`에 keyword가 포함된 방을 반환한다.

```c
void handle_room_search(const char *type, const char *keyword,
                        ClientSession *sess) {
    /* type: "open" | "group" | "all" */
    /* keyword: 방 이름 또는 주제에서 검색 */
    for (int i = 0; i < MAX_ROOMS; i++) {
        RoomInfo *r = &g_rooms[i];
        if (!r->active) continue;
        if (strcmp(type, "open")  == 0 && !r->is_open) continue;
        if (strcmp(type, "group") == 0 &&  r->is_open) continue;
        if (!strstr(r->name, keyword) && !strstr(r->topic, keyword)) continue;
        /* 결과에 추가 */
    }
}
```

---

## FR-O04 — 자유 참여

목록 또는 검색 결과에서 번호 선택 후 입장한다. 비밀번호가 설정된 방은 입력 화면이 표시된다.

```
C→S  ROOM_JOIN|<room_id>              (비밀번호 없는 방)
C→S  ROOM_JOIN|<room_id>:<password>   (비밀번호 있는 방)
S→C  ROOM_JOIN_RES|1:<room_id>:<room_name>  (성공)
     ROOM_JOIN_RES|0:<room_id>:             (실패)
```

### 비밀번호 입력 화면

```
> 이 방은 비밀번호가 필요합니다.
> Enter Password: ****
```

---

## FR-O05 — 오픈채팅 닉네임 (→ FR-C07 참조)

오픈채팅방 입장 시 별도 닉네임을 설정할 수 있다. `room_members.txt`의 `open_nick` 필드에 저장된다.

```
C→S  ROOM_SET_OPEN_NICK|<room_id>:<nick>
S→C  ROOM_SET_OPEN_NICK_RES|0   (성공)
                             1   (실패 — 금지 문자, 길이 초과 등)
```

- 오픈채팅 닉네임 미설정 시: 기본 닉네임 사용
- `ROOM_MSG_RECV`의 `from_nick` 필드에서 `open_nick` 우선 적용
- `ROOM_HISTORY_RES`에서도 `open_nick` 우선 적용

입장 시 오픈채팅 닉네임 설정 흐름:
```
[오픈채팅방 목록] → 번호 선택
    |
    v
> 오픈채팅 닉네임을 설정하시겠습니까? (Enter = 기본 닉네임 사용)
> 닉네임 입력: 익명사용자

    | ROOM_JOIN|5
    | ROOM_SET_OPEN_NICK|5:익명사용자
    |
    v
[채팅방 입장]
```
