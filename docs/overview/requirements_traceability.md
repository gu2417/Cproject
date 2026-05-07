# requirements.md ↔ docs 추적성 매트릭스

requirements.md 모든 FR / NFR / 패킷 항목이 docs의 어느 파일에서 다뤄지는지 정리. 갭 분석(2026-05-07) 후 정기 재검증용.

> **약식 표기**: `pkt` = `protocol/packet_reference.md`, `ui_map` = `features/ui_mapping.md`, `inv` = `uiux/screen_inventory.md`, `arch` = `architecture/`, `db` = `database/`, `sec` = `security/`, `feat` = `features/`, `phase` = `development/development_phases.md`.

---

## 1. 기능 요구사항 (FR)

### FR-A: 계정 관리

| FR | 항목 | 패킷 | 데이터 | UI | 모듈 | Phase |
|----|------|------|--------|-----|------|-------|
| FR-A01 | 회원가입 | `REGISTER_REQ/RES` (pkt §1) | `users.txt` (db §1) | CUI §3, DLG §22 (ui_map) | `module_auth.md` | P0 |
| FR-A02 | 로그인 | `LOGIN_REQ/RES` (pkt §1) | `users.txt` | CUI §2, §35 | `module_auth.md` | P0 |
| FR-A03 | 로그아웃 | `LOGOUT_REQ/RES` (pkt §1) | `users.txt` last_seen | DLG §1 | `module_auth.md` | P0(소켓) / P1(packet) |
| FR-A04 | 프로필 수정 | `PROFILE_UPDATE/RES` (pkt §6) | `users.txt` | CUI §28 | `user_store.c` | P1 |
| FR-A05 | 비밀번호 변경 | `PASS_CHANGE/RES` (pkt §6) | `users.txt` pw_hash | CUI §29 | `module_auth.md` | P1 |
| FR-A06 | 마지막 접속 시간 | `users.txt` last_seen 필드 | db §1 | CUI §38 | `user_store.c` | P1 |

### FR-F: 친구 관리

| FR | 항목 | 패킷 | 데이터 | UI | 모듈 | Phase |
|----|------|------|--------|-----|------|-------|
| FR-F01 | 친구 추가 | `FRIEND_ADD_REQ/RES` (pkt §2) | `friends.txt` (db §4) | CUI §6, DLG §13 | `module_friend.md` | P1 |
| FR-F02 | 수락/거절 | `FRIEND_ACCEPT`, `FRIEND_REJECT` (pkt §2) | `friends.txt` | CUI §7, NTF §2-2 | `module_friend.md` | P1 |
| FR-F03 | 목록 조회 | `FRIEND_LIST_REQ/RES` (pkt §2) | `friends.txt` | CUI §5 | `module_friend.md` | P1 |
| FR-F04 | 친구 삭제 | `FRIEND_DELETE` (pkt §2) | `friends.txt` 양방향 삭제 | DLG §5 | `module_friend.md` | P1 |
| FR-F05 | 차단 | `FRIEND_BLOCK` (pkt §2) | `friends.txt` status=2 | DLG §6, §16 | `feat/FR_F_friend.md` 차단 매트릭스 | P1 |
| FR-F06 | 온라인 상태 | `FRIEND_STATUS_CHANGE` (pkt §2) | `users.txt` online_status | CUI §5, NTF §2-4 | `module_friend.md` | P1 |
| FR-F07 | 유저 검색 | `USER_SEARCH/RES` (pkt §2) | `users.txt` | CUI §8 | `module_friend.md` | P1 |

### FR-D: 1:1 DM

