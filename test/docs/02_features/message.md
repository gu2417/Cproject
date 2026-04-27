# 메시지 기능 (FR-M01 ~ M11)

## FR-M01 귓속말 · P2

| 패킷 | `WHISPER` → 대상자에게 `WHISPER_RECV` |
| 저장 | `messages(msg_type=2, to_id=target, room_id=현재 방)` |
| 표시 | 수신자 화면에 `(귓속말) <nick>: ...` 전용 색상 |

## FR-M02 메시지 삭제 · P2

| 패킷 | `MSG_DELETE` → `MSG_DELETED_NOTIFY` 방 전원 |
| 권한 | 본인 메시지만. 관리자는 방 내 누구 것이든 삭제 가능(P4) |
| 로직 | `UPDATE messages SET is_deleted=1`. 클라이언트는 내용을 `삭제된 메시지` 로 표시 |

## FR-M03 메시지 수정 · P2

| 패킷 | `MSG_EDIT` → `MSG_EDITED_NOTIFY` |
| 제약 | `NOW() - created_at <= 5분`, 본인 메시지, 미삭제 |
| 로직 | `UPDATE messages SET content=?, edited_at=NOW()` |
| 표시 | 클라이언트가 `edited_at IS NOT NULL` 이면 `(수정됨)` 표시 |

## FR-M04 답장 · P3

| 패킷 | `MSG_REPLY` → `ROOM_MSG_RECV` 의 `reply_to_id` + `reply_preview` 로 브로드캐스트 |
| 저장 | `messages.reply_to = <원본 msg_id>` |
| preview | 원본 content 20자 + "…" |

## FR-M05 리액션 · **Out-of-Scope**
> 메시지 리액션은 이 프로젝트 구현 범위에 포함되지 않습니다.

## FR-M06 이모티콘 변환 · P3

| 방식 | 클라이언트가 **송신 직전** 에 `:smile:` → `(^_^)` 치환. 서버는 변환하지 않음 |
| 대응 | `:smile: :heart: :cry: :angry: :wink: :thumbsup: :fire: :tada:` 등 기본 세트 |

## FR-M07 시스템 메시지 · P2

| 종류 | 입장/퇴장/초대/강퇴/공지/핀/방 생성 |
| 저장 | `messages(msg_type=1, from_id='system')` — system 은 예약 user_id 로 `users` 에 1건 시드 |
| 표시 | 회색 색상, 닉네임 없이 `시스템:` 프리픽스 |

## FR-M08 메시지 검색 · P3

| 패킷 | `MSG_SEARCH` / `MSG_SEARCH_RES` |
| 쿼리 | `SELECT ... WHERE room_id=? AND is_deleted=0 AND content LIKE ? ORDER BY id DESC LIMIT 30` |

## FR-M09 타임스탬프 · P2

| 형식 | `user_settings.ts_format`: 0=`HH:MM`, 1=`HH:MM:SS`, 2=`MM-DD HH:MM` |
| 변환 | 클라이언트에서 수행 |

## FR-M10 핀 메시지 · P2

| 패킷 | `MSG_PIN` → `MSG_PIN_NOTIFY` |
| 저장 | `rooms.pinned_msg_id = <msg_id>`. 해제는 `msg_id=NULL` |
| 권한 | 방장/공동 방장 |

## FR-M11 /me 액션 · P3

| 패킷 | `ROOM_MSG` 에 content 가 `/me ...` 이면 서버가 `msg_type=3` 로 저장 |
| 표시 | `* 홍길동 손을 흔든다` 이탤릭(ANSI `\e[3m`) |

## 관련 문서

- 패킷: [`08_api/packets/message.md`](../08_api/packets/message.md)
- 테이블: [`messages.md`](../07_database/tables/messages.md), [`room_invites.md`](../07_database/tables/room_invites.md)
