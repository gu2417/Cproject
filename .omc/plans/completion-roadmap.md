# 채팅 프로그램 완성 로드맵

생성일: 2026-05-08
근거: 4개 architect 에이전트의 정밀 검증 보고서 (서버/클라/프로토콜/파일스키마)
대상 코드베이스: `/Users/wovlf/Project/Cproject/chat_program/`
대상 명세: `/Users/wovlf/Project/Cproject/docs/`

---

## 0. 요구사항 요약

현재 코드는 **P0 ~80% / P1 ~60% / P2 ~20% / P3 ~5%** 상태이며, 다음 6대 결손이 P1 후반부터 모든 미구현의 공통 원인이다.

1. **데이터 모델 결손** — `globals.h/c`에 `g_messages, g_room_members, g_dm_reads, g_room_invites, g_user_settings, g_room_reads` 6개 전역 캐시 배열 부재
2. **파일 I/O 결손** — 9개 파일 중 4개(`dm_reads`, `room_invites`, `user_settings`, `room_reads`) load/save/append 함수 미구현
3. **즉시 데이터 손상 버그** — `save_room_members()` 필드 손상, `room_invites.txt` 필드 순서 오류
4. **프로토콜 페이로드 불일치** — `MYPAGE_RES`, `FRIEND_LIST_RES`, `USER_SEARCH_REQ` 명칭/필드 어긋남
5. **핸들러 결손** — 서버 13개, 클라 13개 패킷 핸들러 미구현
6. **보안 위반** — 회원가입 금지문자 미검사, 클라 비밀번호 평문 echo

본 로드맵은 위를 6개 스프린트로 분할하여 docs 명세 100% 준수 상태로 만든다.

---

## 1. Acceptance Criteria (전체)

### 기능적
- [ ] `data/*.txt` 9개 파일 모두 load/save/append 함수 존재 (`file_io.h/c`)
- [ ] `globals.h/c`에 docs `in_memory_structures.md` 명세 6개 전역 캐시 배열 추가
- [ ] `packet_reference.md` 정의 65개 TYPE 모두 `protocol.h`에 매크로화
- [ ] `router.c`에 등록된 핸들러 수 = packet_reference.md C→S 패킷 수 (스텁 함수 0개)
- [ ] 클라 `packet.c`가 packet_reference.md S→C 패킷을 모두 파싱
- [ ] 서버 재시작 후 모든 데이터(친구/방/멤버/읽음/초대/설정/안읽음) 영속

### 보안적
- [ ] `auth.c` 회원가입/로그인에서 ID/닉네임 금지문자(`:;|\n`) 검사
- [ ] 클라 모든 비밀번호 입력은 `getch()+*` 마스킹 (마이페이지 비번 변경, 방 비번)
- [ ] `is_blocked_by(receiver, sender)` 호출 순서 100% 준수

### 품질적
- [ ] `make` 경고 0건 (`-Wall -Wextra` 기준)
- [ ] 모든 fopen 호출 후 NULL 체크
- [ ] 모든 `g_file_mutex` 보호 영역에 `// MUTEX: g_file_mutex` 주석
- [ ] 모든 fprintf의 `//` 구분자가 `file_schema.md` 필드 수와 일치
- [ ] `recv() <= 0` 시 `leftClient()` 호출 (서버), `g_state.connected=0` 처리 (클라)

### 검증적
- [ ] WinSock-tester 에이전트 통과: 256 동시접속, recv 0/-1 시나리오
- [ ] txt-schema-guard 에이전트 통과: 9개 파일 // 패턴, 5슬래시 0건
- [ ] packet-auditor 에이전트 통과: protocol.h ↔ packet_reference.md 100% 일치

---

## 2. 스프린트 구성 (총 6단계)

| 스프린트 | 명칭 | 범위 | 추정 기간 |
|---|---|---|---|
| **S0** | 즉시 버그/보안 수정 | 데이터 손상·보안 위반 5건 | 0.5일 |
| **S1** | 데이터 모델 재구축 | globals + types + 4개 file_io 모듈 추가 | 2일 |
| **S2** | P1 완성 | DM 읽음, MYPAGE, FRIEND_LIST 정합성 | 1.5일 |
| **S3** | P2 서버 핸들러 | 7개 누락 핸들러 + admin_flags | 2일 |
| **S4** | P2 클라이언트 UI | 채팅방 명령어 + 다이얼로그 | 2일 |
| **S5** | P3 부가 기능 | 답장/검색/핀/타이핑/멘션 | 2.5일 |

총 **약 10.5일** (1인 기준).

---

## Sprint S0 — 즉시 버그/보안 수정 (0.5일)

### S0-1. `save_room_members()` 필드 손상 수정

**파일**: `chat_program/src/server/file_io.c:235-256`