| FR | 항목 | 패킷 | 데이터 | UI | 모듈 | Phase |
|----|------|------|--------|-----|------|-------|
| FR-D01 | DM 시작 | `DM_SEND` (pkt §3) | `messages.txt` room_id=0 | CUI §25, §26 | `module_dm.md` | P1 |
| FR-D02 | 메시지 전송 | `DM_SEND` / `DM_RECV` | `messages.txt` (db §3) | CUI §26 | `module_dm.md` | P1 |
| FR-D03 | 읽음 확인 | `DM_READ_NOTIFY` (pkt §3) | `dm_reads.txt` (db §6) | CUI §26 [읽음] | `module_dm.md`, `feat/FR_D_dm.md` | P1 |
| FR-D04 | 히스토리 | `DM_HISTORY_REQ/RES` (pkt §3) | `messages.txt` | CUI §26 자동 50개 | `module_dm.md` | P1 |
| FR-D05 | 안읽음 카운트 | `DM_LIST_REQ/RES` (pkt §3) | `dm_reads.txt`, `room_reads.txt` | CUI §40 | `module_dm.md` | P1 |

### FR-G: 그룹 채팅방

| FR | 항목 | 패킷 | 데이터 | UI | 모듈 | Phase |
|----|------|------|--------|-----|------|-------|
| FR-G01 | 채팅방 생성 | `ROOM_CREATE/RES` (pkt §4) | `rooms.txt` (db §2) | CUI §11, §10 | `module_room.md` | P0 |
| FR-G02 | 멤버 초대 | `ROOM_INVITE/RES`, `ROOM_INVITE_NOTIFY` (pkt §4) | `room_invites.txt` (db §7) | DLG §11, §12 | `module_room.md` 다중초대 | P1 |
| FR-G03 | 메시지 전송 | `ROOM_MSG`, `ROOM_MSG_RECV` (pkt §4) | `messages.txt` | CUI §16 | `module_room.md` | P0 |
| FR-G04 | 멘션(@) | `ROOM_MSG_RECV` 본문 검출 | — | NTF §2-5 | `feat/FR_G_group_chat.md` | P3 |
| FR-G05 | 채팅방 나가기 | `ROOM_LEAVE` (pkt §4) | `room_members.txt` 제거 | DLG §2 | `module_room.md` | P0 |
| FR-G06 | 방장 권한 (강퇴/삭제/공지/핀) | `ROOM_KICK`, `ROOM_DELETE`, `ROOM_SET_NOTICE`, `MSG_PIN` (pkt §4-5) | `rooms.txt`, `room_members.txt` | DLG §3, §4, §10 | `module_room.md` | P2 |
| FR-G07 | 공동 방장 | `ROOM_GRANT_ADMIN`, `ROOM_REVOKE_ADMIN` (pkt §4) | `room_members.txt` is_admin | DLG §18 | `module_room.md` | P2 |
| FR-G08 | 공지사항 | `ROOM_SET_NOTICE`, `ROOM_NOTICE` (pkt §4) | `rooms.txt` notice | CUI §23, NTF §2-10 | `module_room.md` | P2 |
| FR-G09 | 히스토리 | `ROOM_HISTORY_REQ/RES` (pkt §4) | `messages.txt` + `room_reads.txt` 갱신 | CUI §17 | `module_room.md` open_nick 우선 | P1 |
| FR-G10 | 멤버 목록 | `ROOM_MEMBERS_REQ/RES` (pkt §4) | `room_members.txt` | CUI §20, §21 | `module_room.md` | P2 |

### FR-O: 오픈채팅방

| FR | 항목 | 패킷 | 데이터 | UI | 모듈 | Phase |
|----|------|------|--------|-----|------|-------|
| FR-O01 | 생성 | `ROOM_CREATE` is_open=1 | `rooms.txt` is_open | CUI §13 | `module_room.md` | P0 |
| FR-O02 | 목록 조회 | `ROOM_LIST_REQ/RES` (pkt §4) | `rooms.txt` | CUI §12 | `module_room.md` | P0 |
| FR-O03 | 방 검색 | `ROOM_SEARCH/RES` (pkt §4) | `rooms.txt` | CUI §14 | `module_room.md` | P3 |
| FR-O04 | 자유 참여 | `ROOM_JOIN/RES` (pkt §4) | `room_members.txt` | CUI §10 | `module_room.md` | P0 |
| FR-O05 | 오픈채팅 닉네임 | `ROOM_SET_OPEN_NICK/RES` (pkt §6) | `room_members.txt` open_nick | CUI §15 | `feat/FR_O_open_chat.md` | P2 |

