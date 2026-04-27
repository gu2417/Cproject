# Phase 3 — P2 Enhancements

> **전제**: Phase 2 (P1 Social) 완료 후 시작.  
> 이 Phase가 끝나면 메시지 편집·삭제, 알림 시스템, 커스터마이징, 방 관리 기능이 동작한다.

---

### [P2-01] 서버 메시지 모듈 (귓속말·삭제·수정·시스템 메시지·타임스탬프·핀)

- **구현 파일**: `chat_program/src/server/message.c/h`
- **참조 문서**:
  - `docs/02_features/message.md` — FR-M01(귓속말), FR-M02(메시지 삭제), FR-M03(메시지 수정), FR-M07(시스템 메시지), FR-M09(날짜 구분선·타임스탬프), FR-M10(핀 메시지)
  - `docs/08_api/packets/message.md` — `WHISPER`, `MSG_DELETE`, `MSG_EDIT`, `MSG_PIN` 패킷
  - `docs/07_database/tables/messages.md` — `is_deleted`, `edited_at`, `is_pinned` 필드
  - `docs/07_database/query_catalog.md` — `§메시지` 섹션
- **수용 기준**:
  - `WHISPER|to_id|content\n` → 송수신자만 전달 (room_id 없음)
  - `MSG_DELETE|msg_id\n` → 권한 확인(본인/방장) → `is_deleted=1` soft-delete → 방 전체 알림
  - `MSG_EDIT|msg_id|new_content\n` → 권한 확인(본인) → 내용 UPDATE + `edited_at` 갱신
  - 시스템 메시지: `from_id='system'` 예약 계정 사용
  - `MSG_PIN|msg_id\n` → 방장만 가능 → `is_pinned=1` → 방 전체 알림

---

### [P2-02] 서버 그룹방 고급 운영 (멘션·방장 권한·공지·ROOM_EDIT·ROOM_DELETE)

- **구현 파일**: `chat_program/src/server/room.c/h` (P0-03 확장)
- **참조 문서**:
  - `docs/02_features/group_room.md` — FR-G04(멘션), FR-G06(방장 권한 이전), FR-G07(공동방장), FR-G08(방 공지), FR-G10(멤버 목록)
  - `docs/08_api/packets/room.md` — `ROOM_NOTICE`, `ROOM_KICK`, `ROOM_PROMOTE`, `ROOM_MEMBERS` 패킷
  - `docs/06_ui_ux/screens/room_settings.md` — **신규 패킷**: `ROOM_INFO`, `ROOM_EDIT`, `ROOM_DELETE` 정의
  - `docs/07_database/query_catalog.md` — `§방운영` 섹션
- **수용 기준**:
  - `ROOM_NOTICE|room_id|content\n` → 방장/공동방장만 → 시스템 메시지로 브로드캐스트
  - `ROOM_PROMOTE|room_id|target_id\n` → 방장 권한 이전 또는 공동방장 지정
  - `ROOM_KICK|room_id|target_id\n` → 방장만 가능 → 강제 퇴장 + 알림
  - `ROOM_MEMBERS|room_id\n` → 멤버 목록 + 역할(방장/공동/일반) 응답
  - `ROOM_INFO|room_id\n` → 방 상세 정보 응답 (신규)
  - `ROOM_EDIT|room_id|new_name\n` → 방장만 가능 → 방 이름 변경 (신규)
  - `ROOM_DELETE|room_id\n` → 방장만 가능 → messages soft-delete 후 rooms DELETE (신규)
  - `@nickname` 멘션 파싱 → 해당 유저에게 별도 알림

---

### [P2-03] 서버 알림 시스템 (메시지 알림·친구 요청 알림)

- **구현 파일**: `chat_program/src/server/broadcast.c/h` (확장) + `chat_program/src/server/notify.c/h`
- **참조 문서**:
  - `docs/02_features/notification.md` — FR-N01(메시지 알림), FR-N02(친구 요청 알림)
  - `docs/08_api/packets/notify.md` — `NOTIFY` 패킷 형식
  - `docs/03_architecture/data_flow.md` — 알림 전달 흐름
  - `docs/07_database/tables/notifications.md`
- **수용 기준**:
  - 오프라인 유저: DB `notifications` 테이블에 저장
  - 온라인 복귀 시 대기 알림 일괄 전송
  - `NOTIFY|type|from_id|room_id|content\n` 패킷 포맷
  - 알림 5종: `friend_request`, `mention`, `dm`, `room_invite`, `system`