**현재 (버그)**:
```c
fprintf(fp, "%d//%s////0//0//\n", g_rooms[i].info.id, g_rooms[i].member_ids[j]);
// → 7필드 출력, is_admin이 빈 값으로 들어가 권한 영구 손실
```

**수정 후**:
```c
// MUTEX: g_file_mutex (caller)
// PLAN: g_room_members 전역 도입 전 임시 — 권한 보존을 위해 기존 파일에서 admin_flag 보존 후 재기록
// 정식 수정은 S1-2에서 g_room_members[] 도입 시 적용
fprintf(fp, "%d//%s//%s//%d//%d//%s\n",
        g_rooms[i].info.id, member_id, open_nick,
        is_admin, is_muted, joined_at);
```

**검증**:
- `cd chat_program && make` 후 방 생성 → 멤버 강퇴 → 서버 재시작 → 방장 권한 유지 확인
- `data/room_members.txt`에 정확히 6필드 (`//` 5개) 라인만 존재

### S0-2. `room_invites.txt` 필드 순서 수정

**파일**: `chat_program/src/server/room.c:531-538`

**현재 (버그)**: `id//room_id//inviter//invitee//<created_at>//0` (status와 created_at 위치 뒤바뀜)

**수정 후**:
```c
// MUTEX: g_file_mutex
fprintf(fp, "%d//%d//%s//%s//0//%s\n",
        invite_id, room_id, inviter_id, invitee_id, ts);
// 필드: id // room_id // inviter // invitee // status(0=pending) // created_at
```

**검증**: file_schema.md §7과 동일한 필드 순서 (`id, room_id, inviter_id, invitee_id, status, created_at`)

### S0-3. `MYPAGE_RES` 페이로드 8필드로 정정

**파일**: `chat_program/src/server/user_store.c:74-82`

**수정**:
```c
// MUTEX: g_file_mutex 안에서 호출
int msg_count   = count_user_messages(u->id_str);   // S1-3에서 추가
int room_count  = count_user_rooms(u->id_str);
int friend_count = count_user_friends(u->id_str);

send_packet(sess->sock,
    MYPAGE_RES "|%s:%s:%s:%s:%d:%d:%d:%s",
    u->id_str, u->nickname, u->created_at, u->last_seen,
    msg_count, room_count, friend_count, u->status_msg);
```

**검증**: 클라 `packet.c:582-611`이 정상 파싱하여 마이페이지 화면에 8개 필드 모두 표시

### S0-4. `auth.c` 금지문자 검사 추가 (보안)

**파일**: `chat_program/src/server/auth.c:64-118` (handle_register), `:17-62` (handle_login)

**추가 코드**:
```c
// .claude/rules/40-security.md 준수
if (has_forbidden_char(id) || has_forbidden_char(nickname)) {
    send_packet(sess->sock, REGISTER_RES "|3");  // ERROR
    return;
}
// 비밀번호 hex 검증 (64자 hex-lowercase)
if (strlen(pw_hash) != 64 || !is_hex_lower(pw_hash)) {
    send_packet(sess->sock, REGISTER_RES "|3");
    return;
}
```

**검증**: `id="alice:bob"` 패킷 송신 시 `REGISTER_RES|3` 응답, users.txt 미수정

### S0-5. 클라 비밀번호 평문 echo 제거 (보안)

**파일들**:
- `chat_program/src/client/menu_mypage.c:75-86` (비번 변경)
- `chat_program/src/client/menu_main.c:51-58` (방 비번 입력 - 입장)
- `chat_program/src/client/menu_main.c:96-101` (방 비번 입력 - 생성)

**공통 패턴 (utility로 추출 권장)**:
```c
// menu_initial.c:14-33의 패턴 재사용
static void read_password_masked(char *out, size_t cap) {
    size_t i = 0;
    int ch;
    while ((ch = _getch()) != '\r' && i < cap - 1) {
        if (ch == '\b' && i > 0) { i--; printf("\b \b"); continue; }
        if (ch >= ' ' && ch < 127) {
            out[i++] = (char)ch;
            putchar('*');
        }
    }
    out[i] = '\0';
    putchar('\n');
}
```

`chat_program/src/client/util_input.c/h` 신설하여 헬퍼 통합.

**검증**: 위 3개 위치에서 비밀번호 입력 시 `*`만 표시, 입력값이 콘솔에 노출되지 않음

---

## Sprint S1 — 데이터 모델 재구축 (2일)

### S1-1. `protocol.h` 용량 상수 + TYPE 매크로 추가

**파일**: `chat_program/src/common/protocol.h`

**추가 상수** (in_memory_structures.md §0):
```c
#define MAX_USERS               1000
#define MAX_FRIENDS             5000
#define MAX_ROOM_MEMBER_RECORDS 6400
#define MAX_DM_READS            10000
#define MAX_INVITES             1000
#define MAX_ROOM_READS          6400
```

