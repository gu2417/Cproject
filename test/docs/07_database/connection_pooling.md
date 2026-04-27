# 연결 관리

## 1. 원칙

- **스레드 전용 MYSQL\***. pool 또는 공유 금지(NFR-09).
- 핸들러 스레드 start 시 연결 획득, 종료 시 해제.
- 관리자 제외, 서버는 connection 수 = 활성 클라이언트 수 ≤ `MAX_CLIENTS`.

## 2. 생성

```c
MYSQL *db_connect(void) {
    MYSQL *c = mysql_init(NULL);
    if (!c) return NULL;
    unsigned int to = 5; /* seconds */
    mysql_options(c, MYSQL_OPT_CONNECT_TIMEOUT, &to);
    mysql_options(c, MYSQL_OPT_READ_TIMEOUT,    &to);
    mysql_options(c, MYSQL_SET_CHARSET_NAME,    "utf8mb4");
    if (!mysql_real_connect(c, DB_HOST, DB_USER, DB_PASS, DB_NAME, DB_PORT, NULL, 0)) {
        mysql_close(c);
        return NULL;
    }
    mysql_query(c, "SET SESSION wait_timeout=600");
    mysql_query(c, "SET SESSION TRANSACTION ISOLATION LEVEL REPEATABLE READ"); /* MySQL 기본값이지만 명시적으로 보장 */
    /* autocommit=1(기본) 유지. 트랜잭션이 필요한 곳에서만 BEGIN/COMMIT 사용 */
    return c;
}
```

재시도: 초기 실패 시 3회(100ms, 200ms, 400ms). 이후 실패하면 클라이언트에게 `SERVER_ERROR` 응답 후 세션 종료.

## 3. 해제

- `mysql_close(c)` + `c=NULL`. 스레드 종료 블록에서만 호출.
- 중간에 연결이 끊기면(`CR_SERVER_LOST` 2013 / `CR_SERVER_GONE_ERROR` 2006) 1회 `mysql_ping` 재연결 시도 → 실패 시 세션 종료.

## 4. 쓰레드-safety

- `mysql_thread_init()` / `mysql_thread_end()` 를 핸들러 시작·종료에서 호출.
- 한 MYSQL\* 이 두 스레드에 걸쳐 사용되지 않도록 **함수 범위 밖으로 절대 반환하지 않음**.

## 5. prepared statement 수명

- 기본: 함수 로컬. 호출마다 `db_prepare` → `db_stmt_close`.
- 최적화(v2.1): 핸들러별로 자주 쓰는 것(`SELECT users by id`, `INSERT messages`) 을 lazy 캐시. MYSQL_STMT* 는 자신의 MYSQL* 에 귀속되므로 그대로 스레드 전용.

## 6. MySQL 서버 측 요구

- `max_connections` ≥ `MAX_CLIENTS + 8` (여유분).
- `character_set_server = utf8mb4`.
- `sql_mode` 는 `STRICT_TRANS_TABLES` 권장(길이 초과 등 사전 검출).
