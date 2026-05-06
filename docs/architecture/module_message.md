# 모듈: message.c/h — 메시지 부가기능

## 1. 책임

- 메시지 삭제 (is_deleted=1, 브로드캐스트)
- 메시지 수정 (5분=300초 이내, edited_at 갱신, 브로드캐스트)
- 답장 (reply_to_id 포함 ROOM_MSG_RECV 브로드캐스트)
- 귓속말 (특정 닉네임 대상, WHISPER_RECV)
- 메시지 검색 (키워드 매칭, MSG_SEARCH_RES)
- 핀 메시지 설정 (방장/관리자 전용, rooms.txt 갱신)
- 이모티콘 텍스트 변환
- /me 액션 메시지 처리 (msg_type=3)
- 시스템 메시지 저장/브로드캐스트 (msg_type=1)

---

## 2. 함수 목록

```c
/* message.h */

/* 메시지 삭제 */
void handle_msg_delete(const char *user_id, int room_id, int msg_id,
                       ClientSession *sess);

/* 메시지 수정 (5분 이내) */
void handle_msg_edit(const char *user_id, int room_id, int msg_id,
                     const char *new_content, ClientSession *sess);

/* 답장 */
void handle_msg_reply(int room_id, int reply_to_id,
                      const char *content, ClientSession *sess);

/* 귓속말 */
void handle_whisper(const char *from_nick, const char *to_nick,
                    const char *content, ClientSession *sess);

/* 메시지 검색 */
void handle_msg_search(int room_id, const char *keyword,
                       ClientSession *sess);

/* 핀 메시지 설정 */
void handle_msg_pin(int room_id, int msg_id, ClientSession *sess);

/* 이모티콘 변환 (in-place) */
void convert_emoticons(char *content, int max_len);

/* /me 액션 메시지 감지 및 변환 */
int  parse_me_action(const char *content, char *out, int out_len);

/* 시스템 메시지 브로드캐스트 */
void broadcast_system_msg(int room_id, const char *text);

/* 메시지 저장 헬퍼 */
int  save_room_message(int room_id, const char *from_id,
                       const char *content, int reply_to,
                       int msg_type, int *out_msg_id);
```

---

## 3. handle_msg_delete 상세

```c
void handle_msg_delete(const char *user_id, int room_id, int msg_id,
                       ClientSession *sess) {
    MessageRecord *m = find_message_by_id(msg_id);
    if (!m) return;

    /* 본인 메시지 또는 방장/관리자만 삭제 가능 */
    if (strcmp(m->from_id, user_id) != 0 &&
        !is_room_admin(room_id, user_id)) {
        return;
    }

    /* is_deleted = 1 */
    m->is_deleted = 1;

    WaitForSingleObject(g_file_mutex, INFINITE);
    update_message_deleted(FILE_MESSAGES, msg_id);
    ReleaseMutex(g_file_mutex);

    /* 방 전체에 삭제 알림 */
    broadcast_to_room(room_id,
        make_packet("MSG_DELETED_NOTIFY|%d:%d", room_id, msg_id));
}
```

---

## 4. handle_msg_edit 상세

```c
void handle_msg_edit(const char *user_id, int room_id, int msg_id,
                     const char *new_content, ClientSession *sess) {
    MessageRecord *m = find_message_by_id(msg_id);
    if (!m) return;

    /* 본인 메시지만 수정 가능 */
    if (strcmp(m->from_id, user_id) != 0) return;

    /* 5분(300초) 이내 검사 */
    time_t now      = time(NULL);
    time_t msg_time = parse_timestamp(m->created_at);
    if (difftime(now, msg_time) > 300.0) {
        send_packet(sess->fd, "NOTIFY|SERVER:수정 가능 시간(5분)이 초과되었습니다.");
        return;
    }

    /* 이모티콘 변환 후 수정 내용 저장 */
    char converted[501];
    strncpy(converted, new_content, 500);
    convert_emoticons(converted, sizeof(converted));

    strncpy(m->content, converted, 500);
    get_current_timestamp(m->edited_at);

    WaitForSingleObject(g_file_mutex, INFINITE);
    update_message_content(FILE_MESSAGES, msg_id,
                           converted, m->edited_at);
    ReleaseMutex(g_file_mutex);

    /* "(수정됨)" 접미사 붙여 브로드캐스트 */
    char display[512];
    snprintf(display, sizeof(display), "%s (수정됨)", converted);
    broadcast_to_room(room_id,
        make_packet("MSG_EDITED_NOTIFY|%d:%d:%s", room_id, msg_id, display));
}
```

---

## 5. 이모티콘 변환 테이블