---

### [P2-04] 서버 커스터마이징 (색상·테마·DND·온라인 상태)

- **구현 파일**: `chat_program/src/server/auth.c/h` 또는 `chat_program/src/server/settings.c/h`
- **참조 문서**:
  - `docs/02_features/customization.md` — FR-C01(아바타 색상), FR-C02(테마), FR-C03(온라인 상태), FR-C04(상태 메시지), FR-C05(DND 모드), FR-C06(방 알림 무음)
  - `docs/08_api/packets/settings.md` — `SETTINGS_UPDATE`/`SETTINGS_UPDATE_RES` 패킷
  - `docs/07_database/query_catalog.md` — `§알림설정` 섹션
- **수용 기준**:
  - `SETTINGS_UPDATE|field|value\n` → DB UPDATE → 클라이언트 즉시 반영
  - `invisible` 상태: DB에 저장 안 하고 서버 메모리에서 `online_status=0` 마스킹
  - DND 모드: `dnd_mode=1` → 방 알림 패킷 차단
  - 방 무음: `room_mutes` 테이블 UPSERT

---

### [P2-05] 클라이언트 설정 화면 + 알림 배너

- **구현 파일**:
  - `chat_program/src/client/screen_settings.c/h`
  - `chat_program/src/client/notify.c/h`
- **참조 문서**:
  - `docs/06_ui_ux/screens/settings.md` — 설정 화면 레이아웃
  - `docs/06_ui_ux/interaction_patterns.md` — 배너 알림 UX 패턴
  - `docs/06_ui_ux/components.md` — `NotificationBanner` 컴포넌트 명세
  - `docs/04_file_structure/client_modules.md`
- **수용 기준**:
  - `screen_settings.c/h`: 테마 토글, 온라인 상태 선택(`GtkComboBox`), DND 스위치
  - `notify.c/h`: `GtkRevealer` 기반 배너 최대 3개 큐, 3초 후 자동 해제
  - 배너 클릭 → 관련 화면으로 이동 (방 알림 → 해당 방, 친구 요청 → 친구 탭)

---

### [P2-06] 클라이언트 알림 패널 팝오버 (신규)

- **구현 파일**: `chat_program/src/client/notify_panel.c/h`
- **참조 문서**:
  - `docs/06_ui_ux/screens/notifications.md` — **신규 문서**: `GtkPopover` 기반 알림 패널 전체 명세
  - `docs/06_ui_ux/screen_flow.md` — 🔔 버튼 클릭 → 팝오버 열기 이벤트
  - `docs/06_ui_ux/components.md` — 알림 항목 컴포넌트
- **수용 기준**:
  - 🔔 버튼(`GtkMenuButton`) 클릭 → `GtkPopover` 알림 목록 표시
  - `Notification` 구조체 `GPtrArray` 최대 50개 보관
  - 알림 5종 표시: 친구요청/멘션/DM/방초대/시스템
  - 읽지 않은 알림 수 뱃지 표시 (`GtkLabel` 오버레이)
  - "모두 읽음" 버튼, 개별 알림 클릭 → 관련 화면 이동

---

### [P2-07] 클라이언트 방 정보/관리 모달 (신규)

- **구현 파일**: `chat_program/src/client/room_settings_dialog.c/h`
- **참조 문서**:
  - `docs/06_ui_ux/screens/room_settings.md` — **신규 문서**: 방 설정 모달 전체 명세, `ROOM_INFO`/`ROOM_EDIT`/`ROOM_DELETE` 패킷 정의
  - `docs/06_ui_ux/screen_flow.md` — 방 이름 클릭 → 설정 모달 열기 이벤트
  - `docs/06_ui_ux/components.md` — `Avatar`, `MemberRow` 재사용
- **수용 기준**:
  - `GtkWindow` (모달) — 탭: "방 정보" / "멤버 관리"
  - 방 정보 탭: 방 이름 표시, 방장이면 편집 가능 (`ROOM_EDIT` 전송)
  - 멤버 관리 탭: 멤버 목록 + 방장이면 강퇴/공동방장 버튼
  - 방 삭제 버튼 (방장만, 확인 다이얼로그 후 `ROOM_DELETE` 전송)
  - `ROOM_INFO` 응답 수신 → 모달 내용 채우기