### FR-M: 메시지 부가 기능

| FR | 항목 | 패킷 | 데이터 | UI | 모듈 | Phase |
|----|------|------|--------|-----|------|-------|
| FR-M01 | 귓속말 | `WHISPER`, `WHISPER_RECV` (pkt §5) | `messages.txt` msg_type=2 | NTF §2-6, `/w` | `module_message.md` | P2 |
| FR-M02 | 삭제 | `MSG_DELETE`, `MSG_DELETED_NOTIFY` (pkt §5) | `messages.txt` is_deleted | DLG §7 | `feat/FR_M_message.md` 권한 | P2 |
| FR-M03 | 수정 | `MSG_EDIT`, `MSG_EDITED_NOTIFY` (pkt §5) | `messages.txt` content+edited_at | DLG §8 | `feat/FR_M_message.md` 5분제한 | P2 |
| FR-M04 | 답장 | `MSG_REPLY` → `ROOM_MSG_RECV` (pkt §5) | `messages.txt` reply_to | DLG §9, CUI §19 | `module_message.md` | P3 |
| FR-M05 | 리액션 | `MSG_REACT/_NOTIFY` *(Out-of-Scope)* | — | — | — | OoS |
| FR-M06 | 이모티콘 변환 | 서버 측 `convert_emoticons()` | — | HLP §3 | `arch/module_common.md` utils | P3 |
| FR-M07 | 시스템 메시지 | `ROOM_MSG_RECV` msg_type=1 | `messages.txt` | CUI §39, NTF §2-16 | `module_message.md` | P0~ |
| FR-M08 | 검색 | `MSG_SEARCH/RES` (pkt §5) | `messages.txt` | CUI §32 | `module_message.md` | P3 |
| FR-M09 | 타임스탬프 | 클라이언트 `format_timestamp()` | — | CUI §31 §4 | `arch/module_common.md` utils | P2 |
| FR-M10 | 핀 메시지 | `MSG_PIN`, `MSG_PIN_NOTIFY`, `ROOM_PIN` (pkt §4-5) | `rooms.txt` pinned_msg_id | CUI §18, DLG §17 | `module_room.md` | P3 |
| FR-M11 | /me 액션 | `ROOM_MSG_RECV` msg_type=3 | `messages.txt` msg_type=3 | CUI §16 | `module_message.md` | P3 |

### FR-N: 알림

| FR | 항목 | 패킷 | 데이터 | UI | 모듈 | Phase |
|----|------|------|--------|-----|------|-------|
| FR-N01 | 메시지 알림 | `ROOM_MSG_RECV` 비현재방 | — | NTF §2-1 | `feat/FR_N_notification.md` | P2 |
| FR-N02 | 친구 요청 알림 | `FRIEND_REQUEST_NOTIFY`, `NOTIFY|FRIEND_REQ` | `friends.txt` pending | NTF §2-2 | `module_friend.md` | P1 |
| FR-N03 | 멘션 알림 | `NOTIFY|MENTION` | — | NTF §2-5 | `module_message.md` | P3 |
| FR-N04 | DND 모드 | `SETTINGS_UPDATE` dnd 필드 | `user_settings.txt` dnd | DLG §20 | `module_message.md` filter | P3 |
| FR-N05 | 타이핑 표시 | `TYPING_START/STOP/NOTIFY` (pkt §8) | — | NTF §2-15 | `feat/FR_N_notification.md` | P3 |
| FR-N06 | 방 알림 무음 | `ROOM_MUTE_TOGGLE/RES` (pkt §6) | `room_members.txt` is_muted | DLG §19 | `feat/FR_N_notification.md` | P3 |

