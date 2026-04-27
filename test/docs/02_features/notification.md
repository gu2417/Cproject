# 알림 (FR-N01 ~ N06)

모든 알림은 서버가 `NOTIFY|<type>:<content>` 패킷으로 전송하거나, 전용 패킷(`FRIEND_REQUEST_NOTIFY`) 으로 전송한다.
클라이언트는 상단 배너 큐로 표시하고, 3초 후 자동 사라지도록 한다(저사양 고려: 타이머 없이 다음 입력/렌더 시점에 만료 체크).

## FR-N01 메시지 알림 · P2

- 현재 `current_room_id` 가 아닌 방에서 메시지가 도착하면 서버가 `NOTIFY(type=ROOM_MSG, content=방명/닉/미리보기)` 전송.
- DND 상태에서는 suppress. 단, `type=MENTION` 은 **항상** 통과.

## FR-N02 친구 요청 알림 · P2

- 로그인 시: `SELECT ... FROM friends WHERE friend_id=? AND status=0` 결과를 개별 `FRIEND_REQUEST_NOTIFY` 로 전송.
- 실시간: 상대가 요청 보낸 순간 `FRIEND_REQUEST_NOTIFY` 즉시 전송.

## FR-N03 멘션 알림 · P3

- content 파싱 시 `@nick` 발견 → 해당 `user_id` 조회 → `NOTIFY(type=MENTION, content=방명/내용)` 전송.
- DND 상태 무시.

## FR-N04 DND 모드 · P3

| 패킷 | `STATUS_CHANGE` 또는 클라이언트 로컬 토글 후 `SETTINGS_UPDATE` |
| 저장 | `users.dnd` (0/1) |
| 동작 | N01 억제, N03 통과 |

## FR-N05 타이핑 표시 · P3

| 패킷 | `TYPING_START` / `TYPING_STOP` → 방 멤버에게 `TYPING_NOTIFY` |
| 서버 동작 | **DB 저장하지 않음**. 인메모리로도 상태를 보존하지 않고 단순 릴레이 |
| 자동 해제 | 클라이언트가 3초 유휴 시 `TYPING_STOP` 송신. 수신측은 5초 내 `TYPING_NOTIFY(is_typing=1)` 미수신 시 자동 제거 |

## FR-N06 방 알림 무음 · P3

| 패킷 | 별도 패킷 미정의(P3) — `SETTINGS_UPDATE` 확장으로 구현 검토 |
| 설계 대안 | `room_members.is_muted` 컬럼 사용. 방 메시지 도착 시 서버가 해당 멤버에게 `NOTIFY` 를 보내지 않음 |

## 관련 문서

- 패킷: [`08_api/packets/notify.md`](../08_api/packets/notify.md), [`typing.md`](../08_api/packets/typing.md)
