# FR-G: 그룹 채팅방

## FR-G01 — 채팅방 생성

### 제약조건

| 필드 | 최대 길이 | 금지 문자 | 기본값 |
|------|-----------|-----------|--------|
| 방 이름 | 30자 | `:` `;` `\|` `\n` | 필수 |
| 주제 | 100자 | `\n` `;` `\|` | 빈 값 |
| 최대 인원 | — | — | 10명 (최대 64명) |
| 비밀번호 | 10자 | `:` `;` `\|` `\n` | 없음 (공개) |

### 패킷

```
C→S  ROOM_CREATE|<name>:<max_users>[:<is_open>[:<password>[:<topic>]]]
S→C  ROOM_CREATE_RES|1:<room_id>   (성공)
     ROOM_CREATE_RES|0:0            (실패 — MAX_ROOMS 초과 등)
```

### 콘솔 입력 화면

```
============================================================
    Create New Chat Room
============================================================
> 방 이름 (최대 30자): 개발팀
> 최대 인원 (기본 10, 최대 64): 20
> 비밀번호 (없으면 Enter): ****
> 방 주제 (선택, 최대 100자): C 프로젝트 팀

> [성공] 방 생성 완료 (ID: 3)
```

---

## FR-G02 — 멤버 초대

```
C→S  ROOM_INVITE|<room_id>:<target_id>
S→C  ROOM_INVITE_RES|0   (SENT)
     ROOM_INVITE_RES|1   (NOT_FOUND)
     ROOM_INVITE_RES|2   (ALREADY_MEMBER)
     ROOM_INVITE_RES|3   (ROOM_FULL)
```

- 대상이 온라인: `ROOM_INVITE_NOTIFY` 즉시 전송
- 대상이 오프라인: `room_invites.txt`에 pending 저장 → 로그인 시 알림

### 다중 초대 처리

`dialogs_and_actions.md` §11에서 친구 다수를 쉼표로 구분 입력 가능하지만 (`> 선택: 1,3,5`), `ROOM_INVITE` 패킷은 단건 전용이다 (`<target_id>` 한 명).

**처리 방식**: 클라이언트가 선택된 ID 수만큼 `ROOM_INVITE` 패킷을 **반복 전송**한다. 서버는 패킷별로 독립 처리하며 응답은 결과 코드별로 누적한다.

```c
/* menu_chat.c — 다중 초대 명령 처리 */
void cmd_invite_multi(int room_id, const char *id_list_csv) {
    char buf[256];
    safe_strcpy(buf, id_list_csv, sizeof(buf));

    int sent = 0, fail = 0;
    char *tok = strtok(buf, ",");
    while (tok) {
        trim(tok);
        if (tok[0] != '\0') {
            send_packet(g_state.sock, "ROOM_INVITE|%d:%s", room_id, tok);
            /* RecvMsg 스레드가 ROOM_INVITE_RES 수신 시 카운트 */
            sent++;
        }
        tok = strtok(NULL, ",");
    }
    printf("> 초대 요청 %d건 전송. 결과는 알림으로 표시됩니다.\n", sent);
}
```

서버는 한 트랜잭션에서 여러 초대를 받더라도 각각 `ROOM_FULL` 검사를 다시 수행한다 (마지막 한 자리에 동시에 초대된 경우 첫 처리만 SENT, 나머지 ROOM_FULL 반환).

> 패킷 자체를 다중화하지 않은 이유: `target_id`가 자유 텍스트가 아니므로 `;` 리스트 구분이 가능하지만, 응답 코드가 대상별로 다를 수 있어 단건 응답이 단순함. 또한 `MAX_PKT_SIZE=10240` 제약 내에서 친구 5명 초대도 5번의 작은 패킷이 효율적.

---

## FR-G03 — 메시지 전송 및 브로드캐스트

```
C→S  ROOM_MSG|<room_id>:<content>
S→C  ROOM_MSG_RECV|<room_id>:<from_nick>:<timestamp>:<msg_id>:<reply_to_id>:<msg_type>:<content>
```

서버 처리 순서:
1. `content`에서 이모티콘 변환 (`convert_emoticons()`)
2. `/me` 감지 시 `msg_type=3` 설정
3. `messages.txt`에 저장
4. `broadcast_to_room(room_id, packet)` 호출

P0 브로드캐스트 (전체 전송):
- `broadcast_to_all()` 사용
- 클라이언트가 `room_id` 확인 후 현재 방 메시지만 출력

