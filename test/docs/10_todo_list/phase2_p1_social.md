# Phase 2 — P1 Social

> **전제**: Phase 1 (P0 Core) 완료 후 시작.  
> 이 Phase가 끝나면 친구 관리, DM, 마이페이지, 유저 프로필 기능이 동작한다.

---

### [P1-01] 서버 계정 확장 (프로필 수정·비밀번호 변경)

- **구현 파일**: `chat_program/src/server/auth.c/h` (P0-01 확장)
- **참조 문서**:
  - `docs/02_features/account.md` — FR-A04(프로필 수정), FR-A05(비밀번호 변경), FR-A06(마지막 접속 표시)
  - `docs/08_api/packets/auth.md` — `PROFILE_EDIT`/`PROFILE_EDIT_RES`, `PASSWORD_CHANGE`/`PASSWORD_CHANGE_RES` 패킷
  - `docs/07_database/query_catalog.md` — `§인증` 섹션
- **수용 기준**:
  - `PROFILE_EDIT|nickname|profile_msg\n` → DB UPDATE → 온라인 친구에게 변경 알림
  - `PASSWORD_CHANGE|old_pw|new_pw\n` → 기존 PW 검증 → bcrypt 재해시 → DB UPDATE
  - `last_login_at` 타임스탬프: 로그인 성공 시 DB UPDATE
  - FR-A06: 마지막 접속 시각 포맷 → `utils.c`의 `time_ago()` 활용

---

### [P1-02] 서버 친구 모듈

- **구현 파일**: `chat_program/src/server/friend.c/h`
- **참조 문서**:
  - `docs/02_features/friend.md` — FR-F01(요청 전송), FR-F02(수락), FR-F03(거절), FR-F04(삭제), FR-F05(차단), FR-F06(차단 해제), FR-F07(친구 목록)
  - `docs/08_api/packets/friend.md` — `FRIEND_REQUEST`, `FRIEND_ACCEPT`, `FRIEND_REJECT`, `FRIEND_DELETE`, `FRIEND_BLOCK`, `FRIEND_UNBLOCK`, `FRIEND_LIST` 패킷
  - `docs/07_database/tables/friends.md` — `status`: 0=pending, 1=accepted, 2=blocked
  - `docs/07_database/query_catalog.md` — `§친구` 섹션
- **수용 기준**:
  - 모든 FRIEND_* 패킷 처리 → DB CRUD + 상대방 온라인 시 실시간 알림
  - `FRIEND_LIST\n` → `status=1` 친구 목록 + 온라인 상태 응답
  - 차단(status=2) 시 메시지·방 초대 차단
  - `FriendEntry` 뮤텍스 보호

---

### [P1-03] 서버 DM 모듈

- **구현 파일**: `chat_program/src/server/dm.c/h`
- **참조 문서**:
  - `docs/02_features/dm.md` — FR-D01(DM 전송), FR-D02(DM 목록), FR-D03(읽음 처리), FR-D04(오프라인 전달), FR-D05(DM 이력)
  - `docs/08_api/packets/dm.md` — `DM_SEND`, `DM_LIST`, `DM_READ`, `DM_HISTORY` 패킷
  - `docs/07_database/tables/messages.md` — `room_id IS NULL` = DM
  - `docs/07_database/tables/dm_reads.md` — 읽음 추적
  - `docs/07_database/query_catalog.md` — `§메시지` 섹션
- **수용 기준**:
  - `DM_SEND|to_id|content\n` → `messages` INSERT (`room_id=NULL`) → 상대 온라인 시 즉시 전달, 오프라인 시 DB 저장
  - `DM_LIST\n` → 최근 DM 상대 목록 + 미읽 수
  - `DM_READ|from_id\n` → `dm_reads` UPSERT → 상대에게 읽음 확인 전송
  - `DM_HISTORY|with_id|offset\n` → 최근 50개 메시지 페이지네이션

---

### [P1-04] 서버 마이페이지 핸들러

- **구현 파일**: `chat_program/src/server/auth.c/h` 또는 별도 `chat_program/src/server/mypage.c/h`
- **참조 문서**:
  - `docs/02_features/mypage.md` — FR-P01(프로필 조회), FR-P02(상태 메시지), FR-P03(아바타 색상), FR-P04(온라인 상태 설정), FR-P05(가입 방 목록), FR-P06(통계)
  - `docs/08_api/packets/mypage.md` — `MY_INFO`, `MY_INFO_RES`, `MY_ROOMS`, `MY_ROOMS_RES` 패킷
  - `docs/07_database/query_catalog.md` — `§마이페이지` 섹션
- **수용 기준**:
  - `MY_INFO\n` → 내 프로필 전체 응답 (nickname, profile_msg, online_status, last_login_at)
  - `MY_ROOMS\n` → 참여 중인 방 목록 (일반 + 오픈) 응답
  - `invisible` 상태: 서버가 `online_status=0`으로 마스킹 (DB에 저장 안 함)

---

### [P1-05] 클라이언트 친구 뷰 + 마이페이지 화면

- **구현 파일**:
  - `chat_program/src/client/friend_view.c/h`
  - `chat_program/src/client/screen_mypage.c/h`
- **참조 문서**:
  - `docs/06_ui_ux/screens/main.md` — 친구 탭 레이아웃
  - `docs/06_ui_ux/screens/mypage.md` — 마이페이지 화면 명세
  - `docs/06_ui_ux/components.md` — `FriendItem`, `Avatar`, `StatusDot`
  - `docs/04_file_structure/client_modules.md`
- **수용 기준**:
  - `friend_view.c/h`: 친구 목록 `GtkListBox`, 검색 `GtkSearchEntry`, 친구 요청/수락/삭제 액션
  - 온라인 친구 `StatusDot` (초록), 오프라인 (회색) 표시
  - `screen_mypage.c/h`: 프로필 표시, 닉네임·상태 메시지 인라인 편집, 테마 토글 버튼

---

### [P1-06] 유저 프로필 팝오버 + 서버 USER_VIEW 핸들러

- **구현 파일**:
  - `chat_program/src/server/auth.c/h` (또는 `user_store.c/h`) 확장 — `USER_VIEW` 패킷 처리
  - `chat_program/src/client/user_profile_popover.c/h` (신규)
- **참조 문서**:
  - `docs/06_ui_ux/screens/user_profile.md` — 신규 화면 명세, `USER_VIEW`/`USER_VIEW_RES` 패킷 정의, 관계별 버튼 조건표
  - `docs/06_ui_ux/components.md` — `FriendItem`, `Avatar` 재사용
  - `docs/06_ui_ux/screen_flow.md` — 유저 클릭 이벤트 흐름
- **수용 기준**:
  - 서버: `USER_VIEW|target_id\n` 수신 → 유저 정보 조회 → `USER_VIEW_RES|id|nickname|profile_msg|status\n`
  - 클라이언트: 친구 항목·메시지 아바타 클릭 → `GtkPopover` 표시
  - 팝오버 버튼: 관계에 따라 조건부 표시 (낯선이/친구요청중/친구/차단)
    - 낯선이: "친구 추가" 버튼
    - 친구: "DM 보내기", "친구 삭제" 버튼
    - 차단: "차단 해제" 버튼
