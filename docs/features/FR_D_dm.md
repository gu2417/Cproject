# FR-D: 1:1 DM (다이렉트 메시지)

## FR-D01 — DM 시작

DM은 별도의 방을 생성하지 않는다. `messages.txt`에서 `room_id=0`이고 `from_id`/`to_id`로 대화 상대를 식별한다.

### 개설 방법

1. 친구 목록에서 번호 선택 → DM 대화 화면 진입
2. DM 목록에서 `n` 입력 → 상대방 ID 직접 입력 → DM 대화 화면 진입

---

## FR-D02 — 메시지 전송

```
C→S  DM_SEND|<to_id>:<content>
```

- 최대 500자
- 이모티콘 변환 적용 (서버에서 처리)
- 수신자가 오프라인이면 messages.txt에 저장만 하고 전달 보류
- 수신자가 온라인이면 즉시 `DM_RECV` 전송

```
S→C  DM_RECV|<from_id>:<from_nick>:<timestamp>:<msg_id>:<content>
```

클라이언트 수신 처리:
```c
/* packet_parse()에서 DM_RECV 처리 */
else if (strcmp(type, "DM_RECV") == 0) {
    char *from_id   = strtok(payload, ":");
    char *from_nick = strtok(NULL, ":");
    char *timestamp = strtok(NULL, ":");
    int   msg_id    = atoi(strtok(NULL, ":"));
    char *content   = strtok(NULL, "");  /* content-last */

    if (g_state.current_dm_partner &&
        strcmp(g_state.current_dm_partner, from_id) == 0) {
        /* 현재 해당 DM 화면에 있으면 즉시 출력 */
        printf("[%s] %s: %s\n> ", timestamp, from_nick, content);
        /* 읽음 처리 */
        /* send_packet(g_state.sock, "DM_READ_ACK|%d", msg_id); */
    } else if (!g_state.dnd) {
        /* 다른 화면에 있으면 알림 */
        printf("\n[DM] %s: %s\n> ", from_nick, content);
    }
    fflush(stdout);
}
```

---

## FR-D03 — 읽음 확인

DM 대화 화면 진입 시 또는 메시지 수신 시 `dm_reads.txt`에 읽음 기록을 추가한다.

```
S→C  DM_READ_NOTIFY|<reader_id>
```

클라이언트 표시:
```
[14:22] 홍길동: 오늘 시간 있으세요?      [읽음]
[14:23] 홍길동: 언제 답장 주실 건가요?
```

구현 방식:
- 발신한 메시지 옆에 `[읽음]` 표시
- `DM_READ_NOTIFY` 수신 시 해당 메시지에 `[읽음]` 갱신
- 콘솔에서는 재출력 어려우므로 다음 메시지 출력 시 "이전 메시지 읽힘" 형태로 표시

---

## FR-D04 — 메시지 히스토리

DM 대화 화면 진입 시 최근 50개 메시지를 자동 요청한다.

```
C→S  DM_HISTORY_REQ|<with_id>:50
S→C  DM_HISTORY_RES|<count>:<msg_id>:<from_id>:<timestamp>:<read>:<content>;<msg_id>:...
```

클라이언트 처리:
```c
void request_dm_history(SOCKET sock, const char *with_id) {
    send_packet(sock, "DM_HISTORY_REQ|%s:50", with_id);
    /* RecvMsg 스레드에서 DM_HISTORY_RES 수신 후 화면 출력 */
}

/* packet_parse()에서 DM_HISTORY_RES 처리 */
else if (strcmp(type, "DM_HISTORY_RES") == 0) {
    int count = atoi(strtok(payload, ":"));
    for (int i = 0; i < count; i++) {
        int   msg_id    = atoi(strtok(NULL, ":"));
        char *from_id   = strtok(NULL, ":");
        char *timestamp = strtok(NULL, ":");
        int   read      = atoi(strtok(NULL, ":"));
        char *content   = strtok(NULL, i < count-1 ? ";" : "");
        /* 닉네임 표시: from_id가 본인이면 "나", 아니면 상대방 닉네임 */
        const char *nick = strcmp(from_id, g_state.user_id) == 0
                           ? "나" : g_state.current_dm_partner_nick;
        printf("[%s] %s: %s%s\n",
               timestamp, nick, content,
               read ? " [읽음]" : "");
    }
}
```

---

## FR-D05 — 안읽은 메시지 수

DM 목록 화면에서 각 대화 상대별 안읽은 메시지 수를 표시한다.

```
S→C  DM_LIST_RES|<count>:<partner_id>:<partner_nick>:<timestamp>:<unread>:<last_msg>;<partner_id>:...
```

계산 방법 (서버):
- `messages.txt`에서 `to_id=user_id`이고 `room_id=0`인 메시지 중
- `dm_reads.txt`에 `reader_id=user_id`로 기록되지 않은 메시지 수

```c
int get_unread_dm_count(const char *user_id, const char *with_id) {
    int count = 0;
    for (int i = 0; i < g_msg_count; i++) {
        MessageRecord *m = &g_messages[i];
        if (m->room_id != 0) continue;
        if (strcmp(m->to_id, user_id) != 0) continue;
        if (strcmp(m->from_id, with_id) != 0) continue;
        if (!is_dm_read(g_dm_reads, g_dm_read_count, m->id, user_id))
            count++;
    }
    return count;
}
```

화면 표시:
```
  1. [ON ] 김철수  | 알겠습니다!   | 안읽음: 2
  2. [OFF] 이영희  | 내일 봐요     | 안읽음: 0
```
