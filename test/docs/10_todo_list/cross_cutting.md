# Cross-Cutting — 보안·예외 처리·로깅

> 이 항목들은 특정 Phase에 속하지 않고 **전 단계에 걸쳐 지속적으로 적용**한다.  
> Phase 0 완료 직후부터 각 기능 구현과 동시에 반영해야 한다.

---

### [CC-01] 입력 검증 + SQL Injection 방지

- **적용 위치**: 모든 패킷 핸들러 (서버), 모든 사용자 입력 (클라이언트)
- **구현 파일**: 서버의 모든 `*.c/h` 핸들러 파일
- **참조 문서**:
  - `docs/05_security/input_validation.md` — 필드별 최대 길이, 허용 문자 규칙
  - `docs/05_security/injection_prevention.md` — Prepared Statement 규칙
  - `docs/07_database/query_catalog.md` — 모든 쿼리는 `?` 플레이스홀더 사용
- **수용 기준**:
  - 패킷 수신 직후 필드 길이 및 형식 검증 (공백·특수문자·NULL 바이트 차단)
  - DB 쿼리는 100% Prepared Statement — 문자열 직접 연결(`sprintf`) 금지
  - 클라이언트: `GtkEntry` 최대 길이 `gtk_entry_set_max_length()` 설정
  - 검증 실패 시 연결 유지하되 오류 응답 반환 (연결 강제 종료 금지)

---

### [CC-02] 권한 매트릭스 적용

- **적용 위치**: 방 관련 핸들러 (`room.c/h`), 메시지 핸들러 (`message.c/h`)
- **참조 문서**:
  - `docs/05_security/authorization.md` — 역할(방장/공동방장/일반/차단)별 허용 작업 매트릭스
  - `docs/02_features/group_room.md` — FR-G06, FR-G07 방장 권한 규칙
- **수용 기준**:
  - 방장만: `ROOM_KICK`, `ROOM_PROMOTE`, `ROOM_NOTICE`, `ROOM_EDIT`, `ROOM_DELETE`, `MSG_PIN`
  - 방장/공동방장: 공지 등록 가능
  - 일반 멤버: 메시지 전송, 본인 메시지 수정/삭제만 가능
  - 차단된 유저: 메시지 수신 불가, 방 초대 불가
  - 권한 없는 요청 → `ERR|403|forbidden\n` 응답

---

### [CC-03] 서버·클라이언트 에러 처리 + DB 장애 모드 + 로깅

- **적용 위치**: 전체 서버·클라이언트 코드
- **참조 문서**:
  - `docs/09_exception/server_error_handling.md` — 서버 에러 코드 체계, 세션 청소 규칙
  - `docs/09_exception/client_error_handling.md` — 재연결 정책, UI 에러 표시
  - `docs/09_exception/db_failure_modes.md` — DB 연결 실패 시 graceful degradation
  - `docs/09_exception/edge_cases.md` — 동시성 충돌, 빈 입력, 긴 메시지 등 엣지 케이스
  - `docs/09_exception/logging_and_diagnostics.md` — 로그 레벨·포맷 규칙
- **수용 기준**:
  - 서버: 클라이언트 연결 끊김 → `clients[]` 즉시 정리, room `member_fds[]` 갱신
  - 서버: `pthread` 핸들러 내 unhandled 에러 → `LOG_ERROR` 기록 후 해당 스레드만 종료
  - DB 연결 실패: 서버 기동 시 3회 재시도, 실패 시 `exit(1)` — 런타임 중 실패는 오류 응답
  - 클라이언트: 연결 끊김 → `GtkDialog` 알림 + 5초 후 자동 재연결 (최대 3회)
  - 로그 포맷: `[LEVEL] YYYY-MM-DD HH:MM:SS [thread_id] message`
  - 로그 레벨: `DEBUG`, `INFO`, `WARN`, `ERROR` (빌드 플래그로 `DEBUG` 제어)
