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

/* 외부 헬퍼 (다른 모듈) */
extern ClientSession *find_session_by_id  (const char *user_id);
extern ClientSession *find_session_by_nick(const char *nickname);
extern int            is_blocked_by(const char *receiver_id,
                                    const char *sender_id);   /* friend.h */
extern const char    *resolve_display_nick(int room_id, const char *user_id,
                                           int is_open);       /* room.h */
```

---

## 3. handle_msg_delete 상세

```c
void handle_msg_delete(const char *user_id, int room_id, int msg_id,
                       ClientSession *sess) {
    MessageRecord *m = find_message_by_id(msg_id);
    if (!m || m->is_deleted) return;
    if (m->room_id != room_id) return;
    if (m->msg_type == MSG_TYPE_SYSTEM) return;  /* 시스템 메시지 보호 */

    int is_self = strcmp(m->from_id, user_id) == 0;
    int is_dm   = (m->room_id == 0);

    /* 권한 정책 (FR_M_message.md FR-M02 매트릭스):
       - DM: 송신자 본인만 삭제 가능 (수신자는 불가)
       - 그룹/오픈채팅: 본인 또는 방장/공동방장 */
    if (is_dm) {
        if (!is_self) return;
    } else {
        if (!is_self && !is_room_admin(room_id, user_id)) return;
    }

    m->is_deleted = 1;

    WaitForSingleObject(g_file_mutex, INFINITE);
    update_message_deleted(FILE_MESSAGES, msg_id);
    ReleaseMutex(g_file_mutex);

    if (is_dm) {
        /* DM: 두 당사자에게만 알림 — 다른 세션 노출 금지 */
        ClientSession *peer = find_session_by_id(
            is_self ? m->to_id : m->from_id);
        if (peer)
            send_packet(peer->fd, "MSG_DELETED_NOTIFY|0:%d", msg_id);
        send_packet(sess->fd, "MSG_DELETED_NOTIFY|0:%d", msg_id);
    } else {
        broadcast_to_room(room_id,
            make_packet("MSG_DELETED_NOTIFY|%d:%d", room_id, msg_id));
    }
}
```

---

## 4. handle_msg_edit 상세

```c
void handle_msg_edit(const char *user_id, int room_id, int msg_id,
                     const char *new_content, ClientSession *sess) {
    MessageRecord *m = find_message_by_id(msg_id);
    if (!m || m->is_deleted) return;
    if (m->room_id != room_id) return;

    /* 시스템 메시지 보호 — 누구도 수정 불가 (FR_M_message.md FR-M03 매트릭스) */
    if (m->msg_type == MSG_TYPE_SYSTEM) return;

    /* 본인 메시지만 — 방장도 타인 메시지 수정 불가 */
    if (strcmp(m->from_id, user_id) != 0) {
        send_packet(sess->fd,
            "NOTIFY|SERVER:타인의 메시지는 수정할 수 없습니다.");
        return;
    }

    /* 5분(300초) 이내 검사 */
    time_t now      = time(NULL);
    time_t msg_time = parse_timestamp(m->created_at);
    if (difftime(now, msg_time) > 300.0) {
        send_packet(sess->fd, "NOTIFY|SERVER:수정 가능 시간(5분)이 초과되었습니다.");
        return;
    }

    /* 입력 검증 */
    if (has_forbidden_chars(new_content)) {
        send_packet(sess->fd, "NOTIFY|SERVER:사용할 수 없는 문자가 포함되어 있습니다.");
        return;
    }

    /* 이모티콘 변환 후 수정 내용 저장 */
    char converted[501];
    strncpy(converted, new_content, 500);
    converted[500] = '\0';
    convert_emoticons(converted, sizeof(converted));

    strncpy(m->content, converted, 500);
    m->content[500] = '\0';
    get_current_timestamp(m->edited_at);

    WaitForSingleObject(g_file_mutex, INFINITE);
    update_message_content(FILE_MESSAGES, msg_id,
                           converted, m->edited_at);
    ReleaseMutex(g_file_mutex);

    /* "(수정됨)" 접미사는 클라이언트 표시 단계에서 붙임 — 서버는 raw content만 송신
       (수신 측에서 m.edited_at[0] != '\0' 검사로 (수정됨) 표시) */
    if (room_id == 0) {
        /* DM 수정: 두 당사자에게만 알림 */
        ClientSession *peer = find_session_by_id(m->to_id);
        if (peer)
            send_packet(peer->fd, "MSG_EDITED_NOTIFY|0:%d:%s",
                        msg_id, converted);
        send_packet(sess->fd, "MSG_EDITED_NOTIFY|0:%d:%s",
                    msg_id, converted);
    } else {
        broadcast_to_room(room_id,
            make_packet("MSG_EDITED_NOTIFY|%d:%d:%s",
                        room_id, msg_id, converted));
    }
}
```

---

## 5. handle_whisper 상세 (FR-M01)

귓속말은 닉네임 기반으로 대상 세션을 검색해 단건 전달한다. 차단 검사는 DM과 동일한 정책 (피수신자 측에서 차단했으면 무수신).

```c
void handle_whisper(const char *from_nick, const char *to_nick,
                    const char *content, ClientSession *sess) {
    /* 1. 닉네임으로 수신자 세션 찾기 (닉네임은 unique) */
    ClientSession *to_sess = find_session_by_nick(to_nick);
    if (!to_sess) {
        send_packet(sess->fd,
            "NOTIFY|SERVER:%s 님은 현재 오프라인입니다.", to_nick);
        return;
    }

    /* 2. 자기 자신 검사 */
    if (strcmp(sess->user_id, to_sess->user_id) == 0) {
        send_packet(sess->fd, "NOTIFY|SERVER:자기 자신에게 귓속말할 수 없습니다.");
        return;
    }

    /* 3. 차단 검사 (FR_F_friend.md 차단 매트릭스):
          target이 sender를 차단했으면 조용히 무시 (차단 노출 방지) */
    if (is_blocked_by(to_sess->user_id, sess->user_id)) {
        return;
    }

    /* 4. 이모티콘 변환 */
    char converted[501];
    strncpy(converted, content, 500);
    converted[500] = '\0';
    convert_emoticons(converted, sizeof(converted));

    /* 5. 타임스탬프 + 전송 */
    char ts[20];
    get_current_timestamp(ts);
    send_packet(to_sess->fd, "WHISPER_RECV|%s:%s:%s",
                from_nick, ts, converted);

    /* 6. messages.txt 저장은 정책상 선택 (FR-M01 노트: 저장 선택사항)
          저장하면 msg_type=2로 to_id 채워서 기록 */
}
```

---

## 6. 이모티콘 변환 테이블

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

## 7. /me 액션 메시지 처리

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

## 8. 시스템 메시지 (msg_type=1)

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

## 9. 메시지 검색

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

    /* MSG_SEARCH_RES|<count>:<msg_id>:<from_nick>:<timestamp>:<content>;...
       오픈채팅 시 from_nick은 open_nick 우선 (module_room.md resolve_display_nick) */
    RoomInfo *r = find_room_by_id(room_id);
    int is_open = (r && r->is_open) ? 1 : 0;

    char buf[MAX_BUF_SIZE];
    int  off = 0;
    off += snprintf(buf + off, sizeof(buf) - off,
                    "MSG_SEARCH_RES|%d", found);
    for (int i = 0; i < found; i++) {
        const char *nick = resolve_display_nick(room_id,
                                                results[i].from_id, is_open);
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

## 10. msg_type 코드표

| 값 | 의미 | 클라이언트 표시 |
|----|------|----------------|
| 0 | 일반 메시지 | `[HH:MM] 닉네임: 내용` |
| 1 | 시스템 메시지 | `[시스템] 내용` (회색/다른 색) |
| 2 | 귓속말 | `[귓속말] 닉네임: 내용` |
| 3 | /me 액션 | `* 닉네임 동작` |
