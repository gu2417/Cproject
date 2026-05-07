# 모듈: friend.c/h — 친구 관리

## 1. 책임

- 친구 추가 요청 전송 및 실시간 알림
- 친구 요청 수락/거절
- 친구 삭제 및 차단
- 친구 목록 조회 (온라인 상태 포함)
- 유저 ID/닉네임 검색
- friends.txt 파일 I/O

---

## 2. 함수 목록

```c
/* friend.h */

/* 친구 추가 요청 전송 */
void handle_friend_add(const char *from_id, const char *target_id,
                       ClientSession *sess);

/* 친구 요청 수락 */
void handle_friend_accept(const char *user_id, const char *from_id,
                          ClientSession *sess);

/* 친구 요청 거절 */
void handle_friend_reject(const char *user_id, const char *from_id,
                          ClientSession *sess);

/* 친구 삭제 */
void handle_friend_delete(const char *user_id, const char *target_id,
                          ClientSession *sess);

/* 친구 차단 */
void handle_friend_block(const char *user_id, const char *target_id,
                         ClientSession *sess);

/* 친구 목록 요청 */
void handle_friend_list_req(const char *user_id, ClientSession *sess);

/* 유저 검색 (ID 또는 닉네임 키워드) */
void handle_user_search(const char *keyword, ClientSession *sess);
```

---

## 3. handle_friend_add 상세

```c
void handle_friend_add(const char *from_id, const char *target_id,
                       ClientSession *sess) {
    /* 1. 대상 유저 존재 확인 */
    UserRecord *target = find_user_by_id(target_id);
    if (!target) {
        send_packet(sess->fd, "FRIEND_ADD_RES|1");  /* NOT_FOUND */
        return;
    }

    /* 2. 자기 자신에게 친구 요청 금지 */
    if (strcmp(from_id, target_id) == 0) {
        send_packet(sess->fd, "FRIEND_ADD_RES|1");  /* NOT_FOUND 위장 */
        return;
    }

    /* 3. 차단 검사 — 양방향 분리 (FR_F_friend.md 차단 매트릭스)
       3-a. target이 from을 차단했는가? (TGT→BLK 방향) → BLOCKED
            (피차단자가 차단자에게 친구요청 보낼 수 없음) */
    if (is_blocked_by(target_id, from_id)) {
        send_packet(sess->fd, "FRIEND_ADD_RES|2");  /* BLOCKED */
        return;
    }

    /* 3-b. from이 target을 차단한 상태에서 친구요청 시도 → BLOCKED
            (자기 차단 목록에 있는 사람에게 요청 불가 — 먼저 차단 해제 필요) */
    FriendRecord *self_to_target = find_friend_record(from_id, target_id);
    if (self_to_target && self_to_target->status == 2) {
        send_packet(sess->fd, "FRIEND_ADD_RES|2");  /* BLOCKED */
        return;
    }

    /* 4. 이미 친구/요청 대기 중인지 검사 (양방향 accepted/pending 모두 차단) */
    FriendRecord *forward  = find_friend_record(from_id, target_id);
    FriendRecord *backward = find_friend_record(target_id, from_id);
    if ((forward  && (forward->status  == 0 || forward->status  == 1)) ||
        (backward && (backward->status == 0 || backward->status == 1))) {
        send_packet(sess->fd, "FRIEND_ADD_RES|3");  /* ALREADY_FRIEND */
        return;
    }

    /* 5. friends.txt에 pending 레코드 추가 */
    FriendRecord fr = {0};
    fr.id = g_next_friend_id++;
    strncpy(fr.user_id,   from_id,   20);
    strncpy(fr.friend_id, target_id, 20);
    fr.status = 0;  /* pending */
    get_current_timestamp(fr.created_at);

    WaitForSingleObject(g_file_mutex, INFINITE);
    append_friend(FILE_FRIENDS, &fr);
    ReleaseMutex(g_file_mutex);

    add_friend_to_cache(&fr);

    /* 6. 요청 송신자에게 SENT 응답 */
    send_packet(sess->fd, "FRIEND_ADD_RES|0");  /* SENT */

    /* 7. 대상이 온라인이면 실시간 알림 */
    ClientSession *target_sess = find_session_by_id(target_id);
    if (target_sess) {
        char from_nick[21] = {0};
        get_nickname(from_id, from_nick);
        send_packet(target_sess->fd,
                    "FRIEND_REQUEST_NOTIFY|%s:%s", from_id, from_nick);
    }
}

/* 차단 검사 헬퍼 — receiver가 sender를 차단했는지 */
int is_blocked_by(const char *receiver_id, const char *sender_id) {
    for (int i = 0; i < g_friend_count; i++) {
        FriendRecord *fr = &g_friends[i];
        if (strcmp(fr->user_id,   receiver_id) == 0 &&
            strcmp(fr->friend_id, sender_id)   == 0 &&
            fr->status == 2) {
            return 1;
        }
    }
    return 0;
}
```

---

## 4. handle_friend_accept 상세