**추가 TYPE 매크로** (packet_reference.md 누락분):
```c
/* Room (P2/P3) */
#define ROOM_SEARCH             "ROOM_SEARCH"
#define ROOM_SEARCH_RES         "ROOM_SEARCH_RES"
#define ROOM_MEMBERS_REQ        "ROOM_MEMBERS_REQ"
#define ROOM_MEMBERS_RES        "ROOM_MEMBERS_RES"
#define ROOM_DELETE             "ROOM_DELETE"
#define ROOM_DELETED_NOTIFY     "ROOM_DELETED_NOTIFY"
#define ROOM_SET_NOTICE         "ROOM_SET_NOTICE"
#define ROOM_GRANT_ADMIN        "ROOM_GRANT_ADMIN"
#define ROOM_REVOKE_ADMIN       "ROOM_REVOKE_ADMIN"
#define ROOM_SET_OPEN_NICK      "ROOM_SET_OPEN_NICK"
#define ROOM_SET_OPEN_NICK_RES  "ROOM_SET_OPEN_NICK_RES"
#define ROOM_MUTE_TOGGLE        "ROOM_MUTE_TOGGLE"
#define ROOM_MUTE_TOGGLE_RES    "ROOM_MUTE_TOGGLE_RES"
/* Message */
#define WHISPER                 "WHISPER"
#define WHISPER_RECV            "WHISPER_RECV"
#define MSG_PIN                 "MSG_PIN"
#define MSG_PIN_NOTIFY          "MSG_PIN_NOTIFY"
#define ROOM_PIN                "ROOM_PIN"
/* Typing */
#define TYPING_STOP             "TYPING_STOP"
/* Friend — 명칭 통일 */
#define USER_SEARCH             "USER_SEARCH"   /* docs와 동일, USER_SEARCH_REQ는 deprecated */
```

**응답 코드 매크로화**:
```c
/* ROOM_INVITE_RES */
#define ROOM_INVITE_SENT        0
#define ROOM_INVITE_NOT_FOUND   1
#define ROOM_INVITE_ALREADY     2
#define ROOM_INVITE_FULL        3
/* PROFILE/PASS */
#define PROFILE_OK              0
#define PROFILE_DUP_NICK        1
#define PASS_OK                 0
#define PASS_WRONG_PW           1
```

**검증**: packet-auditor 에이전트로 protocol.h ↔ packet_reference.md 1:1 대조

### S1-2. `types.h` 구조체 docs 정합성 회복

**파일**: `chat_program/src/common/types.h`

**수정 내역**:

1. **`UserRecord`**: `is_admin` 필드 추가 (file_schema.md §1과 일치하도록)
   ```c
   typedef struct {
       char id[21];
       char pw_hash[65];
       char nickname[21];
       char status_msg[101];
       int  online_status;
       int  is_admin;
       char last_seen[20];
       char created_at[20];
   } UserRecord;
   ```

2. **`RoomRecord.notice`**: `[201]` → `[256]` 확장

3. **`RoomInfo`**: `admin_flags[MAX_ROOM_MEMBERS]` 추가 (FR-G07 공동방장 지원)

4. **`RoomInviteRecord`**: 필드 순서 `status` → `created_at`으로 정렬

5. **`RoomReadRecord`**: `read_at[20]` 필드 추가

6. **`ClientSession`**: docs 명세에 맞춰 필드 추가
   - `dnd` (BOOL)
   - `muted_rooms[32]` + `muted_count`
   - `is_admin`
   - `hThread` (HANDLE)
   - `current_room_id` ← `room_id` 명칭 변경 (또는 alias)

**검증**: 컴파일 통과 + 모든 사용처 빌드 성공

### S1-3. `globals.h/c` 6개 전역 캐시 배열 추가

**파일**: `chat_program/src/server/globals.h`, `globals.c`

**추가**:
```c
/* globals.h */
extern UserRecord       g_users[MAX_USERS];
extern int              g_user_count;
extern FriendRecord     g_friends[MAX_FRIENDS];
extern int              g_friend_count;
extern MessageRecord    g_messages[MAX_MSG_HISTORY];
extern int              g_message_count;
extern RoomMemberRecord g_room_members[MAX_ROOM_MEMBER_RECORDS];
extern int              g_room_member_count;
extern DmReadRecord     g_dm_reads[MAX_DM_READS];
extern int              g_dm_read_count;
extern RoomInviteRecord g_room_invites[MAX_INVITES];
extern int              g_room_invite_count;
extern UserSettingsRecord g_user_settings[MAX_USERS];
extern int              g_user_settings_count;
extern RoomReadRecord   g_room_reads[MAX_ROOM_READS];
extern int              g_room_read_count;

/* 카운터 */
extern int g_next_msg_id;
extern int g_next_room_id;
extern int g_next_invite_id;
```

