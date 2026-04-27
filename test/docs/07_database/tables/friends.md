# friends

## DDL

```sql
CREATE TABLE friends (
    id         INT          AUTO_INCREMENT PRIMARY KEY,
    user_id    VARCHAR(20)  NOT NULL,
    friend_id  VARCHAR(20)  NOT NULL,
    status     TINYINT      DEFAULT 0,      -- 0=pending 1=accepted 2=blocked
    created_at DATETIME     DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uniq_pair (user_id, friend_id),
    INDEX idx_friend_status (friend_id, status),
    FOREIGN KEY (user_id)   REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (friend_id) REFERENCES users(id) ON DELETE CASCADE
);
```

## 표현 모델

- pending: 요청자가 수신자 방향으로만 1레코드 `(user_id=나, friend_id=상대, status=0)`.
- accepted: 양방향 2레코드 모두 `status=1`.
- blocked: 차단자 방향 1레코드 `status=2`.

## 전이 시 쿼리

- 요청: `INSERT INTO friends (user_id, friend_id, status) VALUES (?,?,0)`
- 수락:
  ```sql
  UPDATE friends SET status=1 WHERE user_id=? AND friend_id=? AND status=0;
  INSERT INTO friends (user_id, friend_id, status) VALUES (?,?,1)
    ON DUPLICATE KEY UPDATE status=1;
  ```
- 거절: `DELETE FROM friends WHERE user_id=? AND friend_id=? AND status=0`
- 삭제(친구): 양방향 2 `DELETE`.
- 차단: `INSERT ... ON DUPLICATE KEY UPDATE status=2`.
- 차단 해제(unblock): `UPDATE friends SET status=1 WHERE user_id=? AND friend_id=? AND status=2` (양방향 레코드가 이미 status=1 이면 그대로. 없으면 INSERT).

## 인덱스 근거

- `idx_friend_status(friend_id, status)`: 나에게 온 pending 요청 조회용.
- `uniq_pair`: 중복 레코드 방지 + pair 조회 인덱스 역할.
