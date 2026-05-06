# 모듈: broadcast.c/h — 브로드캐스트 및 알림 전송

## 1. 책임

- 특정 채팅방 멤버 전체에게 패킷 전송
- 전체 접속 클라이언트에게 패킷 전송 (P0 브로드캐스트)
- 특정 user_id를 가진 세션에게 패킷 전송
- 친구 상태 변경 알림 전송
- 모든 전송 함수는 g_sessions_mutex 보호 하에 실행

---

## 2. 함수 목록

```c
/* broadcast.h */

/* 특정 채팅방 멤버 전체에게 전송 */
void broadcast_to_room(int room_id, const char *packet);

/* 전체 접속 클라이언트에게 전송 (P0) */
void broadcast_to_all(const char *packet);

/* 특정 user_id 세션에게 전송 */
void send_to_user(const char *user_id, const char *packet);

/* 친구들에게 온라인 상태 변경 알림 */
void notify_friend_status_change(const char *user_id, int new_status);

/* 패킷 문자열 생성 헬퍼 */
const char *make_packet(const char *fmt, ...);
```

---

## 3. broadcast_to_room 상세

```c
void broadcast_to_room(int room_id, const char *packet) {
    RoomInfo *r = find_room_by_id(room_id);
    if (!r) return;

    int  pkt_len = (int)strlen(packet);
    WaitForSingleObject(g_sessions_mutex, INFINITE);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!g_sessions[i].active) continue;
        /* 해당 방의 멤버인지 확인 */
        for (int j = 0; j < r->member_count; j++) {
            if (strcmp(r->member_ids[j], g_sessions[i].user_id) == 0) {
                send(g_sessions[i].fd, packet, pkt_len, 0);
                break;
            }
        }
    }

    ReleaseMutex(g_sessions_mutex);
}
```

---

## 4. broadcast_to_all 상세 (P0)

P0에서는 방별 격리 없이 전체 접속자에게 전송한다.  
클라이언트가 수신 메시지의 `room_id`를 확인해 현재 방 메시지만 화면에 표시한다.

```c
void broadcast_to_all(const char *packet) {
    int pkt_len = (int)strlen(packet);
    WaitForSingleObject(g_sessions_mutex, INFINITE);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_sessions[i].active)
            send(g_sessions[i].fd, packet, pkt_len, 0);
    }

    ReleaseMutex(g_sessions_mutex);
}
```

---

## 5. send_to_user 상세

```c
void send_to_user(const char *user_id, const char *packet) {
    int pkt_len = (int)strlen(packet);
    WaitForSingleObject(g_sessions_mutex, INFINITE);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_sessions[i].active &&
            strcmp(g_sessions[i].user_id, user_id) == 0) {
            send(g_sessions[i].fd, packet, pkt_len, 0);
            break;
        }
    }

    ReleaseMutex(g_sessions_mutex);
}
```

---

## 6. notify_friend_status_change 상세

```c
void notify_friend_status_change(const char *user_id, int new_status) {
    char nick[21] = {0};
    get_nickname(user_id, nick);

    /* invisible(3) 설정자 → 친구에게는 offline(0)으로 전송 */
    int display_status = (new_status == 3) ? 0 : new_status;

    char packet[256];
    snprintf(packet, sizeof(packet),
             "FRIEND_STATUS_CHANGE|%s:%s:%d\n",
             user_id, nick, display_status);

    /* 해당 유저의 모든 accepted 친구에게 전송 */
    for (int i = 0; i < g_friend_count; i++) {
        FriendRecord *fr = &g_friends[i];
        if (fr->status != 1) continue;

        const char *other_id = NULL;
        if (strcmp(fr->user_id,   user_id) == 0) other_id = fr->friend_id;
        else if (strcmp(fr->friend_id, user_id) == 0) other_id = fr->user_id;
        else continue;

        send_to_user(other_id, packet);
    }
}
```

---

## 7. make_packet 헬퍼

```c
/* 스레드 안전하지 않음 — 반환값을 즉시 사용하거나 복사해야 함 */
static char _pkt_buf[MAX_BUF_SIZE];

const char *make_packet(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(_pkt_buf, MAX_BUF_SIZE - 2, fmt, args);
    va_end(args);

    /* '\n' 보장 */
    if (len > 0 && _pkt_buf[len - 1] != '\n') {
        _pkt_buf[len++] = '\n';
        _pkt_buf[len]   = '\0';
    }
    return _pkt_buf;
}
```

> **주의**: `make_packet()`은 정적 버퍼를 사용하므로 반환값을 즉시 `send()`에 전달해야 한다. 연속 호출 시에는 별도 버퍼에 복사 후 사용한다.

---

## 8. send_packet 헬퍼 (단일 소켓)

```c
void send_packet(SOCKET fd, const char *fmt, ...) {
    char buf[MAX_BUF_SIZE];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, MAX_BUF_SIZE - 2, fmt, args);
    va_end(args);

    if (len > 0 && buf[len - 1] != '\n') {
        buf[len++] = '\n';
        buf[len]   = '\0';
    }
    send(fd, buf, len, 0);
}
```

---

## 9. Mutex 보호 패턴 요약

```
broadcast_to_room()
  └─ WaitForSingleObject(g_sessions_mutex)
       └─ g_sessions[] 순회 → send()
  └─ ReleaseMutex(g_sessions_mutex)

broadcast_to_all()
  └─ WaitForSingleObject(g_sessions_mutex)
       └─ 전체 active 세션 → send()
  └─ ReleaseMutex(g_sessions_mutex)

send_to_user()
  └─ WaitForSingleObject(g_sessions_mutex)
       └─ user_id 일치 세션 → send()
  └─ ReleaseMutex(g_sessions_mutex)
```

> **중요**: Mutex 보유 중에 `send()`를 호출한다. 네트워크 지연으로 인한 교착 가능성을 방지하기 위해 서버 소켓은 비블로킹 모드(`ioctlsocket`)나 짧은 `SO_SNDTIMEO` 타임아웃 설정을 권장한다.

---

## 10. DND 처리

`broadcast_to_room()` 또는 `send_to_user()`에서 DND 모드 필터링은 **클라이언트 측**에서 처리한다. 서버는 DND 여부와 무관하게 패킷을 전송하되, 멘션(`@닉네임`) 메시지는 DND에서도 클라이언트가 강조 표시한다.
