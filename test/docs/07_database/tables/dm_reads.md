# dm_reads

## DDL

```sql
CREATE TABLE dm_reads (
    msg_id    INT          NOT NULL,
    reader_id VARCHAR(20)  NOT NULL,
    read_at   DATETIME     DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (msg_id, reader_id),
    FOREIGN KEY (msg_id)    REFERENCES messages(id) ON DELETE CASCADE,
    FOREIGN KEY (reader_id) REFERENCES users(id)    ON DELETE CASCADE
);
```

## 의미

- 한 DM 메시지에 대해 **수신자가 읽었음** 을 기록.
- 자신이 보낸 메시지를 "읽음" 처리하지 않는다(reader_id = to_id 인 레코드만 유의).

## 생성 시점

- 클라이언트가 DM 화면에서 해당 메시지를 뷰포트에 로드하는 순간 →
  서버에 `DM_HISTORY_REQ` 결과 수신 후, 클라가 별도 "읽음 처리" 이벤트를 보내는 대신 **서버가 히스토리 응답 시점에 자동 insert** 하도록 설계:
  ```sql
  INSERT IGNORE INTO dm_reads (msg_id, reader_id)
  SELECT id, :me FROM messages
  WHERE room_id IS NULL AND to_id = :me AND from_id = :peer;
  ```
- 새 DM 수신 후 수신자가 화면에 있으면 즉시 읽음 처리. 없으면 다음 열람 시.

## 안읽은 수 집계

`messages.md` 참조 — `idx_to_unread(to_id, is_deleted)` + `NOT EXISTS dm_reads` 서브쿼리.