P1 이후 브로드캐스트 (방별 격리):
- `broadcast_to_room()` 사용

---

## FR-G04 — 멘션(@)

`@닉네임` 포함 메시지는 일반 `ROOM_MSG_RECV`로 브로드캐스트되며, 클라이언트가 자신의 닉네임이 멘션되었는지 확인하여 강조 표시한다.

```c
/* packet_parse()에서 ROOM_MSG_RECV 수신 시 */
char mention[25];
snprintf(mention, sizeof(mention), "@%s", g_state.nickname);
if (strstr(content, mention) != NULL) {
    printf("\n[멘션] %s: %s\n> ", from_nick, content);
} else {
    printf("[%s] %s: %s\n", timestamp, from_nick, content);
}
```

---

## FR-G05 — 채팅방 나가기

```
C→S  ROOM_LEAVE|<room_id>
     (응답 없음)
```

서버 처리:
1. 퇴장 시스템 메시지 브로드캐스트 (`[시스템] 홍길동 님이 퇴장했습니다.`)
2. `room_members.txt`에서 제거
3. `g_rooms[]`의 `member_ids[]`에서 제거
4. 마지막 멤버가 나가면 방 자동 삭제

---

## FR-G06 — 방장 권한

방장(`owner_id`)만 가능한 작업:

| 작업 | 패킷 |
|------|------|
| 멤버 강퇴 | `ROOM_KICK\|<room_id>:<target_id>` |
| 방 삭제 | `ROOM_DELETE\|<room_id>` |
| 공지 등록 | `ROOM_SET_NOTICE\|<room_id>:<notice>` |
| 핀 메시지 설정 | `MSG_PIN\|<room_id>:<msg_id>` |
| 공동 방장 부여 | `ROOM_GRANT_ADMIN\|<room_id>:<target_id>` |
| 공동 방장 해제 | `ROOM_REVOKE_ADMIN\|<room_id>:<target_id>` |

공동 방장도 강퇴 및 공지 등록 가능. 방 삭제와 방장 부여/해제는 원래 방장만 가능.

---

## FR-G07 — 공동 방장

```
C→S  ROOM_GRANT_ADMIN|<room_id>:<target_id>   (방장 전용)
C→S  ROOM_REVOKE_ADMIN|<room_id>:<target_id>  (방장 전용)
```

`room_members.txt`의 `is_admin` 필드:
- 0 = 일반 멤버
- 1 = 공동 방장

멤버 목록 표시:
```
  1. [방장] alice   [ON ]
  2. [관리자] bob   [OFF]
  3. charlie        [ON ]
```

---

## FR-G08 — 공지사항 등록/조회

```
C→S  ROOM_SET_NOTICE|<room_id>:<notice>
     (응답 없음 — 브로드캐스트로 확인)
```

- `rooms.txt`의 `notice` 필드 갱신
- 방 전체에 `ROOM_NOTICE|<room_id>:<notice>` 브로드캐스트
- 새로 입장하는 멤버에게는 `ROOM_JOIN_RES` 직후 서버가 자동 push

콘솔 표시:
```
------------------------------------------------------------
    공지: 매주 월요일 9시 회의 있습니다.
------------------------------------------------------------
```

---

## FR-G09 — 메시지 히스토리

채팅방 입장 시 최근 100개 메시지 자동 요청 (P1).

```
C→S  ROOM_HISTORY_REQ|<room_id>:100
S→C  ROOM_HISTORY_RES|<count>:<msg_id>:<from_nick>:<timestamp>:<reply_to_id>:<msg_type>:<content>;<msg_id>:...
```

- 오픈채팅에서는 `from_nick`이 `open_nick` 우선
- `is_deleted=1` 메시지는 `[삭제된 메시지]`로 대체하여 전송

---

## FR-G10 — 멤버 목록 조회

```
/members 명령어 입력
C→S  ROOM_MEMBERS_REQ|<room_id>
S→C  ROOM_MEMBERS_RES|<room_id>:<id>:<nick>:<is_admin>:<online>;<id>:...
```

콘솔 출력:
```
============================================================
    [멤버 목록] 개발팀 (3/20명)
============================================================
  [방장] alice   (홍길동)   [ON ]
  [관리자] bob   (김철수)   [OFF]
           charlie (박민준) [ON ]
============================================================
```