### FR-C: 커스터마이징

| FR | 항목 | 패킷 | 데이터 | UI | 모듈 | Phase |
|----|------|------|--------|-----|------|-------|
| FR-C01 | 메시지 색상 | `SETTINGS_UPDATE` msg_color | `user_settings.txt` | CUI §31 §1 | `user_store.c` | P2 |
| FR-C02 | 닉네임 색상 | `SETTINGS_UPDATE` nick_color | `user_settings.txt` | CUI §31 §2 | `user_store.c` | P2 |
| FR-C03 | 테마 | `SETTINGS_UPDATE` theme | `user_settings.txt` | CUI §31 §3 | `user_store.c` | P2 |
| FR-C04 | 타임스탬프 형식 | `SETTINGS_UPDATE` ts_format | `user_settings.txt` | CUI §31 §4 | `user_store.c` | P2 |
| FR-C05 | 상태메시지 | `PROFILE_UPDATE` status_msg | `users.txt` status_msg | CUI §28 | `user_store.c` | P2 |
| FR-C06 | 온라인 상태 | `STATUS_CHANGE` (pkt §6) | `users.txt` online_status | CUI §31 §6 | `user_store.c` | P2 |
| FR-C07 | 오픈채팅 닉네임 | `ROOM_SET_OPEN_NICK/RES` | `room_members.txt` open_nick | CUI §15 | `feat/FR_C_customization.md` | P2 |

### FR-P: 마이페이지

| FR | 항목 | 패킷 | 데이터 | UI | 모듈 | Phase |
|----|------|------|--------|-----|------|-------|
| FR-P01 | 프로필 조회 | `MYPAGE_REQ/RES` (pkt §7) | `users.txt` | CUI §27 | `user_store.c` | P1 |
| FR-P02 | 활동 통계 | `MYPAGE_RES` (msg/room/friend count) | `messages.txt`, `rooms.txt`, `friends.txt` 집계 | CUI §27 | `user_store.c` | P1 |
| FR-P03 | 참여 채팅방 | `MY_ROOMS_REQ/RES` (pkt §7) | `room_members.txt` | CUI §30 | `module_room.md` | P1 |
| FR-P04 | 최근 DM | `DM_LIST_REQ/RES` (pkt §3) | `messages.txt` + `dm_reads.txt` | CUI §24 | `module_dm.md` | P1 |
| FR-P05 | 프로필 수정 | → FR-A04 | — | CUI §28 | — | P1 |
| FR-P06 | 비밀번호 변경 | → FR-A05 | — | CUI §29 | — | P1 |

### Out-of-Scope

| FR | 항목 | 사유 |
|----|------|------|
| FR-M05 | 리액션 | v2.1 이후 별도 구현. `MSG_REACT/_NOTIFY` 패킷 정의는 주석으로 보존 |
| FR-ADM01~05 | 관리자 기능 | requirements.md §3-10 명시. `ADMIN_CMD` 주석으로 보존 |

---

## 2. 비기능 요구사항 (NFR)

| NFR | 항목 | docs 위치 |
|-----|------|-----------|
| NFR-01 | 동시 접속 256명 | `nfr_checklist.md` NFR-01, `MAX_CLIENTS=256` (in_memory_structures.md §1) |
| NFR-02 | 응답 지연 50ms | `nfr_checklist.md` NFR-02 |
| NFR-03 | 안정성 (비정상 종료) | `nfr_checklist.md` NFR-03, `error_and_recovery.md` §1·§2, `server_architecture.md` §7 leftClient |
| NFR-04 | 보안 (SHA-256·gets 금지·금지문자) | `password_hashing.md`, `input_validation.md`, `nfr_checklist.md` NFR-04 |
| NFR-05 | 경량성 100MB | `nfr_checklist.md` NFR-05 메모리 산정 |
| NFR-06 | 이식성 (Windows 단일) | `nfr_checklist.md` NFR-06 (GTK4 의존성 제거 명시) |
| NFR-07 | 확장성 (서버/클라 분리) | `server_architecture.md`, `client_architecture.md` |
| NFR-08 | 영속성 | `file_schema.md`, `nfr_checklist.md` NFR-08 |
| NFR-09 | 스레드 안전 | `in_memory_structures.md` §7, `notification_patterns.md` §7, `nfr_checklist.md` NFR-09 |