**`restore_next_ids()`** (globals.c:35) 갱신:
- `g_next_msg_id`: messages.txt scan max(id)+1
- `g_next_invite_id`: room_invites.txt scan max(id)+1
- `g_next_room_id`: rooms.txt scan max(id)+1

**카운트 헬퍼 추가** (S0-3 참조):
```c
int count_user_messages(const char *user_id);
int count_user_rooms(const char *user_id);
int count_user_friends(const char *user_id);
```

**검증**: 서버 시작 후 `g_*_count` 값이 각 .txt 라인 수와 일치

### S1-4. `file_io.c/h`에 누락 4개 모듈 추가

**파일**: `chat_program/src/server/file_io.h`, `file_io.c`

**4개 파일 × 3개 함수 = 12개 신규 함수**:

```c
/* dm_reads.txt */
void load_dm_reads(void);                                    // MUTEX: g_file_mutex
void append_dm_read(int user_id_int, int partner_id_int, int last_msg_id);
int  is_dm_read(const char *reader_id, int msg_id);
int  get_unread_dm_count(const char *me_id, const char *partner_id);

/* room_invites.txt */
void load_room_invites(void);
void append_room_invite(const RoomInviteRecord *rec);
void update_invite_status(int invite_id, int new_status);    // accept/reject

/* user_settings.txt */
void load_user_settings(void);
void upsert_user_settings(const UserSettingsRecord *rec);
const UserSettingsRecord *get_user_settings(const char *user_id);

/* room_reads.txt */
void load_room_reads(void);
void upsert_room_read(const char *user_id, int room_id, int last_msg_id);
int  get_unread_room_count(const char *user_id, int room_id);
```

**`main.c:43-47` startup 수정**:
```c
WaitForSingleObject(g_file_mutex, INFINITE);
load_users();
load_rooms();
load_room_members();
load_friends();
load_messages();
load_dm_reads();
load_room_invites();
load_user_settings();
load_room_reads();
restore_next_ids();
ReleaseMutex(g_file_mutex);
```

**검증**: txt-schema-guard 에이전트로 9개 파일 // 패턴 검증, 모든 load 함수가 g_*_count 갱신

### S1-5. `save_room_members()` 정식 수정 (S0-1 임시본 대체)

**파일**: `chat_program/src/server/file_io.c:235-256`

`g_room_members[]` 배열 도입 후 그 배열을 직렬화하도록 변경:
```c
void save_room_members(void) {
    // MUTEX: g_file_mutex (caller)
    FILE *fp = fopen(FILE_ROOM_MEMBERS, "w");
    if (!fp) return;
    for (int i = 0; i < g_room_member_count; i++) {
        const RoomMemberRecord *m = &g_room_members[i];
        fprintf(fp, "%d//%s//%s//%d//%d//%s\n",
                m->room_id, m->user_id, m->open_nick,
                m->is_admin, m->is_muted, m->joined_at);
    }
    fclose(fp);
}
```

**검증**: 방장 권한 방 강퇴 시나리오에서 권한 유지 (winsock-tester 시나리오)

---

## Sprint S2 — P1 완성 (1.5일)

### S2-1. `MYPAGE_RES` 8필드 + 통계 카운트 (S0-3 후속)

S1-3의 `count_user_*` 헬퍼를 사용하여 `user_store.c:74` 수정 완료.

### S2-2. `FRIEND_LIST_RES` 페이로드 정합성

**파일**: `chat_program/src/server/friend.c:283-289`

**현재**: `count:id:nick:friend_status:online_status` (status_msg 누락)

**수정**: `count:id:nick:online_status:status_msg` (docs §2와 일치)
- pending 상태(`friend_status=0`)는 별도 패킷 `FRIEND_PENDING_LIST_RES`로 분리, 또는 `friend_status` 필드 5번째로 추가하고 docs 갱신
- 권장: docs 갱신 (5필드로 확장) — 이유: pending도 한 화면에서 보여주는 게 UX 우수

**클라 수정**: `chat_program/src/client/packet.c:478-519`도 5필드 파싱

### S2-3. `USER_SEARCH_REQ` → `USER_SEARCH` 명칭 통일

**파일**:
- `chat_program/src/common/protocol.h:116` — `USER_SEARCH` 매크로 추가 (S1-1에서 완료)
- `chat_program/src/server/friend.c:332` — `register_handler(USER_SEARCH, ...)`로 변경
- `chat_program/src/client/` 송신부 — 신규 작성 시 `USER_SEARCH` 사용

### S2-4. DM 읽음 (FR-D03, FR-D05)

