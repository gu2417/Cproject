# 파일 스키마

## 설계 결정: MySQL 대신 텍스트 파일 사용

`reference/server_main.c` 분석 결과, 원본 구현이 `users.txt`를 `id//pw\n` 포맷으로 읽어 인메모리 구조체에 로드하는 방식을 사용한다. 이를 전체 데이터 모델로 확장하여 MySQL 없이 텍스트 파일만으로 모든 영속 데이터를 관리한다.

**파일 경로**: 서버 실행 디렉토리 기준 `data/` 폴더

**구분자**: `//` (reference 파일 준수)

**주의**: 모든 필드 값에 `//` 문자열 포함 불가.

---

## 1. users.txt — 유저 정보

### 포맷
```
<id>//<pw_hash>//<nickname>//<status_msg>//<online_status>//<is_admin>//<last_seen>//<created_at>
```

### 필드 정의

| 필드 | 타입 | 설명 |
|------|------|------|
| id | 문자열 (최대 20자) | 로그인 ID (고유) |
| pw_hash | 문자열 (64자) | SHA-256 해시값 (hex 소문자) |
| nickname | 문자열 (최대 20자) | 표시 닉네임 (고유) |
| status_msg | 문자열 (최대 100자) | 상태메시지 (빈 값 허용) |
| online_status | 정수 | 0=offline, 1=online, 2=busy, 3=invisible |
| is_admin | 정수 | 0=일반, 1=관리자 |
| last_seen | 문자열 | `YYYY-MM-DD HH:MM:SS` 또는 빈 값 |
| created_at | 문자열 | `YYYY-MM-DD HH:MM:SS` |

### 예시
```
alice//5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8//앨리스//안녕하세요!//1//0//2026-05-07 10:30:00//2026-04-01 09:00:00
bob//6b86b273ff34fce19d6b804eff5a3f5747ada4eaa22f1d49c01e52ddb7875b4b//밥//바쁩니다//2//0//2026-05-07 09:00:00//2026-04-02 10:00:00
```

### 로드/저장 함수 시그니처
```c
int  load_users(const char *path, UserRecord *out, int max_count);
void save_users(const char *path, UserRecord *list, int count);
int  append_user(const char *path, const UserRecord *u);
```

---

## 2. rooms.txt — 채팅방 정보

### 포맷
```
<id>//<name>//<topic>//<pw_hash>//<max_users>//<owner_id>//<notice>//<is_open>//<pinned_msg_id>//<created_at>
```

### 필드 정의

| 필드 | 타입 | 설명 |
|------|------|------|
| id | 정수 | 방 고유 ID (자동 증가) |
| name | 문자열 (최대 30자) | 방 이름 |
| topic | 문자열 (최대 100자) | 방 주제 (빈 값 허용) |
| pw_hash | 문자열 (64자) | 비밀번호 SHA-256 해시, 빈 문자열 = 공개 |
| max_users | 정수 | 최대 인원 (기본 10, 최대 64) |
| owner_id | 문자열 (최대 20자) | 방장 ID |
| notice | 문자열 (최대 255자) | 공지사항 (빈 값 허용) |
| is_open | 정수 | 0=그룹채팅, 1=오픈채팅 |
| pinned_msg_id | 정수 | 핀 메시지 ID (0 = 없음) |
| created_at | 문자열 | `YYYY-MM-DD HH:MM:SS` |

### 예시
```
1//개발팀////64af39e568...//10//alice//매주 월요일 9시 회의//0//0//2026-04-10 12:00:00
2//오픈채팅방//자유롭게 대화해요////20//bob////1//0//2026-04-11 09:00:00
```

### 로드/저장 함수 시그니처
```c
int  load_rooms(const char *path, RoomRecord *out, int max_count);
void save_rooms(const char *path, RoomRecord *list, int count);
int  append_room(const char *path, const RoomRecord *r);
void update_room(const char *path, RoomRecord *list, int count, int room_id);
```

---

## 3. messages.txt — 메시지

### 포맷
```
<id>//<room_id>//<from_id>//<to_id>//<reply_to>//<msg_type>//<is_deleted>//<created_at>//<edited_at>//<content>
```

> `content`는 content-last 규칙 적용 — 마지막 필드.

