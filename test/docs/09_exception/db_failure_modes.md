# DB 장애 모드

## 1. 장애 유형별 대응

| 상황 | 감지 | 대응 |
|------|------|------|
| 초기 connect 실패 | `mysql_real_connect == NULL` | 100/200/400ms 재시도 → 실패 시 세션 종료, 클라에 `SERVER_ERROR` |
| 쿼리 중 `CR_SERVER_LOST` (2013) | `mysql_errno` | `mysql_ping` 1회, 재시도 1회 후 실패면 세션 종료 |
| `CR_SERVER_GONE_ERROR` (2006) | 동일 | 동일 |
| `ER_LOCK_DEADLOCK` (1213) | `mysql_errno` | 트랜잭션 rollback, 1회 재시도. 실패 시 `SERVER_ERROR` |
| `ER_LOCK_WAIT_TIMEOUT` (1205) | 동일 | 위와 동일 |
| `ER_DUP_ENTRY` (1062) | 동일 | 비즈니스 맥락에 따라 `DUPLICATE`(3) 또는 무시(INSERT IGNORE) |
| 기타 서버 오류 | 동일 | 로그 + `SERVER_ERROR` |

## 2. 트랜잭션 사용 범위

- 회원가입: `users` + `user_settings` 두 INSERT → BEGIN / COMMIT.
- 친구 수락: `UPDATE` + `INSERT` → BEGIN / COMMIT.
- 방 생성: `rooms` + `room_members` → BEGIN / COMMIT.
- 그 외 단일 INSERT/UPDATE 는 autocommit.

## 3. 재시도 가능성

- 멱등성 보장된 경우만 재시도:
  - SELECT → 항상 안전.
  - UPDATE … WHERE id=? → 대부분 안전(동일 결과).
  - INSERT … ON DUPLICATE KEY UPDATE → 안전.
  - 일반 INSERT → **재시도 금지**(중복 생성 위험). 대신 클라가 다시 요청하도록 `SERVER_ERROR` 반환.

## 4. 전면 장애 시

- 서버 프로세스는 계속 살아 있되 모든 핸들러가 `SERVER_ERROR` 응답.
- 운영자는 `SYSTEM_NOTIFY|2|...` 로 수동 공지 후 서버 재시작.
- 자동 복구 워치독은 v2.1 이후.
