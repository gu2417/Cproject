# messages

## DDL

```sql
CREATE TABLE messages (
    id         INT          AUTO_INCREMENT PRIMARY KEY,
    room_id    INT          DEFAULT NULL,   -- NULL = DM
    from_id    VARCHAR(20)  NOT NULL,
    to_id      VARCHAR(20)  DEFAULT NULL,   -- DM 수신자
    content    VARCHAR(500) NOT NULL,
    reply_to   INT          DEFAULT NULL,
    msg_type   TINYINT      DEFAULT 0,      -- 0=normal 1=system 2=whisper 3=me
    is_deleted TINYINT      DEFAULT 0,
    created_at DATETIME     DEFAULT CURRENT_TIMESTAMP,
    edited_at  DATETIME     DEFAULT NULL,
    INDEX idx_room_id    (room_id, id),
    INDEX idx_dm_pair    (from_id, to_id, id),
    INDEX idx_to_unread  (to_id, is_deleted),
    FOREIGN KEY (from_id)  REFERENCES users(id),
    FOREIGN KEY (reply_to) REFERENCES messages(id) ON DELETE SET NULL
);
```

## 컬럼

| 컬럼 | 설명 |
|------|------|
| `room_id` | NULL 이면 DM. 값 있으면 그룹/오픈 메시지. |
| `to_id` | DM 에서만 사용. 그룹에서는 NULL. 귓속말(`msg_type=2`)은 `to_id` 로 개인 대상 기록. |
| `reply_to` | 답장 원본 id. 원본 삭제 시 SET NULL (표시는 "원본 사라짐"). |
| `msg_type` | 0 normal / 1 system / 2 whisper / 3 me-action. |
| `is_deleted` | 1 이면 `content` 를 클라에 전달하지 않음(서버가 내용 blank 처리). |
| `edited_at` | 수정된 경우에만 값. 표시 "(수정됨)". |

## 주요 쿼리

- 방 히스토리(100개):
  ```sql
  SELECT id, from_id, content, reply_to, msg_type, is_deleted, created_at, edited_at
  FROM messages WHERE room_id=? ORDER BY id DESC LIMIT 100;
  ```
- DM 히스토리(50개):
  ```sql
  SELECT id, from_id, to_id, content, is_deleted, created_at
  FROM messages
  WHERE room_id IS NULL
    AND ((from_id=:me AND to_id=:peer) OR (from_id=:peer AND to_id=:me))
  ORDER BY id DESC LIMIT 50;
  ```
- 방 내 검색(FR-M08):
  ```sql
  SELECT id, from_id, content, created_at
  FROM messages WHERE room_id=? AND is_deleted=0 AND content LIKE ?
  ORDER BY id DESC LIMIT 30;
  ```

## 인덱스 근거

| 인덱스 | 쿼리 |
|--------|------|
| `idx_room_id(room_id, id)` | 방 히스토리·검색·핀 조회 |
| `idx_dm_pair(from_id, to_id, id)` | DM 히스토리 |
| `idx_to_unread(to_id, is_deleted)` | 안읽은 DM 수 집계 |

`content LIKE '%kw%'` 는 풀스캔이 되지만 LIMIT 30 + `idx_room_id` 로 효율적. 전문 검색 엔진 도입은 out-of-scope.