---

## 3. 패킷 정의 (requirements.md §4-2)

| 카테고리 | requirements.md 패킷 | docs 위치 (`packet_reference.md`) |
|----------|----------------------|-------------------------------------|
| 인증 | LOGIN/REGISTER/LOGOUT (REQ/RES) | §1 |
| 친구 | FRIEND_ADD/ACCEPT/REJECT/DELETE/BLOCK/LIST/STATUS_CHANGE, USER_SEARCH | §2 |
| DM | DM_SEND/RECV/READ_NOTIFY/HISTORY/LIST | §3 |
| 그룹 채팅방 | ROOM_CREATE/LIST/SEARCH/JOIN/MSG/HISTORY/LEAVE/INVITE/KICK/DELETE/SET_NOTICE/GRANT_ADMIN/REVOKE_ADMIN/MEMBERS/NOTICE/PIN | §4 |
| 메시지 부가 | WHISPER/MSG_DELETE/EDIT/REPLY/SEARCH/PIN | §5 |
| 설정 | SETTINGS/STATUS_CHANGE/PROFILE_UPDATE/PASS_CHANGE/ROOM_SET_OPEN_NICK/ROOM_MUTE_TOGGLE | §6 |
| 마이페이지 | MYPAGE/MY_ROOMS | §7 |
| 타이핑 | TYPING_START/STOP/NOTIFY | §8 |
| 알림 | NOTIFY (MENTION/FRIEND_REQ/SERVER/DM) | §9 |
| 연결 유지 | PING/PONG | §10 |
| 관리자 | ADMIN_CMD *(Out-of-Scope)* | 미수록 |
| 리액션 | MSG_REACT/_NOTIFY *(Out-of-Scope)* | §5 주석 |

---

## 4. Phase 우선순위 (requirements.md §11)

| Phase | requirements.md 범위 | docs 위치 |
|-------|----------------------|-----------|
| P0 | FR-A01~A03, FR-G01/G03/G05, FR-O01/O02/O04, ROOM_JOIN | `development_phases.md` Phase 0 체크리스트 |
| P1 | FR-A03(LOGOUT_REQ)·A04~A06, FR-F01~F07, FR-D01~D05, FR-G02·G09, FR-P01~P06, ALREADY_ONLINE | Phase 1 |
| P2 | FR-M01~M03,M07,M09, FR-G06~G08·G10, FR-C01~C07, FR-N01·N02, PING/PONG | Phase 2 |
| P3 | FR-M04,M06,M08,M10,M11, FR-O03, FR-N03~N06 | Phase 3 |
| OoS | FR-M05, FR-ADM01~05 | requirements.md 명시. docs는 정의 보존 (구현 없음) |

---

## 5. requirements.md → docs 변경 결정 (Mapping)

