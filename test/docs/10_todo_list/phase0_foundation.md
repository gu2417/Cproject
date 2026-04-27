# Phase 0 — Foundation

> **선행 필수**: 이 Phase가 완료되어야 모든 후속 Phase를 시작할 수 있다.

---

### [F0-01] Makefile 작성

- **구현 파일**: `Makefile` (프로젝트 루트)
- **참조 문서**:
  - `docs/03_architecture/build_and_portability.md`
  - `docs/04_file_structure/repo_layout.md`
- **수용 기준**:
  - `make` 실행 시 서버·클라이언트 바이너리 생성
  - Linux(`gcc`) 및 Windows MinGW(`x86_64-w64-mingw32-gcc`) 듀얼 빌드 지원
  - `make clean` 동작
  - GTK4 pkg-config 플래그, pthread, libmysqlclient 링크 포함

---

### [F0-02] sql/schema.sql 작성

- **구현 파일**: `sql/schema.sql` (프로젝트 루트)
- **참조 문서**:
  - `docs/07_database/er_diagram.md` — 전체 ERD
  - `docs/07_database/tables/users.md`
  - `docs/07_database/tables/rooms.md`
  - `docs/07_database/tables/room_members.md`
  - `docs/07_database/tables/messages.md`
  - `docs/07_database/tables/friends.md`
  - `docs/07_database/tables/dm_reads.md`
  - `docs/07_database/tables/notifications.md`
  - `docs/07_database/migration_and_seed.md` — 시드 데이터 (`system` 예약 user_id)
  - `docs/07_database/indexes_and_performance.md` — 인덱스 정의
  - ⚠️ `docs/07_database/tables/reactions.md` — **Out-of-Scope, 구현 제외**
- **수용 기준**:
  - MySQL 5.7+ 호환 DDL
  - 모든 테이블 CREATE + 인덱스 CREATE
  - `system` 예약 계정 시드 INSERT
  - `SOURCE sql/schema.sql` 한 번 실행으로 초기화 완료

---

### [F0-03] common/protocol.h 검증 및 완성

- **구현 파일**: `chat_program/src/common/protocol.h` ✅ (기존 파일 검증)
- **참조 문서**:
  - `docs/08_api/packet_format.md` — 패킷 형식 전체 규칙
  - `docs/08_api/packets/auth.md`
  - `docs/08_api/packets/room.md`
  - `docs/08_api/packets/message.md`
  - `docs/08_api/packets/friend.md`
  - `docs/08_api/packets/dm.md`
  - `docs/08_api/packets/notify.md`
  - `docs/08_api/packets/settings.md`
  - `docs/08_api/packets/keepalive.md`
  - `docs/08_api/packets/typing.md`
  - `docs/06_ui_ux/screens/user_profile.md` — `USER_VIEW`/`USER_VIEW_RES` 신규 패킷
  - `docs/06_ui_ux/screens/room_settings.md` — `ROOM_INFO`/`ROOM_EDIT`/`ROOM_DELETE` 신규 패킷
  - `docs/04_file_structure/common_modules.md`
- **수용 기준**:
  - 모든 패킷 타입 상수 (`PKT_*`) 정의 완료
  - 구분자 상수: `FIELD_SEP '|'`, `MULTI_SEP ':'`, `LIST_SEP ';'`, `PKT_TERM '\n'`
  - 최대 패킷 크기: `MAX_PACKET_SIZE 2048`
  - 신규 패킷 (`USER_VIEW`, `ROOM_INFO`, `ROOM_EDIT`, `ROOM_DELETE`) 포함

---

### [F0-04] common/types.h 검증 및 완성

- **구현 파일**: `chat_program/src/common/types.h` ✅ (기존 파일 검증)
- **참조 문서**:
  - `docs/07_database/er_diagram.md` — 필드명·타입 일치 확인
  - `docs/04_file_structure/common_modules.md`
- **수용 기준**:
  - `User` 구조체: `user_id[21]`, `password_hash[65]`, `nickname[21]`, `profile_msg[101]`, `online_status`, `socket_fd`, `dnd_mode`
  - `ChatRoom` 구조체: `room_id`, `room_name[51]`, `owner_id[21]`, `is_open`, `member_count`, `member_fds[64]`
  - `Message` 구조체: `msg_id`, `room_id` (0=DM), `from_id[21]`, `to_id[21]`, `content[501]`, `is_deleted`, `read_count`
  - `FriendEntry` 구조체: `status` (0=pending, 1=accepted, 2=blocked)
  - `Notification` 구조체: `type`(5종), `from_id[21]`, `room_id`, `content[101]`, `is_read`

---

### [F0-05] common/utils.c/h 검증 및 완성

- **구현 파일**: `chat_program/src/common/utils.c/h` ✅ (기존 파일 검증)
- **참조 문서**:
  - `docs/04_file_structure/common_modules.md`
- **수용 기준**:
  - `format_timestamp()` — `YYYY-MM-DD HH:MM:SS` 포맷
  - `time_ago()` — "방금 전", "N분 전", "N시간 전" 등 상대적 표현
  - `fnv1a_hash()` — Avatar 색상 결정용 해시 (FNV-1a)
  - `trim_whitespace()`, `safe_strncpy()` 유틸

---

### [F0-06] common/net_compat (플랫폼 소켓 호환 레이어)