```c
void handle_friend_accept(const char *user_id, const char *from_id,
                          ClientSession *sess) {
    /* 1. pending 레코드 찾기 */
    FriendRecord *fr = find_friend_pending(from_id, user_id);
    if (!fr) return;

    /* 2. status → accepted */
    fr->status = 1;
    WaitForSingleObject(g_file_mutex, INFINITE);
    save_friends(FILE_FRIENDS, g_friends, g_friend_count);
    ReleaseMutex(g_file_mutex);

    /* 3. 요청 송신자(from_id)에게 수락 알림 */
    ClientSession *from_sess = find_session_by_id(from_id);
    if (from_sess) {
        char user_nick[21] = {0};
        get_nickname(user_id, user_nick);
        send_packet(from_sess->fd,
                    "FRIEND_ACCEPT_NOTIFY|%s:%s", user_id, user_nick);
    }
}
```

---

## 5. handle_friend_list_req 상세

```c
void handle_friend_list_req(const char *user_id, ClientSession *sess) {
    /* friends.txt에서 user_id가 관계된 accepted 레코드 수집 */
    FriendEntry list[MAX_CLIENTS];
    int count = 0;

    for (int i = 0; i < g_friend_count; i++) {
        FriendRecord *fr = &g_friends[i];
        if (fr->status != 1) continue;  /* accepted만 */

        const char *other_id = NULL;
        if (strcmp(fr->user_id, user_id) == 0)
            other_id = fr->friend_id;
        else if (strcmp(fr->friend_id, user_id) == 0)
            other_id = fr->user_id;
        else
            continue;

        UserRecord *u = find_user_by_id(other_id);
        if (!u) continue;

        /* invisible 상태는 offline으로 표시 */
        int display_status = (u->online_status == 3) ? 0 : u->online_status;

        strncpy(list[count].id,         u->id,         20);
        strncpy(list[count].nick,       u->nickname,   20);
        strncpy(list[count].status_msg, u->status_msg, 100);
        list[count].status = display_status;
        count++;
    }

    /* FRIEND_LIST_RES|<count>:<id>:<nick>:<status>:<status_msg>;... */
    char buf[MAX_BUF_SIZE];
    int  off = 0;
    off += snprintf(buf + off, sizeof(buf) - off,
                    "FRIEND_LIST_RES|%d", count);
    for (int i = 0; i < count; i++) {
        off += snprintf(buf + off, sizeof(buf) - off,
                        ":%s:%s:%d:%s",
                        list[i].id, list[i].nick,
                        list[i].status, list[i].status_msg);
        if (i < count - 1)
            buf[off++] = ';';
    }
    buf[off++] = '\n';
    buf[off]   = '\0';
    send(sess->fd, buf, off, 0);
}
```

---

## 6. 실시간 상태 알림

로그인/로그아웃/상태 변경 시 해당 유저의 친구 목록을 순회하며 온라인 상태를 전달한다.

```c
/* broadcast.c에서 호출 */
void notify_friend_status_change(const char *user_id, int new_status) {
    char nick[21] = {0};
    get_nickname(user_id, nick);

    /* invisible 설정자 → 친구에게는 offline(0)으로 전송 */
    int display_status = (new_status == 3) ? 0 : new_status;

    for (int i = 0; i < g_friend_count; i++) {
        FriendRecord *fr = &g_friends[i];
        if (fr->status != 1) continue;

        const char *other_id = NULL;
        if (strcmp(fr->user_id, user_id) == 0)
            other_id = fr->friend_id;
        else if (strcmp(fr->friend_id, user_id) == 0)
            other_id = fr->user_id;
        else
            continue;

        ClientSession *other_sess = find_session_by_id(other_id);
        if (other_sess) {
            send_packet(other_sess->fd,
                        "FRIEND_STATUS_CHANGE|%s:%s:%d",
                        user_id, nick, display_status);
        }
    }
}
```

---

## 7. friends.txt status 코드

| 값 | 의미 |
|----|------|
| 0 | pending (요청 대기 중) |
| 1 | accepted (친구 관계 성립) |
| 2 | blocked (차단) |

---

## 8. 응답 패킷 요약

| 패킷 | 코드 | 의미 |
|------|------|------|
| `FRIEND_ADD_RES\|0` | 0 | 요청 전송 성공 (SENT) |
| `FRIEND_ADD_RES\|1` | 1 | 대상 유저 없음 |
| `FRIEND_ADD_RES\|2` | 2 | 차단된 유저 |
| `FRIEND_ADD_RES\|3` | 3 | 이미 친구 |
| `FRIEND_REQUEST_NOTIFY\|<from_id>:<from_nick>` | — | 친구 요청 수신 알림 |
| `FRIEND_ACCEPT_NOTIFY\|<user_id>:<nick>` | — | 수락 알림 |
| `FRIEND_LIST_RES\|<count>:...` | — | 친구 목록 응답 |
| `FRIEND_STATUS_CHANGE\|<id>:<nick>:<status>` | — | 실시간 상태 변경 알림 |