**서버**:
- `chat_program/src/server/dm.c`: `handle_dm_read(...)` 신규 (`DM_READ` 처리)
- `dm.c:215-262` `handle_dm_history`: `is_dm_read()` 호출하여 read 필드 정확히 채우기
- `dm.c:115-181` `handle_dm_list`: `get_unread_dm_count()` 호출하여 unread 정확히 채우기
- 송신: `DM_READ_NOTIFY|<reader_id>:<partner_id>:<last_msg_id>`

**클라**:
- `chat_program/src/client/packet.c`: `DM_READ_NOTIFY` 핸들러 추가
- `menu_dm.c`: DM 화면 진입 시 자동 `DM_READ` 송신
- `packet.c:378-423` DM_HISTORY_RES 출력에서 `[읽음]/[안읽음]` 배지 표시

**검증**: A↔B DM 시나리오에서 unread 카운트 정확, B가 진입 시 A화면에 `[읽음]` 표시

### S2-5. `MY_ROOMS_RES` 처리 (마이페이지)

**서버**: `user_store.c`에 `MY_ROOMS_REQ` 핸들러는 이미 등록(line ~94). 페이로드 형식 검증.

**클라**: `chat_program/src/client/packet.c`에 `MY_ROOMS_RES` 핸들러 추가
**클라 메뉴**: `menu_mypage.c`에 "참여 채팅방 목록" 항목 추가 (CUI §27 4번)

### S2-6. 회원가입 status_msg + 방 생성 topic 필드 추가

**클라**:
- `menu_initial.c:92-131`: 회원가입 시 status_msg 입력 받아 `REGISTER_REQ|id:pw:nick:status` 4필드 송신
- `menu_main.c:74-126`: 방 생성 시 topic 입력 받아 `ROOM_CREATE|name:max:is_open:pw:topic` 5필드 송신

**서버**: `auth.c:64`, `room.c:84`이 추가 필드 파싱

### S2-7. `DM_SEND` 송신자 echo

**파일**: `chat_program/src/server/dm.c:67-72`

`send_to_user(to_id, buf)` 외에 `send_to_user(from_id, buf)`도 호출하여 송신자 본인 화면에도 반영. 클라이언트의 로컬 echo 부담 제거.

---

## Sprint S3 — P2 서버 핸들러 (2일)

### S3-1. 방장 권한 인프라

**`is_room_admin(room_id, user_id)` 헬퍼** (room.c 또는 module_room.md 명세):
```c
int is_room_admin(int room_id, const char *user_id) {
    // MUTEX: g_sessions_mutex (caller) — g_rooms 접근
    int ri = find_room_index(room_id);
    if (ri < 0) return 0;
    if (strcmp(g_rooms[ri].info.owner_id, user_id) == 0) return 1;
    for (int i = 0; i < g_rooms[ri].member_count; i++) {
        if (strcmp(g_rooms[ri].member_ids[i], user_id) == 0) {
            return g_rooms[ri].admin_flags[i];
        }
    }
    return 0;
}
```

### S3-2. 누락 핸들러 7개 구현

각각 `register_handler()` + 실제 함수:

| 핸들러 | 위치 | 핵심 로직 |
|---|---|---|
| `handle_room_delete` | room.c | 방장만 가능, 모든 멤버에게 `ROOM_DELETED_NOTIFY`, `g_rooms[i].is_deleted=1` |
| `handle_room_set_notice` | room.c | 방장/공동방장만, content-last 파싱, 멤버에게 `ROOM_NOTICE` 브로드캐스트 |
| `handle_room_grant_admin` | room.c | 방장만, `admin_flags[]=1` 후 멤버 동기화 |
| `handle_room_revoke_admin` | room.c | 방장만, `admin_flags[]=0` |
| `handle_room_members_req` | room.c | 방 멤버에게만, `ROOM_MEMBERS_RES|count:id:nick:open_nick:is_admin:online;...` |
| `handle_room_set_open_nick` | room.c | `is_open=1`인 방의 멤버만, `g_room_members[]`의 open_nick 갱신 |
| `handle_whisper` | message.c | room 멤버 두 명에게만 `WHISPER_RECV` 송신 |

각 핸들러는 다음 공통 검증:
- 호출자 유효 세션
- 권한 검증 (방장/멤버)
- 금지문자 검사
- 차단 검사 (`is_blocked_by(receiver, sender)`)

### S3-3. `MSG_PIN` 핸들러 + `ROOM_PIN` 입장 push

**서버**:
- `message.c`: `handle_msg_pin(room_id, msg_id)` — 방장/공동방장만, `g_rooms[ri].info.pinned_msg_id = msg_id`
- 멤버에게 `MSG_PIN_NOTIFY|<room_id>:<msg_id>:<from_id>:<from_nick>:<content>` 브로드캐스트
- `room.c handle_room_join` 성공 분기: 방의 `pinned_msg_id`가 0이 아니면 입장한 사용자에게 `ROOM_PIN|<msg_id>:<content>` push

