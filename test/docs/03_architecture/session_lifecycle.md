# 세션 수명주기

## 1. 전체 시퀀스

```mermaid
sequenceDiagram
    participant C as Client
    participant A as Accept
    participant H as Handler Thread
    participant S as g_sessions
    participant DB as MySQL

    C->>A: TCP connect
    A->>H: pthread_create
    H->>DB: mysql_real_connect
    H->>C: ready (no packet)
    C->>H: LOGIN_REQ
    H->>DB: SELECT users
    DB-->>H: row
    H->>S: register session (mutex)
    H-->>C: LOGIN_RES 0
    loop 메시지 루프
        C->>H: ROOM_MSG / DM_SEND / ...
        H->>DB: INSERT / SELECT
        H->>S: find members (mutex, snapshot)
        H-->>C: fan-out
    end
    C->>H: LOGOUT_REQ / disconnect
    H->>DB: UPDATE last_seen
    H->>S: remove session (mutex)
    H->>DB: mysql_close
    H-->>C: close socket
    H->>H: pthread_exit
```

## 2. 상태 전이

```mermaid
stateDiagram-v2
    [*] --> Connected: accept
    Connected --> Authenticated: LOGIN_RES 0
    Connected --> Closed: auth 실패 N회 / LOGOUT_REQ / disconnect
    Authenticated --> InRoom: ROOM_JOIN 성공
    InRoom --> Authenticated: ROOM_LEAVE
    Authenticated --> Closed: LOGOUT_REQ / disconnect
    InRoom --> Closed: disconnect (서버가 ROOM_LEAVE 합성)
    Closed --> [*]
```

## 3. 실패 경로

| 상황 | 처리 |
|------|------|
| `Connected` 에서 60초 내 로그인 없음 | 서버가 소켓 close (DoS 방지) |
| auth 실패 5회 | 소켓 close + 잠시 IP 기록(옵션, P4) |
| `InRoom` 중 연결 끊김 | 핸들러가 `ROOM_LEAVE` 를 합성하여 방에 시스템 메시지 + 세션 정리 |
| DB 연결 끊김 | 3회 재시도. 실패 시 현재 클라이언트만 종료(다른 세션에 영향 없음) |