### 필드 정의

| 필드 | 타입 | 설명 |
|------|------|------|
| id | 정수 | 메시지 고유 ID (자동 증가) |
| room_id | 정수 | 채팅방 ID (0 = DM) |
| from_id | 문자열 (최대 20자) | 발신자 ID |
| to_id | 문자열 (최대 20자) | 수신자 ID (DM 전용, 빈 값 = 그룹) |
| reply_to | 정수 | 답장 원본 msg_id (0 = 없음) |
| msg_type | 정수 | 0=normal, 1=system, 2=whisper, 3=me-action |
| is_deleted | 정수 | 0=정상, 1=삭제됨 |
| created_at | 문자열 | `YYYY-MM-DD HH:MM:SS` |
| edited_at | 문자열 | `YYYY-MM-DD HH:MM:SS` 또는 빈 값 |
| content | 문자열 (최대 500자) | 메시지 본문 (마지막 필드) |

### 예시
```
1//1//alice////0//0//0//2026-05-07 10:30:00////안녕하세요 여러분!
2//1//bob////0//0//0//2026-05-07 10:31:00////반갑습니다 :smile:
3//0//alice//bob//0//0//0//2026-05-07 10:32:00////DM 테스트입니다
```

### 로드/저장 함수 시그니처
```c
int  load_messages(const char *path, MessageRecord *out, int max_count);
int  append_message(const char *path, const MessageRecord *m);
void update_message_deleted(const char *path, int msg_id);
void update_message_content(const char *path, int msg_id,
                            const char *new_content, const char *edited_at);
```

---

## 4. friends.txt — 친구 관계

### 포맷
```
<id>//<user_id>//<friend_id>//<status>//<created_at>
```

### 필드 정의

| 필드 | 타입 | 설명 |
|------|------|------|
| id | 정수 | 레코드 고유 ID |
| user_id | 문자열 (최대 20자) | 요청 발신자 ID (방향성 있음) |
| friend_id | 문자열 (최대 20자) | 요청 수신자 ID |
| status | 정수 | 친구 관계 상태 (아래 표 참조) |
| created_at | 문자열 | `YYYY-MM-DD HH:MM:SS` |

### status 값 정의

| status | 의미 | 발생 시점 | 효과 |
|--------|------|-----------|------|
| `0` | pending (요청 보냄) | `FRIEND_ADD_REQ` 처리 시 신규 레코드 추가 | 수신자가 수락/거절 전까지 대기 |
| `1` | accepted (친구 수락) | `FRIEND_ACCEPT` 처리 시 status 0→1 갱신 | 친구 목록·DM·온라인 상태 알림 활성 |
| `2` | blocked (차단) | `FRIEND_BLOCK` 처리 시 status →2 (기존 관계 무관) | FR-F05: 메시지·DM·친구요청 차단 |

> **거절(`FRIEND_REJECT`)**: 레코드를 `status=0`에 머무르게 두지 않고 **즉시 삭제**한다 (전체 재작성).  
> **삭제(`FRIEND_DELETE`)**: 양방향 레코드 전체 삭제. status=2(blocked) 차단 해제는 `FRIEND_DELETE`로 통일 — `status=0`(pending)은 차단 해제 후 재요청부터 다시 시작한다는 정책상 의미가 없으므로 사용하지 않는다.

### 예시
```
1//alice//bob//1//2026-04-15 09:00:00     # alice ↔ bob 친구 (수락 완료)
2//alice//charlie//0//2026-05-07 10:00:00 # alice → charlie 친구 요청 대기
3//dave//alice//2//2026-04-20 15:00:00    # dave가 alice를 차단
```

### 로드/저장 함수 시그니처
```c
int  load_friends(const char *path, FriendRecord *out, int max_count);
int  append_friend(const char *path, const FriendRecord *f);
void update_friend_status(const char *path, FriendRecord *list,
                          int count, int record_id, int new_status);
```

---

## 5. room_members.txt — 채팅방 멤버

### 포맷
```
<room_id>//<user_id>//<open_nick>//<is_admin>//<is_muted>//<joined_at>
```

### 필드 정의