### S3-4. `broadcast_to_room` 멤버 기반 수정

**파일**: `chat_program/src/server/broadcast.c:59-73`

**현재**: `g_sessions[i].room_id == room_id` 검사 → 메뉴화면에 있는 멤버 미수신

**수정**: RoomInfo의 `member_ids[]` 순회 후 각 멤버의 활성 세션 조회 → `send_to_user(member_id, buf)`
```c
void broadcast_to_room(int room_id, const char *payload) {
    // MUTEX: g_sessions_mutex (caller)
    int ri = find_room_index(room_id);
    if (ri < 0) return;
    for (int i = 0; i < g_rooms[ri].member_count; i++) {
        send_to_user(g_rooms[ri].member_ids[i], payload);
    }
}
```

`module_broadcast.md §3` 명세 준수.

### S3-5. 시스템 메시지 영속화 (FR-M07)

`room.c:228-234, 274-282`의 입퇴장 메시지를 `messages.txt`에 `msg_type=1`로 저장하여 history에 포함. 현재는 메모리에서만 broadcast.

`broadcast_system_msg(room_id, content)` 헬퍼를 `message.c`에 추가하여 중복 제거.

---

## Sprint S4 — P2 클라이언트 UI (2일)

### S4-1. 채팅방 명령어 확장

**파일**: `chat_program/src/client/menu_chat.c`

추가 명령어:
- `/invite <id>` → `ROOM_INVITE` 송신
- `/members` → `ROOM_MEMBERS_REQ` 송신, `ROOM_MEMBERS_RES`에서 출력
- `/del <msg_id>` → 권한 확인 후 `MSG_DELETE` 송신
- `/edit <msg_id> <new_content>` → `MSG_EDIT` 송신
- `/notice <text>` → `ROOM_SET_NOTICE` 송신
- `/kick <id>` → `ROOM_KICK` 송신
- `/grant <id>` / `/revoke <id>` → `ROOM_GRANT_ADMIN` / `ROOM_REVOKE_ADMIN`
- `/open_nick <nick>` (is_open=1 방에서만) → `ROOM_SET_OPEN_NICK`
- `/w <id> <text>` → `WHISPER` 송신
- `/help` 텍스트 확장

### S4-2. 누락 클라 패킷 핸들러 13개

**파일**: `chat_program/src/client/packet.c`

| 패킷 | 동작 |
|---|---|
| `USER_SEARCH_RES` | 검색 결과 출력 + 액션 메뉴 (CUI §8) |
| `MY_ROOMS_RES` | 마이페이지 4번 메뉴 |
| `DM_READ_NOTIFY` | DM 화면 라이브 갱신 |
| `ROOM_DELETED_NOTIFY` | 강제 메뉴 복귀 + 안내 |
| `WHISPER_RECV` | `[귓속말 from X] ...` 출력 |
| `ROOM_MEMBERS_RES` | 멤버 목록 화면 (CUI §20/21) |
| `MSG_PIN_NOTIFY` | 핀 알림 + 채팅창 상단 갱신 |
| `ROOM_PIN` | 방 입장 시 핀 메시지 표시 |
| `ROOM_SEARCH_RES` | 오픈채팅 검색 결과 |
| `MSG_SEARCH_RES` | 검색 결과 |
| `ROOM_SET_OPEN_NICK_RES` | 닉네임 변경 결과 |
| `ERROR` | 서버 에러 메시지 출력 |
| `PING` (서버→클라 keep-alive) | 즉시 `PONG` 회신 |

### S4-3. 다이얼로그 (y/n 확인) 헬퍼

**파일**: `chat_program/src/client/util_input.c/h` (S0-5에서 신설)

```c
int confirm_yn(const char *prompt);  // 1=yes, 0=no
```

적용 위치 (DLG §1, §2, §18~25):
- `menu_main.c` 로그아웃 (`case '7'`)
- `menu_chat.c` `/leave`
- `menu_friend.c` 친구 삭제/차단
- `menu_main.c` 방 삭제

### S4-4. 친구 메뉴 정상화

**파일**: `chat_program/src/client/menu_friend.c`

- `c` 키 매핑을 "유저 검색"으로 변경 (사양 일치, 거절은 항목 액션 메뉴로 이동)
- 친구 항목 번호 선택 시 액션 메뉴 (DM/초대/프로필/삭제/차단) 표시
- pending 요청 별도 섹션으로 시각 분리
- 차단 목록 별도 진입 (CUI §21)
- `USER_SEARCH` 송신 + `USER_SEARCH_RES` 처리

### S4-5. 메시지 액션 컨텍스트 메뉴

**파일**: `chat_program/src/client/menu_chat.c`

