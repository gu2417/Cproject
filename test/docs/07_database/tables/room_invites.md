# room_invites 테이블

오프라인 상태 유저에 대한 채팅방 초대를 영속적으로 저장한다.

## DDL

```sql
CREATE TABLE room_invites (
    id         INT AUTO_INCREMENT PRIMARY KEY,
    room_id    INT          NOT NULL,
    inviter_id VARCHAR(20)  NOT NULL,
    invitee_id VARCHAR(20)  NOT NULL,
    status     TINYINT      NOT NULL DEFAULT 0,
    created_at DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uq_invite (room_id, invitee_id),
    INDEX      idx_invitee (invitee_id),
    CONSTRAINT fk_ri_room    FOREIGN KEY (room_id)    REFERENCES rooms(id)  ON DELETE CASCADE,
    CONSTRAINT fk_ri_inviter FOREIGN KEY (inviter_id) REFERENCES users(id)  ON DELETE CASCADE,
    CONSTRAINT fk_ri_invitee FOREIGN KEY (invitee_id) REFERENCES users(id)  ON DELETE CASCADE
);
```

## 컬럼 설명

| 컬럼 | 타입 | 설명 |
|------|------|------|
| id | INT AUTO_INCREMENT | 기본 키 |
| room_id | INT FK | 초대 대상 채팅방 |
| inviter_id | VARCHAR(20) FK | 초대한 유저 ID |
| invitee_id | VARCHAR(20) FK | 초대받은 유저 ID |
| status | TINYINT | 0=대기(pending), 1=수락, 2=거절 |
| created_at | DATETIME | 초대 생성 시각 |

## 동작 흐름

1. 방장이 `ROOM_INVITE|<room_id>|<target_id>` 전송
2. 서버: 대상이 온라인 → `ROOM_INVITED_NOTIFY` 즉시 발송, 오프라인 → `room_invites`에 `status=0` 저장
3. 대상 로그인 시: `SELECT * FROM room_invites WHERE invitee_id=? AND status=0` → pending 초대 `ROOM_INVITED_NOTIFY`로 재전송
4. 대상이 수락 → `ROOM_INVITE_ACCEPT|<room_id>` → `status=1`, `room_members`에 추가, `ROOM_MEMBER_JOINED_NOTIFY` 브로드캐스트
5. 대상이 거절 → `ROOM_INVITE_REJECT|<room_id>` → `status=2`
6. `UNIQUE KEY (room_id, invitee_id)`: 같은 방에 동일 유저 중복 초대 방지 (이미 pending이면 에러 코드 3)

## 관련

- 패킷: [`../../08_api/packets/room.md`](../../08_api/packets/room.md)
- ER 다이어그램: [`../er_diagram.md`](../er_diagram.md)
