# 오픈 채팅방 (FR-O01 ~ O05)

`rooms.is_open=1`. 목록 발견 기반 공개 채팅방.

## FR-O01 생성 · P0

| 입력 | name, topic, max_users, password(선택) |
| 패킷 | `ROOM_CREATE` (is_open=1) |
| 차이 | 그룹 방과 달리 **초대 없이도** 누구나 목록 발견 후 참여 |

## FR-O02 목록 조회 · P0

| 패킷 | `ROOM_LIST_REQ|open` / `ROOM_LIST_RES` |
| 쿼리 | `SELECT id, name, (SELECT COUNT(*) FROM room_members WHERE room_id=r.id) AS cur, max_users, topic, (password_hash<>'') AS has_pw FROM rooms r WHERE is_open=1 ORDER BY id DESC LIMIT 50` |
| 페이지네이션 | P3 범위(v2.1). 현재는 LIMIT 50 고정 |

## FR-O03 방 검색 · P3

| 패킷 | `ROOM_SEARCH` / `ROOM_SEARCH_RES` |
| 쿼리 | `... WHERE is_open=1 AND (name LIKE ? OR topic LIKE ?) LIMIT 30` |

## FR-O04 자유 참여 · P0

| 패킷 | `ROOM_JOIN` |
| 로직 | ① 방 존재·공개 여부 → ② `password_hash<>''` 이면 입력 pw 비교 → ③ 정원 검사 → ④ `INSERT room_members(is_admin=0)` → ⑤ 시스템 입장 메시지 + 히스토리 100개 전송 |

## FR-O05 익명 닉네임 · P3

| 패킷 | `ROOM_SET_OPEN_NICK` / `ROOM_SET_OPEN_NICK_RES` |
| 저장 | `room_members.open_nick` |
| 표시 | 해당 방의 메시지 브로드캐스트 시 `from_nick` 에 open_nick 우선 사용, 미설정 시 `users.nickname` |

## 관련 문서

- 패킷: [`08_api/packets/room.md`](../08_api/packets/room.md)
