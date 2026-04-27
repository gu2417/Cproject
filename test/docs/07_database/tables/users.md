# users

## DDL

```sql
CREATE TABLE users (
    id            VARCHAR(20)  PRIMARY KEY,
    password_hash VARCHAR(64)  NOT NULL,
    nickname      VARCHAR(20)  NOT NULL,
    status_msg    VARCHAR(100) DEFAULT '',
    online_status TINYINT      DEFAULT 0,   -- 0=off 1=on 2=busy
    last_seen     DATETIME     DEFAULT NULL,
    created_at    DATETIME     DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uq_nickname (nickname),
    INDEX idx_nickname (nickname)
);
```

## 컬럼 의미

| 컬럼 | 설명 |
|------|------|
| `id` | 로그인 식별자. PK. 3~20자 영숫자/_. |
| `password_hash` | SHA2(plain,256) 결과. 64 hex chars. |
| `nickname` | 표시명. 한글 허용(UTF-8 기준 20자 수용 위해 실제 VARCHAR(20) 는 문자 수). UNIQUE 제약으로 중복 불허. |
| `status_msg` | 한 줄 상태. |
| `online_status` | 0=offline, 1=online, 2=busy. invisible 은 저장하지 않음(응답만 0 으로 가림). |
| `last_seen` | 마지막 로그아웃 혹은 연결 종료 시각. 로그인 중에는 갱신하지 않음. |
| `created_at` | 가입 시각. |

## 운영 노트

- 문자 집합: `utf8mb4`. 스키마 파일 상단에서 DB/테이블 기본 charset 지정.
- `UNIQUE KEY uq_nickname` 으로 닉네임 중복 가입 시 오류 코드 8 반환.
- `INDEX idx_nickname` 는 유저 검색(FR-F07)에 사용.
- DND 설정은 `user_settings` 테이블에서 관리.
- 삭제는 하드 delete. 관련 데이터는 FK CASCADE 로 정리(`user_settings`, `friends`, `room_members`, `dm_reads`). `messages.from_id` 는 CASCADE 없음 — 유저 삭제는 v2.0 에서 **지원 안 함**(메시지 보존).

## 접근 쿼리 (요약)

- 로그인 조회: `SELECT id, password_hash FROM users WHERE id=?`
- 검색: `SELECT id, nickname, status_msg FROM users WHERE id LIKE ? OR nickname LIKE ? LIMIT 30`