`/msg` 또는 메시지 번호 선택 시:
1. 답장 → `MSG_REPLY` (S5)
2. 수정 → `MSG_EDIT` (자기 메시지 + 5분 이내만)
3. 삭제 → `MSG_DELETE`
4. 핀 (방장/공동방장) → `MSG_PIN`
5. 신고 → 외부

### S4-6. 클라 동시성 보강

**파일**: `chat_program/src/common/types.h`

- `response_received`, `connected`, `last_pong` 등 RecvMsg ↔ main 공유 변수에 `volatile` 추가
- 또는 `CreateEvent`/`SetEvent` 이벤트 객체로 교체 (권장)

`g_dm_list[]`, `g_friend_list[]` 등 캐시 갱신은 `g_console_mutex`와 별도의 락 도입 또는 wait_response 직렬화로 보호.

---

## Sprint S5 — P3 부가 기능 (2.5일)

### S5-1. 답장 (FR-M04)

**서버**: `message.c:251` 빈 함수 → 정상 구현
- `MSG_REPLY|<room_id>:<reply_to>:<content>` 파싱
- `MessageRecord.reply_to_id` 설정 후 messages.txt append
- `ROOM_MSG_RECV`에 `reply_to` 필드 포함하여 broadcast

**클라**: `menu_chat.c` `/reply <msg_id> <text>` + 출력 시 인용 박스

### S5-2. 검색 (FR-M08)

**서버**: `message.c:257` 빈 함수 → 정상 구현
- `MSG_SEARCH|<scope>:<keyword>` (scope=room/dm), content-last
- 멤버십 검증 후 messages.txt 스캔 (`strstr` 매치)
- `MSG_SEARCH_RES|<count>:<msg_id>:<room_id>:<from_id>:<from_nick>:<ts>:<content>;...`

**클라**: `menu_chat.c` `/search <keyword>` + 결과 화면 (CUI §49)

### S5-3. 핀 (FR-M10) — S3-3 완성

S3-3에서 서버 측 완료. 클라 출력 포맷 정밀화.

### S5-4. 타이핑 인디케이터 (FR-N05)

**서버**: `handle_typing_start`, `handle_typing_stop` (broadcast_to_room으로 `TYPING_NOTIFY`)

**클라**: 사용자가 1초 이상 타자 입력 중일 때 `TYPING_START` 송신, Enter/cancel 시 `TYPING_STOP` 송신
- 타이머 기반 (`SetTimer` 또는 별도 스레드)
- 수신: 화면 하단에 `[X님 입력 중...]` 표시 (NTF §5)

### S5-5. 멘션 (FR-N03)

**서버**: `room.c handle_room_msg`에서 `detect_mention(content, member_nick)` 호출
- 매치 시 해당 멤버에게 추가 `NOTIFY` 발신 또는 `ROOM_MSG_RECV`에 `mentioned=1` 플래그 추가

**클라**: ROOM_MSG_RECV 출력 시 멘션 메시지 강조 (색상/굵게)

### S5-6. DND (FR-O02) + 방 음소거 (FR-N06)

**서버**:
- `STATUS_CHANGE` 처리에서 `g_sessions[i].dnd` 갱신
- `handle_room_mute_toggle`: `g_sessions[i].muted_rooms[]` 갱신
- 알림 발신 전 `g_sessions[receiver].dnd` / `muted_rooms` 검사

**클라**: 설정 메뉴에서 DND 토글, 방 음소거는 채팅방 명령어 `/mute` `/unmute`

### S5-7. 이모티콘 변환 (FR-M06)

`utils.c:42-76 convert_emoticons` 함수는 이미 존재. 활용:
- 클라 송신 직전 또는 서버 ROOM_MSG 처리 시 적용 (정책 결정 필요 — 일관성 위해 서버 측 권장)

### S5-8. /me 액션 (FR-M11)

`utils.c parse_me_action` 헬퍼 신설 + 서버 broadcast 시 `msg_type=3`으로 표시 분기.

---

## 3. 위험 및 완화

| 위험 | 영향 | 완화 |
|---|---|---|
| S1 데이터 모델 재구축 시 P0 회귀 | 빌드 깨짐, 기존 기능 동작 안 함 | (1) 각 스프린트 후 `make && ./server.exe` smoke test, (2) winsock-tester 에이전트 시나리오 자동화, (3) S0 직후 S1 진입 전 git tag `v0-baseline` |
| 구조체 필드 추가로 binary 호환성 깨짐 | 기존 .txt 파싱 실패 | file_io.c load 함수가 누락 필드를 빈 값/0으로 graceful 처리, 신규 필드는 `// + 빈값` 추가만 |
| `MAX_USERS=1000` 등 메모리 폭증 | 스택/.bss 크기 증가 | 정적 배열 크기 합산 ~수백 MB 미만임을 사전 계산. 필요 시 동적 할당으로 전환 |
| 스레드 race (RecvMsg ↔ main) | 클라이언트 캐시 찢김 | S4-6의 `volatile` 또는 이벤트 객체 도입 |
| 5스프린트 누적 PR 충돌 | 머지 지옥 | 스프린트별 독립 브랜치 + 매 스프린트 종료 시 main 머지 |
| docs ↔ 코드 불일치 발견 시 결정 지연 | 작업 중단 | CLAUDE.md "Source-of-Truth Hierarchy" 적용 (in_memory_structures.md 우선), 명백한 docs 오류는 errata 주석 후 수정 |

