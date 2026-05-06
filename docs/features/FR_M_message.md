# FR-M: 메시지 기능

## FR-M01 — 귓속말

```
C→S  WHISPER|<to_nick>:<content>
S→C  WHISPER_RECV|<from_nick>:<timestamp>:<content>
```

- 닉네임 기반으로 대상 세션 검색 (닉네임은 고유)
- 대상이 오프라인이면 조용히 무시 (또는 오류 알림)
- `msg_type=2`로 messages.txt에 저장 (선택)
- 채팅방 내부 명령어: `/w <닉네임> <내용>`

콘솔 표시:
```
[귓속말] 김철수 → 나: 잠깐 얘기 좀 할게요
```

---

## FR-M02 — 메시지 삭제

```
C→S  MSG_DELETE|<room_id>:<msg_id>
S→C  MSG_DELETED_NOTIFY|<room_id>:<msg_id>   (방 전체 브로드캐스트)
```

- 본인 메시지 또는 방장/관리자만 삭제 가능
- `messages.txt`에서 `is_deleted=1`로 변경 (물리 삭제 아님)
- 클라이언트는 `MSG_DELETED_NOTIFY` 수신 시 해당 `msg_id` 메시지를 `[삭제된 메시지]`로 교체

콘솔 명령어: `/del <msg_id>`

클라이언트 처리:
```c
else if (strcmp(type, "MSG_DELETED_NOTIFY") == 0) {
    int room_id = atoi(strtok(payload, ":"));
    int msg_id  = atoi(strtok(NULL, ":"));
    if (room_id == g_state.current_room_id) {
        printf("[알림] 메시지 #%d가 삭제되었습니다.\n", msg_id);
    }
}
```

---

## FR-M03 — 메시지 수정

```
C→S  MSG_EDIT|<room_id>:<msg_id>:<new_content>
S→C  MSG_EDITED_NOTIFY|<room_id>:<msg_id>:<new_content>   (방 전체 브로드캐스트)
```

- **5분(300초) 이내**만 수정 가능
- 본인 메시지만 수정 가능
- `messages.txt`에서 `content` 갱신, `edited_at` 설정
- 클라이언트 표시: `내용 (수정됨)`

콘솔 명령어: `/edit <msg_id> <새내용>`

시간 초과 시 서버 응답:
```
NOTIFY|SERVER:수정 가능 시간(5분)이 초과되었습니다.
```

---

## FR-M04 — 답장(인용) (P3)

```
C→S  MSG_REPLY|<room_id>:<reply_to_id>:<content>
```

서버는 `reply_to_id`를 포함한 `ROOM_MSG_RECV`로 브로드캐스트한다:
```
S→C  ROOM_MSG_RECV|<room_id>:<from_nick>:<timestamp>:<msg_id>:<reply_to_id>:<msg_type>:<content>
```

클라이언트는 `reply_to_id > 0`이면 로컬 캐시에서 원문을 찾아 인용 표시한다:
```
[14:35] 홍길동:
    > 김철수: 오늘 회의 자료 준비됐나요?  ← reply_to 인용
  네, 준비됐습니다!
```

콘솔 명령어: `/reply <msg_id> <내용>`

---

## FR-M05 — 리액션 (Out-of-Scope)

> FR-M05 리액션 기능은 현재 범위에서 제외되었습니다. v2.1 이후 별도 구현.

~~`/react <msg_id> <이모지>`~~ — 구현 없음.

---

## FR-M06 — 이모티콘 변환

서버에서 메시지 저장 전 `convert_emoticons()` 호출로 텍스트 이모티콘을 ASCII 아트로 변환한다.

### 변환 테이블

| 입력 | 출력 |
|------|------|
| `:smile:` | `(^_^)` |
| `:heart:` | `<3` |
| `:sad:` | `(T_T)` |
| `:laugh:` | `(^o^)` |
| `:wink:` | `(^_-)` |
| `:angry:` | `(-_-)` |
| `:cool:` | `(B_B)` |
| `:shock:` | `(O_O)` |
| `:shy:` | `(>_<)` |
| `:sweat:` | `(^_^;)` |
| `:lol:` | `(LoL)` |
| `:wave:` | `( ^_^)/` |

