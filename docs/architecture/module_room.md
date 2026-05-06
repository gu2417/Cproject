# 모듈: room.c/h — 채팅방 관리

## 1. 책임

- 채팅방 생성 (그룹/오픈채팅, rooms.txt 저장)
- 채팅방 입장 (비밀번호 검증, room_members.txt 저장)
- 채팅방 퇴장 (시스템 메시지 브로드캐스트)
- 멤버 초대 (room_invites.txt, 실시간 알림)
- 멤버 강퇴, 방 삭제 (방장/관리자 전용)
- 공지사항 등록, 핀 메시지 설정
- 공동 방장 부여/해제
- 멤버 목록 조회
- 인메모리 RoomInfo 배열과 파일 동기화

---

## 2. 함수 목록

```c
/* room.h */

void handle_room_create(const char *name, int max_users, int is_open,
                        const char *password, const char *topic,
                        ClientSession *sess);

void handle_room_list_req(const char *type, ClientSession *sess);

void handle_room_search(const char *type, const char *keyword,
                        ClientSession *sess);

void handle_room_join(int room_id, const char *password,
                      ClientSession *sess);

void handle_room_leave(int room_id, ClientSession *sess);

void handle_room_invite(int room_id, const char *target_id,
                        ClientSession *sess);

void handle_room_kick(int room_id, const char *target_id,
                      ClientSession *sess);

void handle_room_delete(int room_id, ClientSession *sess);

void handle_room_set_notice(int room_id, const char *notice,
                            ClientSession *sess);

void handle_room_grant_admin(int room_id, const char *target_id,
                             ClientSession *sess);

void handle_room_revoke_admin(int room_id, const char *target_id,
                              ClientSession *sess);

void handle_room_members_req(int room_id, ClientSession *sess);

void handle_room_msg(int room_id, const char *content,
                     ClientSession *sess);

/* 내부 헬퍼 */
RoomInfo *find_room_by_id(int room_id);
int       is_room_member(int room_id, const char *user_id);
int       is_room_admin(int room_id, const char *user_id);
void      add_member_to_room(int room_id, const char *user_id);
void      remove_member_from_room(int room_id, const char *user_id);
```

---

## 3. handle_room_create 상세

```c
void handle_room_create(const char *name, int max_users, int is_open,
                        const char *password, const char *topic,
                        ClientSession *sess) {
    /* 1. MAX_ROOMS 초과 검사 */
    if (g_room_count >= MAX_ROOMS) {
        send_packet(sess->fd, "ROOM_CREATE_RES|0:0");  /* FAIL */
        return;
    }

    /* 2. 비밀번호 해시 */
    char pw_hash[65] = {0};
    if (password && strlen(password) > 0)
        sha256_hex(password, pw_hash);

    /* 3. 방 ID 발급 */
    int new_id = g_next_room_id++;

    /* 4. 인메모리 RoomInfo 등록 */
    WaitForSingleObject(g_sessions_mutex, INFINITE);
    RoomInfo *r = NULL;
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (!g_rooms[i].active) {
            r = &g_rooms[i];
            break;
        }
    }
    if (!r) {
        ReleaseMutex(g_sessions_mutex);
        send_packet(sess->fd, "ROOM_CREATE_RES|0:0");
        return;
    }
    r->id        = new_id;
    r->max_users = max_users > 0 ? max_users : 10;
    r->is_open   = is_open;
    r->active    = 1;
    r->member_count = 0;
    strncpy(r->name,          name,          30);
    strncpy(r->password_hash, pw_hash,       64);
    strncpy(r->owner_id,      sess->user_id, 20);
    strncpy(r->topic,         topic ? topic : "", 100);
    g_room_count++;
    ReleaseMutex(g_sessions_mutex);

    /* 5. rooms.txt에 저장 */
    char created_at[20];
    get_current_timestamp(created_at);
    WaitForSingleObject(g_file_mutex, INFINITE);
    /* append to rooms.txt */
    FILE *fp = fopen(FILE_ROOMS, "a");
    if (fp) {
        fprintf(fp, "%d//%s//%s//%s//%d//%s/////%d//0//%s\n",
                new_id, name, topic ? topic : "", pw_hash,
                r->max_users, sess->user_id, is_open, created_at);
        fclose(fp);
    }
    ReleaseMutex(g_file_mutex);

    /* 6. 방장을 첫 번째 멤버로 추가 */
    add_member_to_room(new_id, sess->user_id);

    send_packet(sess->fd, "ROOM_CREATE_RES|1:%d", new_id);
}
```

---

## 4. handle_room_join 상세

