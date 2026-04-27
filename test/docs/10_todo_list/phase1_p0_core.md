# Phase 1 — P0 Core

> **전제**: Phase 0 (Foundation) 완료 후 시작.  
> 이 Phase가 끝나면 로그인·회원가입·그룹 채팅·오픈채팅이 동작하는 MVP가 완성된다.

---

### [P0-01] 서버 인증 모듈 (회원가입·로그인·로그아웃)

- **구현 파일**: `chat_program/src/server/auth.c/h`
- **참조 문서**:
  - `docs/02_features/account.md` — FR-A01(회원가입), FR-A02(로그인), FR-A03(로그아웃)
  - `docs/08_api/packets/auth.md` — `REGISTER`/`REGISTER_RES`, `LOGIN`/`LOGIN_RES`, `LOGOUT` 패킷
  - `docs/05_security/auth_and_password.md` — bcrypt 단방향 해시, 세션 토큰 규칙
  - `docs/07_database/query_catalog.md` — `§인증` 섹션 Prepared Statement
- **수용 기준**:
  - `REGISTER|id|pw|nickname\n` 수신 → 중복 확인 → bcrypt 해시 → DB INSERT → `REGISTER_RES|ok\n`
  - `LOGIN|id|pw\n` 수신 → 해시 검증 → `clients[]`에 socket_fd 등록 → `LOGIN_RES|ok|nickname\n`
  - `LOGOUT|id\n` 수신 → socket_fd 초기화 → 온라인 상태 갱신
  - 실패 시 `REGISTER_RES|fail|reason\n`, `LOGIN_RES|fail|reason\n`

---

### [P0-02] 서버 유저 스토어

- **구현 파일**: `chat_program/src/server/user_store.c/h`
- **참조 문서**:
  - `docs/04_file_structure/server_modules.md`
  - `docs/07_database/tables/users.md`
  - `docs/07_database/query_catalog.md` — `§유저` 섹션
- **수용 기준**:
  - `user_find_by_id()`, `user_find_by_socket()` 조회 함수
  - `user_set_online()`, `user_set_offline()` 상태 변경
  - `clients[]` 배열 뮤텍스 보호

---

### [P0-03] 서버 그룹방 핵심 (방 생성·참여·퇴장·목록)

- **구현 파일**: `chat_program/src/server/room.c/h`
- **참조 문서**:
  - `docs/02_features/group_room.md` — FR-G01(방 생성), FR-G03(참여), FR-G05(퇴장), FR-G09(방 목록)
  - `docs/08_api/packets/room.md` — `ROOM_CREATE`, `ROOM_JOIN`, `ROOM_LEAVE`, `ROOM_LIST` 패킷
  - `docs/07_database/tables/rooms.md`, `docs/07_database/tables/room_members.md`
  - `docs/07_database/query_catalog.md` — `§방` 섹션
- **수용 기준**:
  - `ROOM_CREATE|name|member_ids\n` → DB INSERT → 멤버 초대 알림 브로드캐스트
  - `ROOM_JOIN|room_id\n` → `room_members` INSERT → `member_fds[]` 갱신
  - `ROOM_LEAVE|room_id\n` → `room_members` DELETE → 시스템 메시지 브로드캐스트
  - `ROOM_LIST\n` → 내가 속한 방 목록 + 미읽 메시지 수 응답

---

### [P0-04] 서버 오픈채팅 핵심 (방 생성·참여·메시지)

- **구현 파일**: `chat_program/src/server/room.c/h` (P0-03 확장)
- **참조 문서**:
  - `docs/02_features/open_room.md` — FR-O01(오픈방 생성), FR-O02(참여), FR-O04(메시지 전송)
  - `docs/08_api/packets/room.md` — `OPEN_CREATE`, `OPEN_JOIN`, `OPEN_CHAT` 패킷
  - `docs/07_database/tables/rooms.md` — `is_open=1` 구분
- **수용 기준**:
  - `OPEN_CREATE|name|max_members\n` → `is_open=1` 방 생성
  - `OPEN_JOIN|room_id\n` → 정원 확인 후 참여 (로그인 불필요 또는 게스트 허용 여부 확인)
  - `OPEN_CHAT|room_id|content\n` → 방 멤버 전체 브로드캐스트

---

### [P0-05] 클라이언트 로그인 화면

- **구현 파일**: `chat_program/src/client/screen_login.c/h`
- **참조 문서**:
  - `docs/06_ui_ux/screens/login.md` — 화면 레이아웃, 위젯 트리, 전환 조건
  - `docs/06_ui_ux/screen_flow.md` — 로그인 성공 시 `main` 화면 전환
  - `docs/04_file_structure/client_modules.md`
- **수용 기준**:
  - `GtkEntry` (ID·PW) + `GtkButton` (로그인·회원가입)
  - 로그인 성공 → `GtkStack` 전환: `login` → `main`
  - 실패 시 `GtkLabel` 오류 메시지 표시
  - 회원가입 폼: ID·PW·닉네임 입력, `REGISTER` 패킷 전송

---

### [P0-06] 클라이언트 메인 화면 (탭 기반)

- **구현 파일**: `chat_program/src/client/screen_main.c/h`
- **참조 문서**:
  - `docs/06_ui_ux/screens/main.md` — 탭 구조 (친구/채팅방/오픈채팅), 헤더바 레이아웃
  - `docs/06_ui_ux/screen_flow.md` — 탭 전환 이벤트, 알림 버튼(🔔)
  - `docs/06_ui_ux/components.md` — `FriendItem`, `RoomItem`, `UnreadBadge`
  - `docs/04_file_structure/client_modules.md`
- **수용 기준**:
  - `GtkNotebook` 또는 `GtkStack`+탭 버튼으로 3탭 구현
  - 🔔 알림 버튼 헤더에 배치 (클릭 → `notify_panel` 팝오버, P2-06에서 구현)
  - 방 목록 `GtkListBox` + 미읽 뱃지 표시
  - 방 클릭 → `screen_chat` 화면 전환

---

### [P0-07] 클라이언트 채팅 화면 + 오픈채팅 화면 + 방 뷰

- **구현 파일**:
  - `chat_program/src/client/screen_chat.c/h`
  - `chat_program/src/client/screen_open_chat.c/h`
  - `chat_program/src/client/room_view.c/h`
- **참조 문서**:
  - `docs/06_ui_ux/screens/chat.md` — 채팅 화면 위젯 트리, 메시지 버블 레이아웃
  - `docs/06_ui_ux/screens/open_chat.md` — 오픈채팅 화면
  - `docs/06_ui_ux/components.md` — `MessageBubble`, `Avatar`, `StatusDot`
  - `docs/06_ui_ux/screen_flow.md` — 메시지 수신 시 스크롤 자동 이동
  - `docs/04_file_structure/client_modules.md`
  - `docs/03_architecture/threading_model.md` — `g_idle_add()` UI 업데이트 패턴
- **수용 기준**:
  - `GtkScrolledWindow` + `GtkListBox`로 메시지 목록
  - 내 메시지(우측 정렬·파란 버블) / 상대 메시지(좌측 정렬·회색 버블)
  - `GtkEntry` + 전송 버튼 → `CHAT|room_id|content\n` 패킷 전송
  - recv 스레드에서 `g_idle_add(append_message_cb, data)` 패턴으로 UI 갱신
  - `room_view.c/h`: 현재 활성 room_id 상태 관리, 화면 초기화/해제