| 영역 | requirements.md | docs 결정 | 사유·문서 |
|------|-----------------|-----------|-----------|
| UI 레이어 | GTK4 GUI (`screen_*.c/h`, css) | 콘솔 UI (`menu_*.c/h`) | `overview/project_summary.md` |
| 영속 저장 | MySQL (DDL 8 tables) | 텍스트 파일 9개 | `database/file_schema.md` |
| `ClientSession.fd` | `int` | `SOCKET` (WinSock2) | `server_architecture.md` 헤더 메모 |
| `ClientSession.MYSQL *db` | MySQL 연결 | **삭제** | 동일 |
| `server/db.c/h` | MySQL 래퍼 | `server/file_io.c/h` + `user_store.c/h` | `project_layout.md` |
| `server/admin.c/h` | 관리자 명령 | **삭제** (Out-of-Scope) | 동일 |
| `client/app_window.c/h`, `notify.c/h` | GTK4 위젯 | **삭제** | 동일 |
| `common/net_win.c` | WinSock2 추상화 | `client/net.c`로 흡수 | 동일 |
| `sql/schema.sql` | DDL 스크립트 | **삭제** | `build_guide.md` `make init` 대체 |
| `data/users.txt` (Phase 0 노트) | 임시 대체 | **정식 채택** | `database/file_schema.md` |
| 그룹 채팅 안읽음 카운트 | (미정의) | `room_reads.txt` 추가 | `database/file_schema.md` §9 |

---

## 6. docs 전용 정책 (requirements.md 미명시 사항)

requirements.md에 없으나 docs에서 결정한 정책. 향후 requirements 갱신 시 반영 필요.

| 항목 | 정책 | docs 위치 |
|------|------|-----------|
| 비밀번호 3회 실패 시 | 초기 화면 자동 복귀 | `dialogs_and_actions.md` §23 |
| 회원가입 후 자동 로그인 | 옵션 (Y/N 선택) | `dialogs_and_actions.md` §22 |
| 재연결 후 자동 로그인 | `auto_login()` 호출 | `error_and_recovery.md` §2 |
| 안읽음 카운트 표시 상한 | `[안읽음 99+]` | `screen_inventory.md` CUI §40 |
| 다중 초대 | `ROOM_INVITE` 반복 전송 | `feat/FR_G_group_chat.md` FR-G02 |
| 차단된 유저 방 초대 | `ROOM_INVITE_RES|1` (NOT_FOUND 위장) | `feat/FR_F_friend.md` 차단 매트릭스 |
| 차단 해제 정책 | `FRIEND_DELETE`로 통일 (재요청 필요) | 동일 |
| 메시지 삭제 권한 | 본인 + 방장/공동방장 | `feat/FR_M_message.md` FR-M02 권한 표 |
| 메시지 수정 권한 | 본인만 (방장도 타인 수정 불가) | `feat/FR_M_message.md` FR-M03 권한 표 |
| 시스템 메시지 수정 | 누구도 불가 | 동일 |
| `welcome_shown` 플래그 | `user_settings.txt` 신규 필드 | `database/file_schema.md` §8 |
| `MAX_MSG_HISTORY=1000` | 인메모리 메시지 캐시 한도 | `database/in_memory_structures.md` §1 |
| 슬래시 명령어 추가 | `/select`, `/status`, `/pinlist`, `/mute`, `/open_nick` | `uiux/menu_flow.md` §2~5 |

---

## 7. 검증 결과 — 전체 합치성

| 영역 | 항목 수 | 추적성 확보 |
|------|---------|-------------|
| FR (P0~P3) | 56개 | ✅ 모두 추적 가능 |
| FR (Out-of-Scope) | 6개 | ✅ docs에서 OoS 명시 |
| NFR | 9개 | ✅ 모두 nfr_checklist.md 매핑 |
| 패킷 (활성) | 65종 | ✅ packet_reference.md 등록 |
| 패킷 (Out-of-Scope) | 3종 | ✅ 주석 형태로 보존 |
| Phase 우선순위 | P0~P3 + OoS | ✅ development_phases.md 매핑 |
| requirements 변경 결정 | 11항목 | ✅ project_layout.md 변경표 |
| docs 전용 정책 | 13항목 | ✅ 본 문서 §6 정리 |

**잔여 작업**: 없음. 본 매트릭스를 정기 갱신용 진실의 근거(source-of-truth)로 사용.
