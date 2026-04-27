# 마이페이지 (FR-P01 ~ P06)

## FR-P01 프로필 조회 · P1

| 패킷 | `MYPAGE_REQ` / `MYPAGE_RES` |
| 필드 | id, nickname, status_msg, created_at, last_seen, msg_count, room_count, friend_count |

## FR-P02 활동 통계 · P1

- `msg_count`  = `SELECT COUNT(*) FROM messages WHERE from_id=? AND is_deleted=0`
- `room_count` = `SELECT COUNT(*) FROM room_members WHERE user_id=?`
- `friend_count` = `SELECT COUNT(*) FROM friends WHERE user_id=? AND status=1`

단일 패킷에 합쳐 반환. 개별 쿼리 3개는 인덱스로 O(logN).

## FR-P03 참여 채팅방 목록 · P1

- 별도 패킷 미정의. `ROOM_LIST_REQ|joined` 로 확장하거나, 마이페이지 응답에 `rooms[]` 추가(설계 결정: **별도 패킷 `ROOM_LIST_REQ` 에 type 확장**).

## FR-P04 최근 DM 목록 · P1

| 쿼리 (의사) | 최근 대화 상대별 최신 메시지 1건 + 안읽은 수 조회 |
```sql
SELECT peer, MAX(id) AS last_id, SUM(unread) AS unread_cnt
FROM (
  SELECT IF(from_id=:me, to_id, from_id) AS peer, id,
         IF(to_id=:me AND NOT EXISTS(SELECT 1 FROM dm_reads r WHERE r.msg_id=m.id AND r.reader_id=:me), 1, 0) AS unread
  FROM messages m WHERE room_id IS NULL AND (from_id=:me OR to_id=:me)
) t GROUP BY peer ORDER BY last_id DESC LIMIT 20;
```

## FR-P05 프로필 인라인 수정 · P1

FR-A04 와 동일 패킷 재사용. 화면에서 입력 → `PROFILE_UPDATE`.

## FR-P06 비밀번호 변경 · P1

FR-A05 와 동일. `PASS_CHANGE`.

## 관련 문서

- 패킷: [`08_api/packets/mypage.md`](../08_api/packets/mypage.md)
