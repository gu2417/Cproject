# dm 패킷

## 요청

```
DM_SEND|<to_id>|<content>
DM_HISTORY|<peer_id>
DM_LIST                        # 대화 상대 목록 + 안읽은 수
```

응답:

```
DM_SEND_RES|0|<msg_id>|<created_at>
DM_SEND_RES|7                 # 차단
DM_SEND_RES|4                 # 대상 없음

# 히스토리 (자동 읽음 처리)
DM_HISTORY_RES|0|<msg_id>:<from_id>:<to_id>:<is_deleted>:<created_at>:<content>;...

# 대화 상대 목록
DM_LIST_RES|0|<peer_id>:<peer_nick>:<unread>:<last_preview>;...
```

## 서버 알림

```
DM_NEW_NOTIFY|<msg_id>|<from_id>|<from_nick>|<created_at>|<content>
DM_READ_NOTIFY|<peer_id>|<last_read_msg_id>
```
- 상대가 내 DM 을 읽으면 `DM_READ_NOTIFY` 로 나에게 통지.

## 특수 케이스

- 내가 보낸 DM 이 상대에게 도달해도 상대가 오프라인이면 `DM_NEW_NOTIFY` 미전송(다음 로그인 시 `DM_LIST` 로 확인).
- 차단: `BLOCKED` 로 reject. 차단당한 쪽은 전송 자체 불가 — 조용한 실패 대신 **명시적 에러**.

## 관련 DB

- `messages` (room_id IS NULL), `dm_reads`. `query_catalog.md` "메시지" 절 중 DM 블록.