```c
static const struct { const char *from; const char *to; } EMOTICON_TABLE[] = {
    { ":smile:",   "(^_^)"  },
    { ":heart:",   "<3"     },
    { ":sad:",     "(T_T)"  },
    { ":laugh:",   "(^o^)"  },
    { ":wink:",    "(^_-)"  },
    { ":angry:",   "(-_-)"  },
    { ":cool:",    "(B_B)"  },
    { ":shock:",   "(O_O)"  },
    { ":shy:",     "(>_<)"  },
    { ":sweat:",   "(^_^;)" },
    { ":lol:",     "(LoL)"  },
    { ":wave:",    "( ^_^)/" },
    { NULL, NULL }
};

void convert_emoticons(char *content, int max_len) {
    for (int i = 0; EMOTICON_TABLE[i].from != NULL; i++) {
        char *pos;
        while ((pos = strstr(content, EMOTICON_TABLE[i].from)) != NULL) {
            size_t from_len = strlen(EMOTICON_TABLE[i].from);
            size_t to_len   = strlen(EMOTICON_TABLE[i].to);
            size_t rest     = strlen(pos + from_len);
            /* 길이 초과 방지 */
            if ((pos - content) + to_len + rest + 1 > (size_t)max_len)
                break;
            memmove(pos + to_len, pos + from_len, rest + 1);
            memcpy(pos, EMOTICON_TABLE[i].to, to_len);
        }
    }
}
```

---

## 6. /me 액션 메시지 처리

```c
/* 입력: "/me 손을 흔든다"
   출력: "* 홍길동 손을 흔든다"
   반환값: 1 = me 액션, 0 = 일반 메시지 */
int parse_me_action(const char *content, char *out, int out_len) {
    if (strncmp(content, "/me ", 4) == 0) {
        snprintf(out, out_len, "* %s", content + 4);
        return 1;
    }
    strncpy(out, content, out_len - 1);
    out[out_len - 1] = '\0';
    return 0;
}
```

메시지 저장 시 `/me`가 감지되면 `msg_type=3`으로 저장하고, `ROOM_MSG_RECV` 브로드캐스트에 `msg_type=3`을 포함시킨다. 클라이언트는 `msg_type=3`이면 `* 닉네임 동작` 형식으로 표시한다.

---

## 7. 시스템 메시지 (msg_type=1)

입/퇴장, 초대, 강퇴, 공지 등 이벤트 발생 시 시스템 메시지를 방 전체에 브로드캐스트한다.

```c
void broadcast_system_msg(int room_id, const char *text) {
    char ts[20];
    get_current_timestamp(ts);

    /* messages.txt에 저장 */
    int msg_id;
    save_room_message(room_id, "SYSTEM", text, 0, 1, &msg_id);

    /* ROOM_MSG_RECV로 브로드캐스트 (msg_type=1) */
    /* 클라이언트는 msg_type=1이면 [시스템] 접두사로 표시 */
    broadcast_to_room(room_id,
        make_packet("ROOM_MSG_RECV|%d:SYSTEM:%s:%d:0:1:%s",
                    room_id, ts, msg_id, text));
}
```

---

## 8. 메시지 검색

```c
void handle_msg_search(int room_id, const char *keyword,
                       ClientSession *sess) {
    MessageRecord results[50];
    int found = 0;

    for (int i = 0; i < g_msg_count && found < 50; i++) {
        MessageRecord *m = &g_messages[i];
        if (m->room_id != room_id) continue;
        if (m->is_deleted) continue;
        if (strstr(m->content, keyword) != NULL)
            results[found++] = *m;
    }

    /* MSG_SEARCH_RES|<count>:<msg_id>:<from_nick>:<timestamp>:<content>;... */
    char buf[MAX_BUF_SIZE];
    int  off = 0;
    off += snprintf(buf + off, sizeof(buf) - off,
                    "MSG_SEARCH_RES|%d", found);
    for (int i = 0; i < found; i++) {
        char nick[21] = {0};
        get_nickname(results[i].from_id, nick);
        off += snprintf(buf + off, sizeof(buf) - off,
                        ":%d:%s:%s:%s",
                        results[i].id, nick,
                        results[i].created_at,
                        results[i].content);
        if (i < found - 1) buf[off++] = ';';
    }
    buf[off++] = '\n';
    buf[off]   = '\0';
    send(sess->fd, buf, off, 0);
}
```

---

## 9. msg_type 코드표

| 값 | 의미 | 클라이언트 표시 |
|----|------|----------------|
| 0 | 일반 메시지 | `[HH:MM] 닉네임: 내용` |
| 1 | 시스템 메시지 | `[시스템] 내용` (회색/다른 색) |
| 2 | 귓속말 | `[귓속말] 닉네임: 내용` |
| 3 | /me 액션 | `* 닉네임 동작` |
