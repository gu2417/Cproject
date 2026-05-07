# 공통 모듈 (common/)

서버·클라이언트가 공유하는 헤더와 유틸리티. 빌드 시 양쪽 모두에 링크된다 (`build_guide.md` 참조).

```
src/common/
├── protocol.h    # 패킷 타입 상수, 구분자, 응답 코드 #define
├── types.h       # 공통 구조체 (UserRecord, RoomRecord, MessageRecord 등)
├── utils.c       # 타임스탬프, 이모티콘 변환, 문자열 유틸 구현
└── utils.h       # utils.c 외부 선언
```

> requirements.md §10의 `common/net_win.c` 는 콘솔 + 단일 플랫폼(Windows) 전제하에 별도 추상화 레이어가 불필요하므로 **삭제**되었다. 클라이언트 소켓 헬퍼는 `client/net.c`로 흡수되었다.

---

## 1. protocol.h — 패킷 타입 상수

`packet_reference.md`의 모든 패킷 TYPE 문자열을 `#define`으로 노출한다. 클라이언트·서버 모두 매크로를 통해 비교/송신함으로써 오타 방지.

```c
/* protocol.h */
#ifndef PROTOCOL_H
#define PROTOCOL_H

/* === 구분자 === */
#define PKT_DELIM_TYPE   '|'
#define PKT_DELIM_FIELD  ':'
#define PKT_DELIM_LIST   ';'
#define PKT_TERMINATOR   '\n'

/* === 인증 (FR-A) === */
#define LOGIN_REQ        "LOGIN_REQ"
#define LOGIN_RES        "LOGIN_RES"
#define REGISTER_REQ     "REGISTER_REQ"
#define REGISTER_RES     "REGISTER_RES"
#define LOGOUT_REQ       "LOGOUT_REQ"      /* P1+ */
#define LOGOUT_RES       "LOGOUT_RES"      /* P1+ */

/* === 친구 (FR-F) === */
#define FRIEND_ADD_REQ          "FRIEND_ADD_REQ"
#define FRIEND_ADD_RES          "FRIEND_ADD_RES"
#define FRIEND_REQUEST_NOTIFY   "FRIEND_REQUEST_NOTIFY"
#define FRIEND_ACCEPT           "FRIEND_ACCEPT"
#define FRIEND_ACCEPT_NOTIFY    "FRIEND_ACCEPT_NOTIFY"
#define FRIEND_REJECT           "FRIEND_REJECT"
#define FRIEND_DELETE           "FRIEND_DELETE"
#define FRIEND_BLOCK            "FRIEND_BLOCK"
#define FRIEND_LIST_REQ         "FRIEND_LIST_REQ"
#define FRIEND_LIST_RES         "FRIEND_LIST_RES"
#define FRIEND_STATUS_CHANGE    "FRIEND_STATUS_CHANGE"
#define USER_SEARCH             "USER_SEARCH"
#define USER_SEARCH_RES         "USER_SEARCH_RES"

/* === DM (FR-D) === */
#define DM_SEND          "DM_SEND"
#define DM_RECV          "DM_RECV"
#define DM_READ_NOTIFY   "DM_READ_NOTIFY"
#define DM_HISTORY_REQ   "DM_HISTORY_REQ"
#define DM_HISTORY_RES   "DM_HISTORY_RES"
#define DM_LIST_REQ      "DM_LIST_REQ"     /* P1+ */
#define DM_LIST_RES      "DM_LIST_RES"

/* === 방 (FR-G, FR-O) === */
#define ROOM_CREATE             "ROOM_CREATE"
#define ROOM_CREATE_RES         "ROOM_CREATE_RES"
#define ROOM_LIST_REQ           "ROOM_LIST_REQ"
#define ROOM_LIST_RES           "ROOM_LIST_RES"
#define ROOM_SEARCH             "ROOM_SEARCH"
#define ROOM_SEARCH_RES         "ROOM_SEARCH_RES"
#define ROOM_JOIN               "ROOM_JOIN"
#define ROOM_JOIN_RES           "ROOM_JOIN_RES"
#define ROOM_NOTICE             "ROOM_NOTICE"
#define ROOM_PIN                "ROOM_PIN"
#define ROOM_MSG                "ROOM_MSG"
#define ROOM_MSG_RECV           "ROOM_MSG_RECV"
#define ROOM_HISTORY_REQ        "ROOM_HISTORY_REQ"     /* P1+ */
#define ROOM_HISTORY_RES        "ROOM_HISTORY_RES"
#define ROOM_LEAVE              "ROOM_LEAVE"
#define ROOM_INVITE             "ROOM_INVITE"
#define ROOM_INVITE_RES         "ROOM_INVITE_RES"
#define ROOM_INVITE_NOTIFY      "ROOM_INVITE_NOTIFY"   /* P1+ */
#define ROOM_KICK               "ROOM_KICK"            /* P2+ */
#define ROOM_KICKED_NOTIFY      "ROOM_KICKED_NOTIFY"   /* P2+ */
#define ROOM_DELETE             "ROOM_DELETE"          /* P2+ */
#define ROOM_DELETED_NOTIFY     "ROOM_DELETED_NOTIFY"  /* P2+ */
#define ROOM_SET_NOTICE         "ROOM_SET_NOTICE"
#define ROOM_GRANT_ADMIN        "ROOM_GRANT_ADMIN"
#define ROOM_REVOKE_ADMIN       "ROOM_REVOKE_ADMIN"
#define ROOM_MEMBERS_REQ        "ROOM_MEMBERS_REQ"
#define ROOM_MEMBERS_RES        "ROOM_MEMBERS_RES"

/* === 메시지 부가 (FR-M) === */
#define WHISPER             "WHISPER"
#define WHISPER_RECV        "WHISPER_RECV"
#define MSG_DELETE          "MSG_DELETE"
#define MSG_DELETED_NOTIFY  "MSG_DELETED_NOTIFY"
#define MSG_EDIT            "MSG_EDIT"
#define MSG_EDITED_NOTIFY   "MSG_EDITED_NOTIFY"
#define MSG_REPLY           "MSG_REPLY"
#define MSG_SEARCH          "MSG_SEARCH"
#define MSG_SEARCH_RES      "MSG_SEARCH_RES"
#define MSG_PIN             "MSG_PIN"
#define MSG_PIN_NOTIFY      "MSG_PIN_NOTIFY"

/* === 설정 (FR-C) === */
#define SETTINGS_REQ            "SETTINGS_REQ"
#define SETTINGS_RES            "SETTINGS_RES"
#define SETTINGS_UPDATE         "SETTINGS_UPDATE"
#define SETTINGS_UPDATE_RES     "SETTINGS_UPDATE_RES"
#define STATUS_CHANGE           "STATUS_CHANGE"
#define PROFILE_UPDATE          "PROFILE_UPDATE"
#define PROFILE_UPDATE_RES      "PROFILE_UPDATE_RES"
#define PASS_CHANGE             "PASS_CHANGE"
#define PASS_CHANGE_RES         "PASS_CHANGE_RES"
#define ROOM_SET_OPEN_NICK      "ROOM_SET_OPEN_NICK"
#define ROOM_SET_OPEN_NICK_RES  "ROOM_SET_OPEN_NICK_RES"
#define ROOM_MUTE_TOGGLE        "ROOM_MUTE_TOGGLE"        /* P3+ */
#define ROOM_MUTE_TOGGLE_RES    "ROOM_MUTE_TOGGLE_RES"

/* === 마이페이지 (FR-P) === */
#define MYPAGE_REQ      "MYPAGE_REQ"
#define MYPAGE_RES      "MYPAGE_RES"
#define MY_ROOMS_REQ    "MY_ROOMS_REQ"     /* P1+ */
#define MY_ROOMS_RES    "MY_ROOMS_RES"

/* === 타이핑 (FR-N05) === */
#define TYPING_START    "TYPING_START"
#define TYPING_STOP     "TYPING_STOP"
#define TYPING_NOTIFY   "TYPING_NOTIFY"

/* === 알림 (FR-N) === */
#define NOTIFY          "NOTIFY"
#define NOTIFY_TYPE_MENTION     "MENTION"
#define NOTIFY_TYPE_FRIEND_REQ  "FRIEND_REQ"
#define NOTIFY_TYPE_SERVER      "SERVER"
#define NOTIFY_TYPE_DM          "DM"

/* === 연결 유지 (P2+) === */
#define PING            "PING"
#define PONG            "PONG"

/* === 응답 코드 — 공통 === */
#define CODE_OK         0
#define CODE_FAIL       1

/* === 응답 코드 — LOGIN_RES === */
#define LOGIN_OK             0
#define LOGIN_WRONG_ID       1
#define LOGIN_WRONG_PW       2
#define LOGIN_ALREADY_ONLINE 3   /* P1+ */

/* === 응답 코드 — REGISTER_RES === */
#define REGISTER_OK           1
#define REGISTER_DUPLICATE_ID 2

/* === 응답 코드 — FRIEND_ADD_RES === */
#define FRIEND_ADD_SENT          0
#define FRIEND_ADD_NOT_FOUND     1
#define FRIEND_ADD_BLOCKED       2
#define FRIEND_ADD_ALREADY       3

/* === 응답 코드 — ROOM_INVITE_RES === */
#define INVITE_SENT          0
#define INVITE_NOT_FOUND     1
#define INVITE_ALREADY       2
#define INVITE_ROOM_FULL     3

/* === 메시지 타입 (msg_type) === */
#define MSG_TYPE_NORMAL    0
#define MSG_TYPE_SYSTEM    1
#define MSG_TYPE_WHISPER   2
#define MSG_TYPE_ME_ACTION 3

/* === 온라인 상태 === */
#define STATUS_OFFLINE   0
#define STATUS_ONLINE    1
#define STATUS_BUSY      2
#define STATUS_INVISIBLE 3

#endif /* PROTOCOL_H */
```

