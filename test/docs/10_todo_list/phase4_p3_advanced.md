# Phase 4 — P3 Advanced

> **전제**: Phase 3 (P2 Enhancements) 완료 후 시작.  
> 이 Phase는 선택적 고급 기능으로, MVP 완성 후 구현한다.

---

### [P3-01] 메시지 고급 기능 (답장·이모티콘·검색·/me)

- **구현 파일**:
  - `chat_program/src/server/message.c/h` (P2-01 확장)
  - `chat_program/src/client/screen_chat.c/h` (P0-07 확장)
- **참조 문서**:
  - `docs/02_features/message.md` — FR-M04(답장), FR-M06(이모티콘), FR-M08(메시지 검색), FR-M11(/me 액션)
  - `docs/08_api/packets/message.md` — `MSG_REPLY`, `MSG_EMOTE`, `MSG_SEARCH` 패킷
- **수용 기준**:
  - `MSG_REPLY|msg_id|content\n` → `reply_to_id` 필드 포함 메시지 저장 → 인용 버블 렌더링
  - `MSG_EMOTE|code\n` → 유니코드 이모티콘 코드 전송 → 이모티콘 버블 렌더링
  - `MSG_SEARCH|room_id|keyword\n` → `content LIKE ?` 검색 → 결과 목록 응답
  - `/me action` 입력 → 이탤릭 시스템 메시지 스타일로 렌더링

---

### [P3-02] 오픈채팅 고급 기능 (방 검색·익명 닉네임)

- **구현 파일**:
  - `chat_program/src/server/room.c/h` (P0-04 확장)
  - `chat_program/src/client/screen_open_chat.c/h` (P0-07 확장)
- **참조 문서**:
  - `docs/02_features/open_room.md` — FR-O03(방 검색), FR-O05(익명 닉네임)
  - `docs/02_features/customization.md` — FR-C07(오픈채팅 닉네임)
  - `docs/08_api/packets/room.md` — `OPEN_SEARCH`, `OPEN_NICK` 패킷
- **수용 기준**:
  - `OPEN_SEARCH|keyword\n` → `room_name LIKE ?` 검색 → 오픈방 목록 응답
  - `OPEN_NICK|room_id|anon_nickname\n` → 해당 방에서만 사용할 닉네임 설정
  - 익명 닉네임은 서버 메모리 저장 (DB 저장 안 함)
  - 검색 결과 화면: `GtkListBox` + 방 이름·인원수·설명 표시

---

### [P3-03] 알림 고급 기능 (멘션 알림·DND·타이핑 표시·방 무음)

- **구현 파일**:
  - `chat_program/src/server/broadcast.c/h` (확장)
  - `chat_program/src/client/screen_chat.c/h` (확장)
  - `chat_program/src/client/notify.c/h` (확장)
- **참조 문서**:
  - `docs/02_features/notification.md` — FR-N03(멘션 알림), FR-N04(DND 예외 설정), FR-N05(타이핑 표시), FR-N06(방 무음)
  - `docs/08_api/packets/typing.md` — `TYPING_START`, `TYPING_STOP` 패킷
- **수용 기준**:
  - `@nickname` 파싱 → 멘션된 유저에게 별도 `NOTIFY|mention|...` 전송
  - DND 모드 예외: `@mention`은 DND에도 전달
  - `TYPING_START|room_id\n` → 방 멤버에게 "입력 중..." 표시 브로드캐스트
  - `TYPING_STOP|room_id\n` → "입력 중..." 해제
  - 클라이언트: 입력 필드 `GtkEntry::changed` → 1초 디바운스 후 `TYPING_START` 전송
  - 방 무음: 해당 방의 `NOTIFY` 패킷 클라이언트에서 무음 처리

---

### [P3-04] Keepalive (PING/PONG)

- **구현 파일**:
  - `chat_program/src/server/client_handler.c/h` (확장)
  - `chat_program/src/client/net.c/h` (확장)
- **참조 문서**:
  - `docs/08_api/packets/keepalive.md` — `PING`/`PONG` 패킷 정의
  - `docs/08_api/packet_format.md` — §6 Keepalive 규칙
- **수용 기준**:
  - 클라이언트: 30초마다 `PING\n` 전송
  - 서버: `PING\n` 수신 → 즉시 `PONG\n` 응답
  - 서버: 90초 이내 PING 없으면 연결 강제 종료 + 세션 청소
  - 클라이언트: PONG 미수신 60초 → 자동 재연결 시도
