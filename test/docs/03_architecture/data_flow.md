# 데이터 흐름

## 1. 방 메시지 전송

```mermaid
sequenceDiagram
    participant A as Client A
    participant HA as Handler(A)
    participant DB as MySQL
    participant S as g_sessions
    participant HB as Handler(B)
    participant B as Client B

    A->>HA: ROOM_MSG|room_id:content
    HA->>HA: 멤버십 검사 (SELECT)
    HA->>DB: INSERT messages
    DB-->>HA: msg_id
    HA->>S: 멤버 fd 스냅샷 (mutex)
    HA->>HB: (send via fd)\nROOM_MSG_RECV
    HB->>B: 표시
    HA->>A: (send)\nROOM_MSG_RECV (에코)
```

**순서 결정**: `INSERT → 멤버 fan-out` (DB 저장 완료 후 전달). 이 순서는 "삭제된 메시지 먼저 브로드캐스트되는" 레이스를 방지한다. NFR-02 50ms 는 로컬 기준이므로 DB 왕복 포함해도 달성 가능.

## 2. DM 전송

```mermaid
sequenceDiagram
    participant A as Client A
    participant HA as Handler(A)
    participant DB as MySQL
    participant S as g_sessions
    participant HB as Handler(B)
    participant B as Client B

    A->>HA: DM_SEND|to:content
    HA->>HA: 차단 검사
    HA->>DB: INSERT messages (room_id=NULL)
    DB-->>HA: msg_id
    HA->>S: find B session
    alt B 접속 중
        HA->>HB: send DM_RECV
        HB->>B: 표시
    else B 오프라인
        Note over HA: 메시지는 히스토리로만 전달<br/>로그인/DM 열람 시 조회
    end
    HA->>A: DM_RECV (에코)
```

## 3. 친구 상태 변경 전파

로그인 시 친구 목록을 조회하고, **각 친구의 세션에 `FRIEND_STATUS_CHANGE` 송신**. 로그아웃 시도 동일.

```mermaid
flowchart LR
    login[LOGIN 성공] --> q[SELECT friends WHERE friend_id=me AND status=1]
    q --> loop{각 친구 세션}
    loop --> active{온라인?}
    active -- yes --> push[FRIEND_STATUS_CHANGE 송신]
    active -- no --> skip[skip]
```

## 4. 알림 전달 규칙 요약

| 이벤트 | 대상 | 억제 조건 |
|--------|------|-----------|
| 방 메시지 | 방 멤버 중 접속 & `current_room_id != 해당 방` | DND, room mute |
| 멘션 | 멘션된 유저 | **억제 없음** |
| DM 수신 | 수신자 | DND |
| 친구 요청 | 수신자 | 없음 |
| 서버 공지 | 전체 | 없음 |