---

## FR-M07 — 시스템 메시지

`msg_type=1`인 메시지는 시스템 메시지로 처리한다. 서버가 특정 이벤트 발생 시 자동 생성한다.

| 이벤트 | 시스템 메시지 |
|--------|--------------|
| 입장 | `홍길동 님이 입장했습니다.` |
| 퇴장 | `홍길동 님이 퇴장했습니다.` |
| 강퇴 | `홍길동 님이 강퇴되었습니다.` |
| 초대 | `김철수 님이 홍길동 님을 초대했습니다.` |
| 공지 변경 | `[공지] 새 공지사항이 등록되었습니다.` |
| 방장 변경 | `홍길동 님이 새 방장이 되었습니다.` |

콘솔 표시 (회색 또는 대시 구분선 형식):
```
--- 홍길동 님이 입장했습니다. ---
```

---

## FR-M08 — 메시지 검색 (P3)

```
C→S  MSG_SEARCH|<room_id>:<keyword>
S→C  MSG_SEARCH_RES|<count>:<msg_id>:<from_nick>:<timestamp>:<content>;<msg_id>:...
```

콘솔 명령어: `/search <키워드>`

출력 예:
```
============================================================
    [검색 결과] "회의" — 3건
============================================================
  [14:30] 홍길동: 오늘 회의 자료 준비됐나요? (#42)
  [15:00] 김철수: 회의 시간이 변경되었습니다. (#58)
  [16:20] 이영희: 다음 회의는 수요일입니다. (#71)
```

---

## FR-M09 — 타임스탬프

모든 메시지에 타임스탬프를 표시한다. 형식은 설정(`ts_format`)에 따라 다르다.

| ts_format | 형식 | 예시 |
|-----------|------|------|
| 0 | `HH:MM` | `14:30` |
| 1 | `HH:MM:SS` | `14:30:25` |
| 2 | `MM-DD HH:MM` | `05-07 14:30` |

```c
void format_timestamp(const char *raw, char *out, int out_len, int ts_format) {
    /* raw: "YYYY-MM-DD HH:MM:SS" */
    switch (ts_format) {
        case 0:
            /* HH:MM */
            strncpy(out, raw + 11, 5);
            out[5] = '\0';
            break;
        case 1:
            /* HH:MM:SS */
            strncpy(out, raw + 11, 8);
            out[8] = '\0';
            break;
        case 2:
            /* MM-DD HH:MM */
            strncpy(out, raw + 5, 11);
            out[11] = '\0';
            break;
        default:
            strncpy(out, raw + 11, 5);
            out[5] = '\0';
    }
}
```

---

## FR-M10 — 핀 메시지 (P3)

```
C→S  MSG_PIN|<room_id>:<msg_id>       (방장/관리자 전용)
S→C  MSG_PIN_NOTIFY|<room_id>:<msg_id>:<from_nick>:<content_preview>
```

- `rooms.txt`의 `pinned_msg_id` 필드 갱신
- 방 전체에 `MSG_PIN_NOTIFY` 브로드캐스트
- 새로 입장하는 유저에게는 `ROOM_JOIN_RES` 직후 `ROOM_PIN` push

콘솔 명령어: `/pin <msg_id>`

채팅방 상단 핀 표시:
```
============================================================
    [채팅방: 개발팀] (3/10명)
============================================================
    [PIN] 홍길동: 매주 월요일 9시 회의가 있습니다.
------------------------------------------------------------
```

---

## FR-M11 — /me 액션 (P3)

```
C→S  ROOM_MSG|<room_id>:/me <동작>
```

서버에서 `/me ` 접두사 감지 시 `msg_type=3`으로 저장하고 브로드캐스트한다.

콘솔 명령어: `/me <동작>`

입력 예시: `/me 손을 흔든다`

콘솔 표시:
```
* 홍길동 손을 흔든다
```

클라이언트 처리:
```c
case 3:  /* me-action */
    /* content에 이미 닉네임 없이 동작만 포함되어 있음 */
    printf("* %s %s\n", from_nick, content);
    break;
```
