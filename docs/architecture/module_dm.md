# 모듈: dm.c/h — 1:1 DM (다이렉트 메시지)

## 1. 책임

- DM 전송 (messages.txt에 room_id=0으로 저장)
- DM 수신 알림 (수신자가 온라인이면 즉시 전달)
- 읽음 처리 (dm_reads.txt 저장, DM_READ_NOTIFY 전송)
- DM 히스토리 조회 (최근 N개, messages.txt 필터링)
- DM 목록 조회 (최근 대화 상대 목록, P1)

---

## 2. 함수 목록

```c
/* dm.h */

/* DM 전송 */
void handle_dm_send(const char *from_id, const char *to_id,
                    const char *content, ClientSession *sess);

/* DM 히스토리 요청 */
void handle_dm_history_req(const char *user_id, const char *with_id,
                           int count, ClientSession *sess);

/* DM 목록 요청 (P1, FR-P04) */
void handle_dm_list_req(const char *user_id, ClientSession *sess);

/* 읽음 처리 — 수신자가 DM을 열람할 때 호출 */
void handle_dm_read(const char *reader_id, int msg_id,
                    ClientSession *sess);

/* 내부 헬퍼 */
int  save_dm_message(const char *from_id, const char *to_id,
                     const char *content);
int  get_unread_dm_count(const char *user_id, const char *with_id);
```

---

## 3. handle_dm_send 상세

```c
void handle_dm_send(const char *from_id, const char *to_id,
                    const char *content, ClientSession *sess) {
    /* 1. 수신자 존재 확인 */
    UserRecord *target = find_user_by_id(to_id);
    if (!target) return;

    /* 2. 차단 여부 확인 */
    FriendRecord *fr = find_friend_record(to_id, from_id);
    if (fr && fr->status == 2) return;  /* 차단됨 */

    /* 3. messages.txt에 저장 (room_id=0 = DM) */
    MessageRecord m = {0};
    m.id       = g_next_msg_id++;
    m.room_id  = 0;                    /* DM 식별자 */
    m.msg_type = 0;
    strncpy(m.from_id,  from_id, 20);
    strncpy(m.to_id,    to_id,   20);
    strncpy(m.content,  content, 500);
    get_current_timestamp(m.created_at);

    WaitForSingleObject(g_file_mutex, INFINITE);
    append_message(FILE_MESSAGES, &m);
    ReleaseMutex(g_file_mutex);

    /* 4. 수신자가 온라인이면 즉시 DM_RECV 전송 */
    ClientSession *to_sess = find_session_by_id(to_id);
    if (to_sess) {
        char from_nick[21] = {0};
        get_nickname(from_id, from_nick);
        send_packet(to_sess->fd,
                    "DM_RECV|%s:%s:%s:%d:%s",
                    from_id, from_nick,
                    m.created_at, m.id, content);
    }
}
```

---

## 4. handle_dm_history_req 상세

```c
void handle_dm_history_req(const char *user_id, const char *with_id,
                           int count, ClientSession *sess) {
    /* messages.txt에서 room_id=0, (from_id=user_id AND to_id=with_id)
       OR (from_id=with_id AND to_id=user_id) 조건으로 필터링 */

    MessageRecord results[100];
    int found = 0;
    int limit = (count > 0 && count <= 100) ? count : 50;

    /* 인메모리 메시지 캐시 역순 탐색 (최신 N개) */
    for (int i = g_msg_count - 1; i >= 0 && found < limit; i--) {
        MessageRecord *m = &g_messages[i];
        if (m->room_id != 0) continue;
        int match =
            (strcmp(m->from_id, user_id) == 0 &&
             strcmp(m->to_id,   with_id) == 0) ||
            (strcmp(m->from_id, with_id) == 0 &&
             strcmp(m->to_id,   user_id) == 0);
        if (match)
            results[found++] = *m;
    }

    /* 결과를 시간 오름차순으로 뒤집기 */
    for (int i = 0; i < found / 2; i++) {
        MessageRecord tmp = results[i];
        results[i] = results[found - 1 - i];
        results[found - 1 - i] = tmp;
    }

    /* DM_HISTORY_RES|<count>:<msg_id>:<from_id>:<timestamp>:<read>:<content>;... */
    char buf[MAX_BUF_SIZE];
    int  off = 0;
    off += snprintf(buf + off, sizeof(buf) - off,
                    "DM_HISTORY_RES|%d", found);
    for (int i = 0; i < found; i++) {
        MessageRecord *m = &results[i];
        int read = is_dm_read(g_dm_reads, g_dm_read_count,
                              m->id, with_id);
        off += snprintf(buf + off, sizeof(buf) - off,
                        ":%d:%s:%s:%d:%s",
                        m->id, m->from_id, m->created_at,
                        read, m->content);
        if (i < found - 1)
            buf[off++] = ';';
    }
    buf[off++] = '\n';
    buf[off]   = '\0';
    send(sess->fd, buf, off, 0);
}
```

---

## 5. 읽음 처리 흐름

```
[클라이언트 — DM 화면 열람]
        |
        | (DM 수신 또는 화면 진입 시 자동)
        v
handle_dm_read(reader_id, msg_id)
        |
        +-- dm_reads.txt에 레코드 추가
        |   (msg_id // reader_id // read_at)
        |
        +-- 원래 송신자(from_id)가 온라인이면
            DM_READ_NOTIFY|<reader_id> 전송
```

```c
void handle_dm_read(const char *reader_id, int msg_id,
                    ClientSession *sess) {
    /* 이미 읽음 처리된 경우 중복 방지 */
    if (is_dm_read(g_dm_reads, g_dm_read_count, msg_id, reader_id))
        return;

    DmReadRecord dr = {0};
    dr.msg_id = msg_id;
    strncpy(dr.reader_id, reader_id, 20);
    get_current_timestamp(dr.read_at);

    WaitForSingleObject(g_file_mutex, INFINITE);
    append_dm_read(FILE_DM_READS, &dr);
    ReleaseMutex(g_file_mutex);

    add_dm_read_to_cache(&dr);

    /* 송신자에게 읽음 알림 */
    MessageRecord *m = find_message_by_id(msg_id);
    if (m) {
        ClientSession *from_sess = find_session_by_id(m->from_id);
        if (from_sess) {
            send_packet(from_sess->fd,
                        "DM_READ_NOTIFY|%s", reader_id);
        }
    }
}
```

---

## 6. DM vs 그룹 메시지 구분

messages.txt에서 DM과 그룹 메시지를 구분하는 기준:

| 조건 | 유형 |
|------|------|
| `room_id == 0` AND `to_id` 있음 | DM |
| `room_id > 0` | 그룹/오픈채팅 메시지 |

---

## 7. 응답 패킷 요약

| 패킷 | 의미 |
|------|------|
| `DM_RECV\|<from_id>:<nick>:<ts>:<msg_id>:<content>` | DM 수신 알림 |
| `DM_READ_NOTIFY\|<reader_id>` | 읽음 알림 → 송신자에게 |
| `DM_HISTORY_RES\|<count>:...` | DM 히스토리 응답 |
| `DM_LIST_RES\|<count>:...` | DM 목록 응답 (P1) |
