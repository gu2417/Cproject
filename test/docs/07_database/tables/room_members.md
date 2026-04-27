# room_members

## DDL

```sql
CREATE TABLE room_members (
    room_id    INT          NOT NULL,
    user_id    VARCHAR(20)  NOT NULL,
    open_nick  VARCHAR(20)  DEFAULT '',
    is_admin   TINYINT      DEFAULT 0,
    is_muted   TINYINT      DEFAULT 0,
    joined_at  DATETIME     DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (room_id, user_id),
    INDEX idx_user (user_id),
    FOREIGN KEY (room_id) REFERENCES rooms(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);
```

## 컬럼

| 컬럼 | 설명 |
|------|------|
| `open_nick` | 오픈채팅 전용 닉네임. 비어 있으면 `users.nickname` 사용. |
| `is_admin` | 방 관리자 권한. 방장은 추가로 `rooms.owner_id` 로 식별. |
| `is_muted` | 해당 방 알림 무음. 서버는 이 멤버에게 `NOTIFY` 를 보내지 않음(메시지 자체는 정상 수신). |

## 운영 패턴

- 멤버십 검사: `SELECT 1 FROM room_members WHERE room_id=? AND user_id=?`
- 방 멤버 fd 스냅샷: 서버는 세션 배열을 순회해 `current room` 이 아닌 멤버도 포함해 fan-out 대상을 결정(방 메시지는 방에 없어도 수신). 현재 접속 중인 fd 수집은 `g_sessions` 에서.

## 인덱스

- PK `(room_id, user_id)` 로 멤버십 조회 O(logN).
- `idx_user` 로 FR-P02 `참여 방 수` 집계.