---

## 2. types.h — 공통 구조체 위치 정책

| 구조체 | 위치 | 사용 |
|--------|------|------|
| `ClientSession`, `RoomInfo` | `server/globals.h` | 서버 전용 (RAM only) |
| `ClientState` | `client/state.h` | 클라이언트 전용 |
| `UserRecord`, `RoomRecord`, `MessageRecord`, `FriendRecord`, `RoomMemberRecord`, `DmReadRecord`, `RoomInviteRecord`, `UserSettingsRecord`, `RoomReadRecord` | `common/types.h` | 서버 파일 직렬화 + 일부 클라이언트 표시용 |

`common/types.h`는 서버 파일 I/O 평탄 구조체를 정의한다. 클라이언트는 `MessageRecord`를 콘솔 표시 시 부분적으로만 사용한다.

```c
/* types.h */
#ifndef TYPES_H
#define TYPES_H

#include "config.h"

/* 정의는 database/in_memory_structures.md 참조.
   여기서는 forward declaration + include 순서만 책임. */
typedef struct UserRecord_         UserRecord;
typedef struct RoomRecord_         RoomRecord;
typedef struct MessageRecord_      MessageRecord;
typedef struct FriendRecord_       FriendRecord;
typedef struct RoomMemberRecord_   RoomMemberRecord;
typedef struct DmReadRecord_       DmReadRecord;
typedef struct RoomInviteRecord_   RoomInviteRecord;
typedef struct UserSettingsRecord_ UserSettingsRecord;
typedef struct RoomReadRecord_     RoomReadRecord;

/* 실제 멤버 정의 (in_memory_structures.md §5와 동일) ... */

#endif
```

