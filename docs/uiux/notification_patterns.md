# 알림 표시 패턴

비동기 수신 메시지(RecvMsg 스레드)가 메인 스레드의 입력 프롬프트 위에 출력되는 패턴을 정의한다. 모든 알림은 `\n` 선출력 → 알림 → `> 프롬프트` 재출력 형식을 따른다.

---

## 1. 공통 출력 패턴

```c
/* 모든 알림은 다음 매크로로 출력 — 입력 프롬프트와 충돌 방지 */
#define NOTIFY_OUT(fmt, ...) do { \
    printf("\n" fmt "\n> ", ##__VA_ARGS__); \
    fflush(stdout); \
} while (0)
```

색상 적용 매크로:
```c
#define NOTIFY_HIGHLIGHT(color, fmt, ...) do { \
    printf("\n" color fmt COLOR_RESET "\n> ", ##__VA_ARGS__); \
    fflush(stdout); \
} while (0)
```

---

## 2. 알림 유형별 출력 형식

### 2-1. 메시지 알림 (FR-N01)

다른 방의 메시지가 도착했을 때:
```
[알림] 채팅방 #3 (개발팀): 김철수: 회의 시작합니다!
> 메시지 입력:
```

DM 화면 밖에서 DM이 도착했을 때:
```
[DM] 김철수: 잠깐 시간 되세요?
> 메시지 입력:
```

### 2-2. 친구 요청 (FR-N02)

```
[친구 요청] alice (홍길동) 님이 친구 요청을 보냈습니다.
            → 친구 목록 → b. 친구 요청 목록 에서 수락/거절
> 메시지 입력:
```

### 2-3. 친구 수락

```
[친구] bob (김철수) 님이 친구 요청을 수락했습니다!
> 메시지 입력:
```

### 2-4. 친구 상태 변경 (FR-F06)

```
[상태] 김철수 [ON ] (로그인)
[상태] 이영희 [OFF] (로그아웃)
[상태] 박민준 [바쁨] (상태 변경)
```

### 2-5. 멘션 (FR-N03) — 강조 (DND에서도 표시)

```
[멘션] ⚡ 김철수: @홍길동 오늘 회의 자료 확인하셨나요?
       → 채팅방 #3 (개발팀)
> 메시지 입력:
```

색상 적용 (노란색 굵게):
```c
NOTIFY_HIGHLIGHT(COLOR_BOLD COLOR_YELLOW,
    "[멘션] ⚡ %s: %s\n       → 채팅방 #%d (%s)",
    from_nick, content, room_id, room_name);
```

### 2-6. 귓속말 수신 (FR-M01)

```
[귓속말] 김철수 → 나: 잠깐 얘기 좀 할게요
> 메시지 입력:
```

색상 (자홍색):
```c
NOTIFY_HIGHLIGHT(COLOR_MAGENTA, "[귓속말] %s → 나: %s",
                 from_nick, content);
```

### 2-7. 채팅방 초대 (FR-G02)

```
[초대] 이영희 (leeya) 님이 방 "스터디 그룹"에 초대했습니다.
       → 메인 로비 → 알림 처리 가능
> 메시지 입력:
```

### 2-8. 강퇴 (FR-G06)

```
[강퇴] ⚠ 방 "개발팀"에서 강퇴되었습니다. (by 방장 alice)
       → 메인 로비로 자동 복귀
```

### 2-9. 방 삭제 (FR-G06)

```
[방 삭제] ⚠ 방 "개발팀"이 삭제되었습니다.
          → 메인 로비로 자동 복귀
```

### 2-10. 공지 변경 (FR-G08)

```
[공지 변경] 방 "개발팀"
            새 공지: 다음 회의는 수요일 오후 2시입니다.
```

### 2-11. 핀 메시지 변경 (FR-M10)

```
[핀 고정] alice가 메시지 #58를 핀으로 고정했습니다.
          미리보기: "다음 회의 수요일 14시"
```

### 2-12. 메시지 삭제 (FR-M02)

```
[알림] 메시지 #42가 삭제되었습니다.
       → 채팅방 화면에서 [삭제된 메시지]로 표시됩니다.
```

