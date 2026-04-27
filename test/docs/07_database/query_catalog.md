# 쿼리 카탈로그

기능별 대표 SQL. 모두 **prepared statement** 로 실행(`?` 플레이스홀더).

## 인증

```sql
-- 로그인
SELECT id, password_hash FROM users WHERE id = ?;

-- 회원가입
INSERT INTO users (id, password_hash, nickname, status_msg)
VALUES (?, SHA2(?, 256), ?, ?);
INSERT INTO user_settings (user_id) VALUES (?);

-- 로그아웃(상태/last_seen)
UPDATE users SET online_status = 0, last_seen = NOW() WHERE id = ?;

-- 비밀번호 변경
UPDATE users SET password_hash = SHA2(?, 256) WHERE id = ?;
```

## 친구

```sql
-- 요청
INSERT INTO friends (user_id, friend_id, status) VALUES (?, ?, 0);

-- 수락 (양방향)
UPDATE friends SET status = 1 WHERE user_id = ? AND friend_id = ? AND status = 0;
INSERT INTO friends (user_id, friend_id, status) VALUES (?, ?, 1)
  ON DUPLICATE KEY UPDATE status = 1;

-- 목록
SELECT u.id, u.nickname, u.online_status, u.status_msg
FROM friends f JOIN users u ON f.friend_id = u.id
WHERE f.user_id = ? AND f.status = 1;

-- 받은 pending 요청
SELECT f.user_id, u.nickname
FROM friends f JOIN users u ON f.user_id = u.id
WHERE f.friend_id = ? AND f.status = 0;

-- 차단 여부
SELECT 1 FROM friends WHERE user_id = ? AND friend_id = ? AND status = 2;

-- 차단 해제(unblock)
UPDATE friends SET status = 1 WHERE user_id = ? AND friend_id = ? AND status = 2;
INSERT INTO friends (user_id, friend_id, status) VALUES (?, ?, 1)
  ON DUPLICATE KEY UPDATE status = 1;

-- 검색
SELECT id, nickname, status_msg FROM users
WHERE id LIKE ? ESCAPE '\\' OR nickname LIKE ? ESCAPE '\\'
ORDER BY id LIMIT 30;
```

## 방

```sql
-- 생성
INSERT INTO rooms (name, topic, password_hash, max_users, owner_id, is_open)
VALUES (?, ?, ?, ?, ?, ?);
INSERT INTO room_members (room_id, user_id, is_admin) VALUES (?, ?, 1);

-- 오픈방 목록
SELECT r.id, r.name, r.topic, r.max_users, r.password_hash<>'' AS has_pw,
       (SELECT COUNT(*) FROM room_members m WHERE m.room_id = r.id) AS cur
FROM rooms r WHERE r.is_open = 1 ORDER BY r.id DESC LIMIT 50;

-- 멤버십 검사
SELECT 1 FROM room_members WHERE room_id = ? AND user_id = ?;

-- 참여
INSERT INTO room_members (room_id, user_id) VALUES (?, ?);

-- 멤버 목록
SELECT u.id, u.nickname, m.is_admin, u.online_status, m.open_nick
FROM room_members m JOIN users u ON m.user_id = u.id
WHERE m.room_id = ?;

-- 공지 변경
UPDATE rooms SET notice = ? WHERE id = ?;

-- 핀 설정/해제
UPDATE rooms SET pinned_msg_id = ? WHERE id = ?;  -- NULL 로 해제
```

## 메시지

