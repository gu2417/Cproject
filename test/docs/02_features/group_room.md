# 그룹 채팅방 (FR-G01 ~ G10)

초대 기반 닫힌 채팅방. `rooms.is_open=0`.

## FR-G01 채팅방 생성 · P0

| 입력 | name(≤30), topic(≤100), max_users(기본 30, ≤100), password(선택) |
| 패킷 | `ROOM_CREATE` / `ROOM_CREATE_RES` |
| 로직 | `INSERT rooms(..., is_open=0, owner_id=나, password_hash=SHA2(pw,256) or '')` → `INSERT room_members(room_id, user_id=나, is_admin=1)` |

## FR-G02 멤버 초대 · P1

| 패킷 | `ROOM_INVITE` |
| 권한 | 방장 또는 멤버. 초대 대상이 친구 차단 상태면 실패 |
| 로직 | `INSERT room_members(room_id, user_id=target)`. 상대 접속 중이면 `NOTIFY(type=INVITE)` |

## FR-G03 메시지 전송 · P0

| 패킷 | `ROOM_MSG` → 방 참여자 전원 `ROOM_MSG_RECV` |
| 로직 | ① 방 멤버십 검사 → ② `INSERT messages(room_id, from_id, content)` → ③ 접속 중 멤버 fan-out → ④ 오프라인 멤버는 히스토리로만 전달 |

## FR-G04 멘션(@) · P2

| 파싱 | 서버가 content 에서 `@<nick>` 토큰 추출 → 해당 멤버에게 `NOTIFY(type=MENTION)` 추가 전송(DND 상태여도) |

## FR-G05 채팅방 나가기 · P0

| 패킷 | `ROOM_LEAVE` |
| 로직 | `DELETE room_members WHERE room_id=? AND user_id=?` → 시스템 메시지 `"<nick> 님이 퇴장했습니다"` 브로드캐스트. 방장이 나갈 때: 남은 관리자 중 최장수자에게 자동 위임, 없으면 방 삭제 |

## FR-G06 방장 권한 · P2

| 포함 | 멤버 강퇴(`ROOM_KICK`), 방 삭제, 공지 등록, 핀 설정 |
| 권한 검사 | `room_members.is_admin=1 AND user_id=요청자` |

## FR-G07 공동 방장 · P2

| 패킷 | `ROOM_GRANT_ADMIN` / `ROOM_REVOKE_ADMIN` |
| 권한 | 방장(owner)만 가능 — **is_admin 만으로는 grant 불가**. 오너는 `rooms.owner_id` 로 판별 |

## FR-G08 공지 등록 · P2

| 패킷 | `ROOM_SET_NOTICE` → `ROOM_NOTICE` 브로드캐스트 |
| 저장 | `rooms.notice` 컬럼 |

## FR-G09 메시지 히스토리 · P0

| 패킷 | `ROOM_JOIN` 성공 시 자동으로 최근 100개 전송 (`ROOM_MSG_RECV` 반복) |
| 쿼리 | `SELECT ... FROM messages WHERE room_id=? ORDER BY id DESC LIMIT 100` → 역순 출력 |

## FR-G10 멤버 목록 · P2

| 패킷 | `ROOM_MEMBERS_REQ` / `ROOM_MEMBERS_RES` |
| 쿼리 | `SELECT u.id, u.nickname, rm.is_admin, u.online_status FROM room_members rm JOIN users u ON rm.user_id=u.id WHERE rm.room_id=?` |

## 관련 문서

- 패킷: [`08_api/packets/room.md`](../08_api/packets/room.md)
- 테이블: [`rooms.md`](../07_database/tables/rooms.md), [`room_members.md`](../07_database/tables/room_members.md)