| 필드 | 타입 | 설명 |
|------|------|------|
| room_id | 정수 | 채팅방 ID |
| user_id | 문자열 (최대 20자) | 멤버 ID |
| open_nick | 문자열 (최대 20자) | 오픈채팅 전용 닉네임 (빈 값 = 기본 닉네임 사용) |
| is_admin | 정수 | 0=일반, 1=공동 방장 |
| is_muted | 정수 | 0=알림 켜짐, 1=알림 무음 |
| joined_at | 문자열 | `YYYY-MM-DD HH:MM:SS` |

### 예시
```
1//alice////1//0//2026-04-10 12:00:00
1//bob////0//0//2026-04-10 12:05:00
2//charlie//오픈닉//0//0//2026-04-11 09:30:00
```

### 로드/저장 함수 시그니처
```c
int  load_room_members(const char *path, RoomMemberRecord *out, int max_count);
int  append_room_member(const char *path, const RoomMemberRecord *rm);
void remove_room_member(const char *path, RoomMemberRecord *list,
                        int *count, int room_id, const char *user_id);
void update_room_member_admin(const char *path, RoomMemberRecord *list,
                              int count, int room_id,
                              const char *user_id, int is_admin);
```

---

## 6. dm_reads.txt — DM 읽음 상태

### 포맷
```
<msg_id>//<reader_id>//<read_at>
```

### 필드 정의

| 필드 | 타입 | 설명 |
|------|------|------|
| msg_id | 정수 | 메시지 ID |
| reader_id | 문자열 (최대 20자) | 읽은 사용자 ID |
| read_at | 문자열 | `YYYY-MM-DD HH:MM:SS` |

### 예시
```
3//bob//2026-05-07 10:33:00
5//alice//2026-05-07 10:45:00
```

### 로드/저장 함수 시그니처
```c
int  load_dm_reads(const char *path, DmReadRecord *out, int max_count);
int  append_dm_read(const char *path, const DmReadRecord *dr);
int  is_dm_read(DmReadRecord *list, int count, int msg_id, const char *reader_id);
```

---

## 7. room_invites.txt — 채팅방 초대

### 포맷
```
<id>//<room_id>//<inviter_id>//<invitee_id>//<status>//<created_at>
```

### 필드 정의

| 필드 | 타입 | 설명 |
|------|------|------|
| id | 정수 | 레코드 고유 ID |
| room_id | 정수 | 채팅방 ID |
| inviter_id | 문자열 (최대 20자) | 초대한 사용자 ID |
| invitee_id | 문자열 (최대 20자) | 초대받은 사용자 ID |
| status | 정수 | 0=pending, 1=수락, 2=거절 |
| created_at | 문자열 | `YYYY-MM-DD HH:MM:SS` |

### 예시
```
1//1//alice//dave//0//2026-05-07 11:00:00
2//2//bob//eve//1//2026-05-06 14:00:00
```

### 로드/저장 함수 시그니처
```c
int  load_room_invites(const char *path, RoomInviteRecord *out, int max_count);
int  append_room_invite(const char *path, const RoomInviteRecord *ri);
void update_invite_status(const char *path, RoomInviteRecord *list,
                          int count, int invite_id, int new_status);
```

---

## 8. user_settings.txt — 유저 설정

### 포맷
```
<user_id>//<msg_color>//<nick_color>//<theme>//<ts_format>//<dnd>//<welcome_shown>
```

### 필드 정의

| 필드 | 타입 | 설명 |
|------|------|------|
| user_id | 문자열 (최대 20자) | 유저 ID |
| msg_color | 문자열 (최대 15자) | 메시지 색상 (예: cyan, white, green) |
| nick_color | 문자열 (최대 15자) | 닉네임 색상 (예: yellow, magenta) |
| theme | 문자열 (최대 10자) | dark 또는 light |
| ts_format | 정수 | 0=HH:MM, 1=HH:MM:SS, 2=MM-DD HH:MM |
| dnd | 정수 | 0=알림 켜짐, 1=방해금지 모드 |
| welcome_shown | 정수 | 첫 로그인 환영 안내 표시 완료 여부 (0=미표시, 1=표시 완료) |

> `welcome_shown` 필드는 `help_and_guides.md §5` (초기 사용자 가이드) 표시 1회 정책에 사용된다. 회원가입 직후 처음 로그인 시 0으로 시작 → 가이드 표시 후 1로 갱신.