```c
void handle_room_join(int room_id, const char *password,
                      ClientSession *sess) {
    RoomInfo *r = find_room_by_id(room_id);
    if (!r) {
        send_packet(sess->fd, "ROOM_JOIN_RES|0:%d:", room_id);
        return;
    }

    /* 인원 초과 검사 */
    if (r->member_count >= r->max_users) {
        send_packet(sess->fd, "ROOM_JOIN_RES|0:%d:", room_id);
        return;
    }

    /* 비밀번호 검증 */
    if (strlen(r->password_hash) > 0) {
        char pw_hash[65] = {0};
        if (!password || strlen(password) == 0) {
            send_packet(sess->fd, "ROOM_JOIN_RES|0:%d:", room_id);
            return;
        }
        sha256_hex(password, pw_hash);
        if (strcmp(r->password_hash, pw_hash) != 0) {
            send_packet(sess->fd, "ROOM_JOIN_RES|0:%d:", room_id);
            return;
        }
    }

    /* 멤버 추가 */
    if (!is_room_member(room_id, sess->user_id))
        add_member_to_room(room_id, sess->user_id);

    sess->current_room_id = room_id;

    /* 입장 성공 응답 */
    send_packet(sess->fd, "ROOM_JOIN_RES|1:%d:%s", room_id, r->name);

    /* 공지사항 전송 */
    if (strlen(r->notice) > 0)
        send_packet(sess->fd, "ROOM_NOTICE|%d:%s", room_id, r->notice);

    /* 핀 메시지 전송 */
    if (r->pinned_msg_id > 0) {
        MessageRecord *pm = find_message_by_id(r->pinned_msg_id);
        if (pm) {
            send_packet(sess->fd, "ROOM_PIN|%d:%d:%s:%s",
                        room_id, pm->id, pm->from_id, pm->content);
        }
    }

    /* 입장 시스템 메시지 브로드캐스트 */
    char sys_msg[128];
    snprintf(sys_msg, sizeof(sys_msg),
             "%s 님이 입장했습니다.", sess->nickname);
    broadcast_system_msg(room_id, sys_msg);
}
```

---

## 5. handle_room_leave 상세

```c
void handle_room_leave(int room_id, ClientSession *sess) {
    /* 퇴장 시스템 메시지 브로드캐스트 */
    char sys_msg[128];
    snprintf(sys_msg, sizeof(sys_msg),
             "%s 님이 퇴장했습니다.", sess->nickname);
    broadcast_system_msg(room_id, sys_msg);

    /* 멤버 배열에서 제거 */
    remove_member_from_room(room_id, sess->user_id);
    sess->current_room_id = 0;

    /* room_members.txt 갱신 */
    WaitForSingleObject(g_file_mutex, INFINITE);
    remove_room_member(FILE_ROOM_MEMBERS, g_room_members,
                       &g_room_member_count, room_id, sess->user_id);
    ReleaseMutex(g_file_mutex);

    /* 마지막 멤버가 나가면 방 삭제 */
    RoomInfo *r = find_room_by_id(room_id);
    if (r && r->member_count == 0) {
        r->active = 0;
        g_room_count--;
        /* rooms.txt 갱신 */
        WaitForSingleObject(g_file_mutex, INFINITE);
        save_rooms(FILE_ROOMS, g_rooms, MAX_ROOMS);
        ReleaseMutex(g_file_mutex);
    }
    /* ROOM_LEAVE 응답 패킷 없음 — 항상 성공 */
}
```

---

## 6. 방장 권한 검사

```c
int is_room_admin(int room_id, const char *user_id) {
    RoomInfo *r = find_room_by_id(room_id);
    if (!r) return 0;

    /* 방장인 경우 */
    if (strcmp(r->owner_id, user_id) == 0) return 1;

    /* 공동 방장인 경우 */
    for (int i = 0; i < r->member_count; i++) {
        if (strcmp(r->member_ids[i], user_id) == 0)
            return r->admin_flags[i];
    }
    return 0;
}
```

---

## 7. 인메모리 RoomInfo ↔ 파일 동기화 전략

| 연산 | 인메모리 | 파일 처리 |
|------|----------|-----------|
| 방 생성 | g_rooms[]에 추가 | rooms.txt에 `fopen("a")` append |
| 방 삭제 | active=0 | rooms.txt 전체 재작성 |
| 공지 변경 | r->notice 갱신 | rooms.txt 전체 재작성 |
| 핀 변경 | r->pinned_msg_id 갱신 | rooms.txt 전체 재작성 |
| 멤버 입장 | member_ids[] 추가 | room_members.txt append |
| 멤버 퇴장 | member_ids[] 제거 | room_members.txt 전체 재작성 |
| 관리자 부여 | admin_flags[] 갱신 | room_members.txt 전체 재작성 |

---

## 8. 응답 패킷 요약

| 패킷 | 의미 |
|------|------|
| `ROOM_CREATE_RES\|1:<room_id>` | 방 생성 성공 |
| `ROOM_CREATE_RES\|0:0` | 방 생성 실패 |
| `ROOM_JOIN_RES\|1:<id>:<name>` | 입장 성공 |
| `ROOM_JOIN_RES\|0:<id>:` | 입장 실패 |
| `ROOM_NOTICE\|<id>:<notice>` | 공지사항 전달 |
| `ROOM_PIN\|<id>:<msg_id>:<nick>:<content>` | 핀 메시지 전달 |
| `ROOM_INVITE_RES\|0` | 초대 전송 성공 |
| `ROOM_INVITE_RES\|1` | 대상 없음 |
| `ROOM_INVITE_RES\|2` | 이미 멤버 |
| `ROOM_INVITE_RES\|3` | 방 인원 초과 |
| `ROOM_MEMBERS_RES\|<id>:<uid>:<nick>:...` | 멤버 목록 |