- **구현 파일**:
  - `chat_program/src/common/net_compat.h`
  - `chat_program/src/common/net_posix.c`
  - `chat_program/src/common/net_win.c`
- **참조 문서**:
  - `docs/03_architecture/build_and_portability.md`
  - `docs/04_file_structure/common_modules.md`
- **수용 기준**:
  - Linux: `<sys/socket.h>`, `<netinet/in.h>` 사용
  - Windows: `<winsock2.h>` 사용, `WSAStartup`/`WSACleanup` 래퍼
  - `#ifdef _WIN32` 가드로 분기
  - `sock_close()`, `sock_set_nonblock()` 공통 인터페이스

---

### [F0-07] 서버 기반 모듈 (config + globals + db)

- **구현 파일**:
  - `chat_program/src/server/config.h`
  - `chat_program/src/server/globals.c/h`
  - `chat_program/src/server/db.c/h`
- **참조 문서**:
  - `docs/04_file_structure/server_modules.md`
  - `docs/07_database/connection_pooling.md` — 스레드별 전용 MYSQL\* 핸들 규칙
  - `docs/07_database/query_catalog.md` — Prepared Statement 패턴
  - `docs/03_architecture/threading_model.md`
- **수용 기준**:
  - `config.h`: `DEFAULT_PORT 8080`, `MAX_CLIENTS 100`, DB 연결 파라미터 상수
  - `globals.c/h`: `User clients[]`, `ChatRoom rooms[]`, 뮤텍스 선언
  - `db.c/h`: `db_connect()`, `db_disconnect()`, `db_prepare()` 구현
  - 스레드별 `mysql_thread_init()` / `mysql_thread_end()` 호출 보장
  - 모든 쿼리는 Prepared Statement (`?` 플레이스홀더)

---

### [F0-08] 서버 메인 루프 + 핸들러 + 라우터 + 브로드캐스트

- **구현 파일**:
  - `chat_program/src/server/main.c`
  - `chat_program/src/server/client_handler.c/h`
  - `chat_program/src/server/router.c/h`
  - `chat_program/src/server/broadcast.c/h`
- **참조 문서**:
  - `docs/03_architecture/system_context.md`
  - `docs/03_architecture/threading_model.md` — thread-per-client 모델
  - `docs/03_architecture/data_flow.md` — 브로드캐스트 흐름
  - `docs/04_file_structure/server_modules.md`
  - `docs/08_api/packet_format.md` — `TYPE|field1|...\n` 파싱
  - `docs/09_exception/server_error_handling.md`
- **수용 기준**:
  - `main.c`: TCP listen → accept loop → `pthread_create` per client
  - `client_handler.c/h`: recv 루프, `\n` 기준 패킷 분리, router 호출
  - `router.c/h`: 패킷 타입 → 핸들러 함수 디스패치 테이블
  - `broadcast.c/h`: room 멤버 전체 전송, DM 단일 전송, 오프라인 알림 큐

---

### [F0-09] 클라이언트 기반 모듈 (main + app_window + net + packet)

- **구현 파일**:
  - `chat_program/src/client/main.c`
  - `chat_program/src/client/app_window.c/h`
  - `chat_program/src/client/net.c/h`
  - `chat_program/src/client/packet.c/h`
- **참조 문서**:
  - `docs/04_file_structure/client_modules.md`
  - `docs/03_architecture/threading_model.md` — GTK4 메인 루프 + recv 스레드
  - `docs/06_ui_ux/screen_flow.md` — GtkStack 화면 전환 흐름
  - `docs/08_api/packet_format.md`
- **수용 기준**:
  - `main.c`: `gtk_application_new()`, CSS 로드, `app_window` 생성
  - `app_window.c/h`: `GtkStack` 기반 화면 전환 (`login` → `main` → `chat`)
  - `net.c/h`: TCP 연결, recv 스레드 (`g_idle_add()`로 UI 업데이트), send 함수
  - `packet.c/h`: `packet_build()`, `packet_parse()` — `TYPE|f1|f2\n` 직렬화/역직렬화

---

### [F0-10] GTK4 CSS 파일 완성

- **구현 파일**:
  - `chat_program/src/client/css/theme-dark.css` ✅
  - `chat_program/src/client/css/theme-light.css` ✅
  - `chat_program/src/client/css/components.css` ✅
  - `chat_program/src/client/css/chat.css` (미생성)
  - `chat_program/src/client/css/login.css` (미생성)
- **참조 문서**:
  - `docs/06_ui_ux/design_tokens.md` — CSS 변수 체계 (색상·타이포·간격)
  - `docs/06_ui_ux/components.md` — 22종 컴포넌트 명세
  - `docs/06_ui_ux/screens/login.md` — 로그인 화면 레이아웃
  - `docs/06_ui_ux/screens/chat.md` — 채팅 화면 레이아웃
  - `docs/06_ui_ux/low_spec_rendering.md` — 저사양 렌더링 규칙
- **수용 기준**:
  - 기존 3개 파일 내용 `design_tokens.md` 대조 검증
  - `chat.css`: `.message-bubble`, `.message-bubble.mine/other`, `.chat-input-bar` 스타일
  - `login.css`: `.login-box`, `.login-logo`, `.login-form` 스타일
  - 다크/라이트 CSS 변수 전환: `@define-color` 기반