### 예시
```
alice//cyan//yellow//dark//0//0//1
bob//white//green//light//1//0//1
charlie//magenta//cyan//dark//2//1//0
```

### 로드/저장 함수 시그니처
```c
int  load_user_settings(const char *path, UserSettingsRecord *out, int max_count);
int  upsert_user_settings(const char *path, UserSettingsRecord *list,
                          int *count, const UserSettingsRecord *s);
int  get_user_settings(UserSettingsRecord *list, int count,
                       const char *user_id, UserSettingsRecord *out);
```

---

## 9. room_reads.txt — 그룹/오픈채팅방 읽음 상태 (FR-D05·FR-G09)

`dm_reads.txt`가 1:1 DM 메시지 단위 읽음 기록인 것과 달리, 그룹/오픈채팅방은 **사용자별 마지막으로 읽은 msg_id만 저장**한다. 매 메시지마다 읽음 레코드를 쌓으면 수만 건 단위로 폭증하므로 (room_id, user_id) 페어당 1건만 유지한다.

### 포맷
```
<room_id>//<user_id>//<last_read_msg_id>//<read_at>
```

### 필드 정의

| 필드 | 타입 | 설명 |
|------|------|------|
| room_id | 정수 | 채팅방 ID |
| user_id | 문자열 (최대 20자) | 사용자 ID |
| last_read_msg_id | 정수 | 사용자가 마지막으로 읽은 messages.txt 의 msg_id |
| read_at | 문자열 | `YYYY-MM-DD HH:MM:SS` |

### 안읽음 카운트 산출

```c
/* (room_id, user_id) 페어의 안읽음 메시지 수 */
int get_unread_room_count(int room_id, const char *user_id) {
    int last_read = 0;
    for (int i = 0; i < g_room_read_count; i++) {
        if (g_room_reads[i].room_id == room_id &&
            strcmp(g_room_reads[i].user_id, user_id) == 0) {
            last_read = g_room_reads[i].last_read_msg_id;
            break;
        }
    }
    int unread = 0;
    for (int i = 0; i < g_msg_count; i++) {
        if (g_messages[i].room_id == room_id &&
            g_messages[i].id > last_read &&
            !g_messages[i].is_deleted) {
            unread++;
        }
    }
    return unread;
}
```

### 갱신 시점
- `ROOM_JOIN_RES`(P0) 응답 직후 — 입장과 동시에 마지막 메시지까지 읽음 처리.
- `ROOM_MSG_RECV`(P0) 수신 시 — 사용자가 현재 보고 있는 방이면 즉시 갱신.
- `ROOM_LEAVE`(P0) 처리 시 — 마지막 본 시점 보존.

### 예시
```
1//alice//42//2026-05-07 10:30:00
1//bob//40//2026-05-07 10:25:00
2//charlie//100//2026-05-07 11:00:00
```

### 로드/저장 함수 시그니처
```c
int  load_room_reads(const char *path, RoomReadRecord *out, int max_count);
int  upsert_room_read(const char *path, RoomReadRecord *list,
                      int *count, int room_id, const char *user_id,
                      int last_read_msg_id);
int  get_unread_room_count(int room_id, const char *user_id);
```

> 표시 상한: 클라이언트는 99개 초과 시 `[안읽음 99+]` 으로 캡한다 (콘솔 폭 제한).

---

## 10. 파일 없을 때 처리

서버 시작 시 데이터 파일이 존재하지 않아도 정상 동작해야 한다.

```c
// 올바른 처리 (fopen NULL 체크 필수)
int load_users(const char *path, UserRecord *out, int max_count) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        // 파일 없음 -> 빈 배열로 시작 (정상)
        return 0;
    }
    int count = 0;
    char line[512];
    while (fgets(line, sizeof(line), fp) && count < max_count) {
        // 파싱 ...
        count++;
    }
    fclose(fp);
    return count;
}

// 잘못된 처리 (절대 금지 - 세그폴트 발생)
// FILE *fp = fopen("users.txt", "r");
// while (!feof(fp)) { ... }  // fp가 NULL이면 크래시!
```
