# 서버 에러 처리

## 1. 스레드 격리

- 핸들러 스레드 내의 모든 예외는 해당 스레드의 세션만 종료시킨다.
- `main.c` 의 accept 스레드는 `errno=EMFILE` 등 일시적 오류는 로그 후 재시도.

## 2. 세션 종료 시 청소 순서

```c
cleanup:
    if (stmt) mysql_stmt_close(stmt);
    if (db)   mysql_close(db);
    pthread_mutex_lock(&g_sessions_mu);
    remove_session(fd);
    pthread_mutex_unlock(&g_sessions_mu);
    close(fd);
    mysql_thread_end();
    return NULL;
```

- `close(fd)` 는 반드시 한 번만. double-close 는 다른 스레드가 재사용한 fd 를 닫을 위험이 있어 **절대 금지**.
- `remove_session` 은 소속 방들에게 `ROOM_MEMBER_LEFT_NOTIFY` 대신 **오프라인 알림만** 발송.

## 3. mutex 규약

- 브로드캐스트는 **락을 잡은 동안 I/O 금지**. fd 스냅샷을 로컬 배열로 복사 후 unlock → send.
- 락 홀딩 중 발생한 exit 경로라도 unlock 을 반드시 수행(goto cleanup 패턴).

## 4. write 실패

- `send()` 가 `EPIPE` / `ECONNRESET` → 해당 세션만 종료, 다른 fan-out 대상은 계속 처리.
- 부분 write(일부 byte 만 전송) 는 루프로 잔여 전송. 30초 타임아웃 시 drop.

## 5. DB 오류 후 복구

- `mysql_errno == 2006 (GONE) or 2013 (LOST)` → `mysql_ping` 1회, 실패 시 세션 종료.
- `mysql_errno == 1213 (DEADLOCK)` → 트랜잭션 1회 재시도, 실패 시 `SERVER_ERROR`.
- 그 외 오류 → 로그 + `SERVER_ERROR`, 세션은 유지(치명 아님).

## 6. 패닉 가드

- `SIGPIPE` 는 서버 시작 시 `signal(SIGPIPE, SIG_IGN)` (또는 `MSG_NOSIGNAL`).
- `SIGSEGV` 등 치명 시그널은 기본 동작(코어 덤프) 유지. 최소한 다른 세션 상태를 저장하려 시도하지 않음(복구 비용 > 재시작 비용).

## 7. accept 레이어

- `accept` 가 `EINTR` → 재시도.
- `EMFILE`(fd 고갈) → 1초 slept 후 재시도 + 경고 로그.
- `MAX_CLIENTS` 초과 → 즉시 close, `SYSTEM_NOTIFY|2|서버 정원이 찼습니다.` 전송 후 close.