```sql
-- 방 메시지 저장
INSERT INTO messages (room_id, from_id, content, reply_to, msg_type)
VALUES (?, ?, ?, ?, ?);

-- DM 저장
INSERT INTO messages (room_id, from_id, to_id, content)
VALUES (NULL, ?, ?, ?);

-- 방 히스토리
SELECT id, from_id, content, reply_to, msg_type, is_deleted, created_at, edited_at
FROM messages WHERE room_id = ? ORDER BY id DESC LIMIT 100;

-- DM 히스토리
SELECT id, from_id, to_id, content, is_deleted, created_at
FROM messages WHERE room_id IS NULL
  AND ((from_id = ? AND to_id = ?) OR (from_id = ? AND to_id = ?))
ORDER BY id DESC LIMIT 50;

-- 안읽은 DM 수
SELECT COUNT(*) FROM messages m
WHERE m.room_id IS NULL AND m.to_id = ? AND m.is_deleted = 0
  AND NOT EXISTS (SELECT 1 FROM dm_reads r WHERE r.msg_id = m.id AND r.reader_id = ?);

-- DM 읽음 처리(일괄)
INSERT IGNORE INTO dm_reads (msg_id, reader_id)
SELECT id, ? FROM messages
WHERE room_id IS NULL AND to_id = ? AND from_id = ?;

-- 수정 (5분 내, 본인, 미삭제)
UPDATE messages SET content = ?, edited_at = NOW()
WHERE id = ? AND from_id = ? AND is_deleted = 0
  AND TIMESTAMPDIFF(MINUTE, created_at, NOW()) <= 5;

-- 삭제
UPDATE messages SET is_deleted = 1 WHERE id = ? AND from_id = ?;

-- 검색
SELECT id, from_id, content, created_at FROM messages
WHERE room_id = ? AND is_deleted = 0 AND content LIKE ? ESCAPE '\\'
ORDER BY id DESC LIMIT 30;
```

## 마이페이지

```sql
SELECT u.id, u.nickname, u.status_msg, u.created_at, u.last_seen,
       (SELECT COUNT(*) FROM messages WHERE from_id = u.id AND is_deleted = 0) msg_count,
       (SELECT COUNT(*) FROM room_members WHERE user_id = u.id)                 room_count,
       (SELECT COUNT(*) FROM friends WHERE user_id = u.id AND status = 1)       friend_count
FROM users u WHERE u.id = ?;
```

## 관리자

```sql
-- 서버 상태
SELECT (SELECT COUNT(*) FROM users)    AS total_users,
       (SELECT COUNT(*) FROM rooms)    AS total_rooms,
       (SELECT COUNT(*) FROM messages WHERE is_deleted=0) AS total_messages;

-- 유저 목록
SELECT id, nickname, online_status FROM users ORDER BY id LIMIT 100;
```

## 방 운영

```sql
-- 강퇴(ROOM_KICK): 방 멤버 제거
DELETE FROM room_members WHERE room_id = ? AND user_id = ?;

-- 공동 방장 부여 (ROOM_GRANT_ADMIN)
UPDATE room_members SET is_admin = 1 WHERE room_id = ? AND user_id = ?;

-- 공동 방장 박탈 (ROOM_REVOKE_ADMIN): owner_id 는 변경 불가
UPDATE room_members SET is_admin = 0
WHERE room_id = ? AND user_id = ?
  AND user_id <> (SELECT owner_id FROM rooms WHERE id = ?);

-- 방장 승계: 가장 일찍 가입한 관리자를 신규 owner 로
SELECT user_id FROM room_members
WHERE room_id = ? AND user_id <> ? AND is_admin = 1
ORDER BY joined_at ASC LIMIT 1;

UPDATE rooms SET owner_id = ? WHERE id = ?;

-- 방 삭제(관리자 부재 시): messages soft-delete 후 rooms 삭제
UPDATE messages SET is_deleted = 1 WHERE room_id = ?;
DELETE FROM rooms WHERE id = ?;
```

## 알림 설정 (DND)

```sql
-- DND 토글
UPDATE user_settings SET dnd = ? WHERE user_id = ?;

-- 현재 DND 상태 조회
SELECT dnd FROM user_settings WHERE user_id = ?;
```

## 오픈채팅 닉네임

```sql
-- 설정 (ROOM_SET_OPEN_NICK)
UPDATE room_members SET open_nick = ? WHERE room_id = ? AND user_id = ?;

-- 초기화(빈 문자열로 복원)
UPDATE room_members SET open_nick = '' WHERE room_id = ? AND user_id = ?;
```