### 2-13. 메시지 수정 (FR-M03)

```
[알림] 메시지 #42가 수정되었습니다.
       → 화면에서 (수정됨) 표시가 추가됩니다.
```

### 2-14. DM 읽음 (FR-D03)

자체적인 다이얼로그는 없고, DM 대화 화면 안에서 인라인 표시:
```
[14:22] 나: 오늘 시간 있으세요?              [읽음]
[14:23] 나: 답장 부탁드려요                  [안읽음]
```

DM 화면 밖에서 읽음 알림 수신 시:
```
[DM] 김철수가 마지막 메시지를 읽었습니다.
```

### 2-15. 타이핑 표시 (FR-N05)

채팅방 내부 (현재 보고 있는 방):
```
[홍길동 님이 입력 중...]
```

다중 사용자:
```
[홍길동, 김철수 님이 입력 중...]      ← 2명
[홍길동 외 2명이 입력 중...]          ← 3명 이상
```

`\r`로 같은 줄을 덮어쓰는 방식:
```c
void show_typing(const char **nicks, int count) {
    if (count == 0) {
        printf("\r%-60s\r> ", "");  /* 줄 지우기 */
    } else if (count == 1) {
        printf("\r[%s 님이 입력 중...]%-30s\r> ", nicks[0], "");
    } else if (count == 2) {
        printf("\r[%s, %s 님이 입력 중...]%-30s\r> ",
               nicks[0], nicks[1], "");
    } else {
        printf("\r[%s 외 %d명이 입력 중...]%-30s\r> ",
               nicks[0], count - 1, "");
    }
    fflush(stdout);
}
```

### 2-16. 시스템 메시지 (FR-M07)

채팅방 내부에 표시 (회색 또는 대시 구분선):
```
--- alice 님이 입장했습니다. ---
--- bob 님이 퇴장했습니다. ---
--- charlie 님이 강퇴되었습니다. (by alice) ---
--- alice가 charlie 님을 초대했습니다. ---
--- 새 공지가 등록되었습니다: "다음 회의 수요일" ---
--- bob 님이 공동 방장이 되었습니다. ---
--- charlie 님이 공동 방장에서 해제되었습니다. ---
--- 메시지 #58이 핀으로 고정되었습니다. ---
--- 메시지 #58의 핀이 해제되었습니다. ---
```

색상:
```c
printf(COLOR_GRAY "--- %s ---" COLOR_RESET "\n", system_msg);
```

### 2-17. 서버 공지 (NOTIFY|SERVER:...)

```
[서버 공지] 서버 점검 안내: 오늘 23시 ~ 24시 점검이 진행됩니다.
```

색상 (빨간 배경 또는 굵은 빨강):
```c
NOTIFY_HIGHLIGHT(COLOR_BOLD COLOR_RED, "[서버 공지] %s", content);
```

---

## 3. 알림 필터링 로직 (DND, 무음방, 멘션 우선)

```c
/* 알림 출력 전 공통 검사 */
int should_notify(int room_id, int is_mention, int is_dm) {
    /* 멘션은 항상 표시 (DND 무관) */
    if (is_mention) return 1;

    /* DND 모드 → 차단 (멘션 제외) */
    if (g_state.dnd) return 0;

    /* 방 무음 → 차단 */
    if (room_id > 0 && is_room_muted(room_id)) return 0;

    /* DM은 DND가 아니면 항상 표시 */
    if (is_dm) return 1;

    return 1;
}

/* ROOM_MSG_RECV 처리 */
void on_room_msg_recv(int room_id, const char *from_nick,
                     const char *timestamp, const char *content) {
    int is_mention = check_mention(content);

    if (room_id == g_state.current_room_id) {
        /* 현재 보는 방 → 화면 출력 */
        display_chat_message(from_nick, timestamp, content);
        if (is_mention) ring_bell();  /* 옵션: \a 출력 */
    } else if (should_notify(room_id, is_mention, 0)) {
        if (is_mention) {
            NOTIFY_HIGHLIGHT(COLOR_BOLD COLOR_YELLOW,
                "[멘션] ⚡ %s: %s\n       → 채팅방 #%d",
                from_nick, content, room_id);
        } else {
            NOTIFY_OUT("[알림] 채팅방 #%d: %s: %s",
                       room_id, from_nick, content);
        }
    }
    /* DND 또는 무음방이면 무시 */
}
```

