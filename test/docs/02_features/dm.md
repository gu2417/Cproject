# 1:1 DM (FR-D01 ~ D05)

`messages.room_id IS NULL` + `from_id`, `to_id` 쌍으로 저장. 별도 DM 채팅방 레코드는 생성하지 않고 상대 id 를 "가상 방" 키로 사용한다.

## FR-D01 DM 시작 · P1

| 입력 | 상대 id (친구 목록 선택 또는 직접 입력) |
| 로직 | DM 은 **레코드 생성 없이** 클라이언트 화면만 전환. 첫 메시지 전송 시점에 `messages` 에 insert. |
| 차단 검사 | 상대가 나를 차단했거나 내가 상대를 차단했으면 `DM_SEND` 거부 |

## FR-D02 메시지 전송 · P1

| 패킷 | `DM_SEND` → 저장 후 수신자에게 `DM_RECV` |
| 제한 | content ≤500자 |
| 로직 | ① 차단 검사 → ② `INSERT messages(room_id=NULL, from_id, to_id, content)` → ③ 상대 접속 중이면 `DM_RECV` 송신 → ④ 발신자 본인에게도 `DM_RECV` 에코(메시지 id 공유용) |

## FR-D03 읽음 확인 · P1

| 패킷 | 수신자가 해당 DM 화면에서 조회하는 순간 → 서버가 `INSERT INTO dm_reads` → 발신자에게 `DM_READ_NOTIFY` |
| 표시 | 발신자 메시지 옆 `[읽음]` 배지 |

## FR-D04 메시지 히스토리 · P1

| 패킷 | `DM_HISTORY_REQ` / `DM_HISTORY_RES` |
| 기본 | 최근 50개 |
| 쿼리 | `SELECT ... FROM messages WHERE room_id IS NULL AND ((from_id=:me AND to_id=:peer) OR (from_id=:peer AND to_id=:me)) ORDER BY id DESC LIMIT 50` |

## FR-D05 안읽은 메시지 수 · P1

| 쿼리 | `SELECT COUNT(*) FROM messages m WHERE m.to_id=:me AND m.room_id IS NULL AND NOT EXISTS(SELECT 1 FROM dm_reads r WHERE r.msg_id=m.id AND r.reader_id=:me)` |
| 표시 | 메인 화면 DM 목록에 뱃지 |

## 관련 문서

- 패킷: [`08_api/packets/dm.md`](../08_api/packets/dm.md)
- 테이블: [`messages.md`](../07_database/tables/messages.md), [`dm_reads.md`](../07_database/tables/dm_reads.md)