---

## 3. utils.c/h — 공통 유틸리티

### 3-1. 타임스탬프

```c
/* 현재 시각을 "YYYY-MM-DD HH:MM:SS" 형식으로 반환 */
void get_current_timestamp(char out[20]);

/* 타임스탬프 문자열을 ts_format(0=HH:MM, 1=HH:MM:SS, 2=MM-DD HH:MM)에 맞게 변환 */
void format_timestamp(const char *full_ts, int ts_format, char *out, size_t out_size);

/* "YYYY-MM-DD HH:MM:SS" 문자열을 time_t로 변환 (5분 수정 만료 등 산술용) */
time_t parse_timestamp(const char *ts);

/* 두 타임스탬프 차이를 분 단위로 반환 (음수 = ts2가 더 이른 시각) */
int    diff_minutes(const char *ts1, const char *ts2);

/* 사람이 읽기 쉬운 상대 시각: "방금 전", "5분 전", "어제", "2026-04-15" */
void   relative_time(const char *ts, char *out, size_t out_size);
```

### 3-2. 이모티콘 변환 (FR-M06)

```c
/* :smile: → (^_^), :heart: → <3 등 텍스트 치환 (in-place 또는 buf 출력) */
void convert_emoticons(char *content, size_t buf_size);
```

치환 테이블은 `help_and_guides.md §3` 참조.

### 3-3. 입력 검증 (NFR-04, FR-A01)

```c
/* 금지문자(:, ;, |, \n) 포함 여부 — 0=OK, 1=금지문자 발견 */
int  has_forbidden_char(const char *s);

/* UTF-8 바이트 길이 검증 — 한글 1자 = 3바이트 처리 */
int  validate_length(const char *s, int min_bytes, int max_bytes);

/* 양 끝 공백 제거 */
void trim(char *s);

/* 빈 문자열 또는 공백만 있는지 — 1=빈, 0=비어있지 않음 */
int  is_blank(const char *s);
```

### 3-4. 문자열 유틸

```c
/* 안전한 strncpy: 항상 null-terminate 보장 */
void safe_strcpy(char *dst, const char *src, size_t dst_size);

/* 멘션(@닉네임) 검출 — 닉네임이 발견된 위치 반환, 없으면 NULL */
const char *find_mention(const char *content, const char *nickname);
```

---

## 4. 헤더 의존성

```
config.h ──┬──> globals.h (server)
           ├──> state.h   (client)
           └──> types.h   (common)

types.h  ──┬──> file_io.h (server)
           └──> packet.h  (client packet_parse 시 일부 사용)

protocol.h ─> 모든 패킷 송수신 코드 (server router.c, client packet.c)

utils.h  ──> 모든 모듈 (타임스탬프·이모티콘·검증)
```

순환 의존을 막기 위해 `common/`은 `server/`나 `client/`를 include하지 않는다.