---

## 4. 채팅방 알림 무음 (FR-N06) 동작 매트릭스

| 상황 | 일반 메시지 | 멘션 | DM | 친구요청 |
|------|-------------|------|-----|----------|
| 평상시 | 표시 | 강조 표시 | 표시 | 표시 |
| 방 무음 | 차단 | 강조 표시 | 표시 | 표시 |
| DND 모드 | 차단 | 강조 표시 | 차단 | 차단 |
| DND + 방 무음 | 차단 | 강조 표시 | 차단 | 차단 |

---

## 5. 알림 시각적 우선순위 (색상)

| 알림 유형 | 색상 | 강조 |
|-----------|------|------|
| 시스템 메시지 | 회색 | `---` 구분 |
| 일반 알림 | 기본 | 없음 |
| 친구 상태 변경 | 청록색 (cyan) | 없음 |
| 친구 요청/수락 | 녹색 (green) | 없음 |
| 귓속말 | 자홍색 (magenta) | 없음 |
| 멘션 | 노란색 (yellow) | **굵게** + ⚡ |
| 강퇴/방삭제 | 빨간색 (red) | ⚠ 표시 |
| 서버 공지 | 굵은 빨강 | **강조** |
| 오류 | 빨간색 | `[오류]` 접두 |

---

## 6. 비프음 (Bell) 활용

```c
/* '\a' 또는 0x07 출력 시 콘솔이 비프음 발생 */
void ring_bell(void) {
    putchar('\a');
    fflush(stdout);
}

/* 멘션 수신 시 1회, 강퇴 시 2회 */
ring_bell();
Sleep(200);
ring_bell();
```

활성화 조건:
- 멘션 수신
- 강퇴/방삭제 알림
- 서버 공지

설정에서 비활성화 가능 (선택 구현).

---

## 7. RecvMsg 스레드 ↔ 메인 스레드 안전 출력

콘솔 출력은 단일 스레드에서만 안전하다. 출력 충돌 방지 패턴:

```c
HANDLE g_console_mutex;  /* main()에서 CreateMutex로 생성 */

void safe_print(const char *fmt, ...) {
    WaitForSingleObject(g_console_mutex, INFINITE);
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    fflush(stdout);
    ReleaseMutex(g_console_mutex);
}

/* 메인 스레드 입력도 mutex 보호 권장 */
void safe_prompt_and_input(char *buf, int size) {
    WaitForSingleObject(g_console_mutex, INFINITE);
    printf("> 메시지 입력: ");
    fflush(stdout);
    ReleaseMutex(g_console_mutex);
    fgets(buf, size, stdin);
}
```

---

## 8. 알림 재출력 패턴 (입력 도중 알림)

사용자가 메시지를 입력 중일 때 알림이 도착하면, 이미 입력된 내용을 보존해야 한다.

**문제**:
```
> 메시지 입력: 안녕하세      ← 사용자 입력 중
[알림] 김철수: 도착!         ← 알림이 입력 위에 출력
> 메시지 입력:               ← 입력 내용 사라짐
```

**해결책 1**: 입력 버퍼 추적 후 재출력
```c
char g_input_buffer[BUF_SIZE] = "";
int  g_input_pos = 0;

void on_async_notify(const char *msg) {
    WaitForSingleObject(g_console_mutex, INFINITE);
    printf("\r%-60s\r%s\n> 메시지 입력: %s",
           "", msg, g_input_buffer);
    fflush(stdout);
    ReleaseMutex(g_console_mutex);
}
```

**해결책 2**: 별도 알림 영역 (Windows Console API로 cursor 제어) — 복잡도 ↑

P0~P2에서는 단순화: 알림 출력 후 빈 프롬프트만 다시 표시 (사용자가 입력 중인 내용은 재입력).