---

## 4. 검증 단계 (스프린트별)

### 자동 검증 (각 스프린트 종료 시)
```bash
cd chat_program && make clean && make
# → 경고 0건, 빌드 성공
```

### 에이전트 검증
- `txt-schema-guard` — 9개 파일 // 패턴, 5슬래시 0건 (S0, S1, S3-5 후)
- `packet-auditor` — protocol.h ↔ packet_reference.md, content-last/count-first (S1, S2, S3 후)
- `winsock-tester` — 256 동시접속, recv 0/-1, leftClient (S1, S3, S4 후)
- `oh-my-claudecode:verifier` — Acceptance Criteria 체크리스트 (각 스프린트 종료 시)

### 수동 시나리오 (S5 후 최종)
1. **회원가입 → 로그인 → 방 생성 → 멤버 초대 → 메시지 송수신 → 수정 → 삭제 → 답장 → 핀** 전 흐름 1회 완주
2. **DM A→B → B 진입 → A화면 [읽음] 갱신 → DM 검색** 정상 동작
3. **친구 추가 → 차단 → 차단 해제 → DM/초대 거부 검증** (`is_blocked_by` 방향 검증)
4. **방장 권한 위임 → 공지 등록 → 강퇴 → 방 삭제** 일련 동작
5. **서버 재시작 → 모든 데이터 유지** 확인 (data/*.txt 9개 파일 기반)

---

## 5. 마일스톤 및 git 전략

| 시점 | 태그 | 의미 |
|---|---|---|
| 현재 | `v0-baseline` | 분석 직후 상태 (백업) |
| S0 종료 | `v0.1-bugfix` | 보안/데이터 손상 해결 |
| S1 종료 | `v0.5-foundation` | 데이터 모델/파일 I/O 완성 |
| S2 종료 | `v1.0-p1-complete` | P1 완료 (P0+P1 docs 일치) |
| S3 종료 | `v1.5-p2-server` | 서버 P2 핸들러 전부 |
| S4 종료 | `v2.0-p2-complete` | P2 완료 (클라 UI 포함) |
| S5 종료 | `v3.0-final` | docs 100% 구현 (P3 포함) |

각 스프린트는 별도 브랜치 (`sprint/s0`, `sprint/s1`, ...) → 종료 시 main 머지 + 태그.

**커밋 컨벤션** (`.claude/rules/60-git-commit.md` 준수):
- S0: `fix(file_io): save_room_members 5슬래시 버그 정정`, `fix(security): 회원가입 금지문자 검사 추가`
- S1: `refactor(globals): docs 기준 6개 캐시 배열 추가`, `feat(file_io): dm_reads/invites/settings/reads 모듈 추가`
- S2~S5: `feat(<scope>): ...` 위주

---

## 6. 우선순위 요약

```
S0 (즉시) ─────────────────── 데이터 손상·보안 차단
   │
   ▼
S1 (기반) ─────────────────── 결손 6개 캐시 + 4개 file_io
   │                          (이후 모든 P1+ 작업의 전제)
   ▼
S2 (P1 완성) ──────────────── 사용자 체감 기능 정합화
   │
   ▼
S3 (P2 서버) + S4 (P2 클라) ─ 권한/관리/명령어 (병렬 가능)
   │
   ▼
S5 (P3) ──────────────────── 부가 기능
```

각 스프린트는 다음 스프린트의 전제 조건이며, 특히 **S1은 모든 후속 작업의 결정적 의존**이다.

---

## 7. 참고 문서 (Source-of-Truth Hierarchy)

1. `docs/overview/requirements_traceability.md`
2. `docs/database/in_memory_structures.md`
3. `docs/protocol/packet_reference.md`
4. `docs/database/file_schema.md`
5. `docs/architecture/module_*.md`
6. `docs/features/FR_*.md`
7. `requirements.md` (FR 의미만)

본 로드맵은 1~6번 문서를 baseline으로 하며 코드와 docs 충돌 시 (예: `packet_reference.md:65` ROOM_JOIN_RES 표기 오류) 별도 errata 노트를 추가한 후 진행한다.
