# 인덱스 · 성능

## 1. 인덱스 요약

| 테이블 | 인덱스 | 쿼리 |
|--------|--------|------|
| `users` | PK(id), `idx_nickname` | 로그인, 검색 |
| `user_settings` | PK(user_id) | 1:1 조회 |
| `friends` | PK(id), `uniq_pair`, `idx_friend_status` | 친구 관리, 받은 요청 조회 |
| `rooms` | PK(id), `idx_is_open`, `idx_name` | 오픈방 목록, 방 검색 |
| `room_members` | PK(room_id,user_id), `idx_user` | 멤버십·참여 방 수 |
| `messages` | PK(id), `idx_room_id`, `idx_dm_pair`, `idx_to_unread` | 히스토리·검색·안읽은 수 |
| `dm_reads` | PK(msg_id,reader_id) | 읽음 기록 |

## 2. 쿼리별 기대 비용

| 쿼리 | 접근 경로 | 비용 |
|------|----------|------|
| 로그인 | `users` PK 1행 | O(logN) |
| 방 히스토리 100개 | `idx_room_id` 역순 100행 | O(logN + 100) |
| DM 히스토리 50개 | `idx_dm_pair` 역순 50행 | O(logN + 50) |
| 메시지 전문 검색 | `idx_room_id` + content LIKE | O(방크기). LIMIT 30 으로 절단 |
| 안읽은 DM 수 | `idx_to_unread` + `NOT EXISTS(dm_reads PK)` | O(unread × logN) |
| 오픈방 목록 | `idx_is_open` 50행 + 서브쿼리 COUNT | COUNT 는 `idx_user`로 O(멤버수) 누적. 방 50×30 ≈ 1500, 허용 |
| 친구 목록 | `friends(user_id, status)` 인덱스 스캔 + users PK 조인 | O(친구수) |

## 3. 주의점

- `messages.content LIKE '%kw%'` 은 인덱스 미활용. 방당 메시지가 수천~수만 수준까지는 문제없음. 수십만 이상이면 FULLTEXT 또는 외부 엔진(out-of-scope).
- 오픈방 목록의 `COUNT(*)` 서브쿼리가 스케일 이슈면 `rooms.member_count` 비정규화 컬럼 도입(v2.1).

## 4. Connection-level 설정

- `SET NAMES utf8mb4;` 연결 직후 1회.
- `SET SESSION wait_timeout=600;` (10분) — 유휴 연결 빨리 끊기.
- prepared stmt 준비는 호출마다 — 성능 부족 시 **세션 수명 캐시** 로 전환(v2.1 검토).

## 5. 병목 시뮬레이션 체크리스트

- [ ] 100 동시 접속 × 초당 1 메시지 = 100 INSERT/s. `messages` 인덱스 4개 갱신 부하 수용.
- [ ] 방 히스토리 100명 동시 입장 = 100 × 100 = 1만 행 조회. `idx_room_id` 역순 범위 스캔으로 충분.
