# AGENTS.md — C Chat Application

이 파일은 모든 AI 에이전트(Copilot, Claude, Gemini 등)에게 프로젝트 맥락과 작업 규칙을 전달합니다.

## 프로젝트 한 줄 요약

**C11 + GTK4 GUI 실시간 채팅 앱** — KakaoTalk/Google Chat 스타일, TCP Server-Client, thread-per-client, MySQL 영속 저장.

## 핵심 아키텍처 규칙 (반드시 준수)

### 1. 파일 구조
```
chat_program/src/
├── common/      # 양측 공유: protocol.h, types.h, utils.c/h, net_compat.h
├── server/      # accept 루프, thread-per-client, MySQL, 기능 핸들러
└── client/      # GTK4 GUI (GtkStack 화면 전환), send/recv 스레드
```

### 2. 스레드 모델
- **서버**: main 스레드 → accept → `pthread_create(client_handler)` per client
- **클라이언트**: GTK main 루프 + send 스레드 + recv 스레드
- **GTK4 규칙**: recv 스레드에서 GTK widget API 직접 호출 금지 → 반드시 `g_idle_add()` 사용
- **Mutex 규칙**: `g_sessions_mutex` 보유 중 `send()` / DB 호출 금지 (데드락)

### 3. 패킷 프로토콜
- 형식: `TYPE|field1|field2\n`
- 최대 크기: 2048 bytes
- 구분자: `|` (필드), `:` (복합값), `;` (목록), `\n` (종료)
- 모든 수신 패킷은 필드 수·길이·NULL을 검증한 뒤 처리
- 패킷 타입 상수: `chat_program/src/common/protocol.h`

### 4. DB / 보안
- **모든 SQL은 Prepared Statement만** — `db_prepare()` + `db_bind_str/int()` 패턴
- raw `mysql_query()` 또는 sprintf SQL 조합 절대 금지
- 비밀번호는 단방향 해시만 저장 (plaintext 금지)
- 사용자 입력은 길이·형식 검증 후 DB에 전달

### 5. 빌드
```bash
make           # 전체 빌드 (server + client)
make server    # 서버만
make client    # 클라이언트만
make clean     # 정리
```

### 6. 코드 품질
- C11 표준 (`-std=c11`)
- `-Wall -Wextra -Wshadow` 경고 0개 유지
- 모든 `malloc`/`calloc` 반환값 NULL 검사
- 포맷: `.clang-format` (Allman, indent 4, limit 100)
- 정적 분석: `.cppcheck-suppress` 기준 cppcheck 통과

## 구현 우선순위

| Phase | 목표 | 핵심 파일 |
|-------|------|-----------|
| P0 | 소켓 연결·인증·그룹 채팅 | common/, server/main.c, server/auth.c, server/globals.c |
| P1 | DM·친구 관리·읽음 확인 | server/friend.c, server/dm.c, client/screen_chat.c |
| P2 | 귓속말·메시지 삭제·방 공지 | server/message.c, server/room.c |
| P3 | 검색·이모티콘·관리자·DND | server/admin.c, client/screen_settings.c |

Phase 0 TODO: `docs/10_todo_list/phase0_foundation.md` 참고.

## 설계 문서 위치

| 주제 | 경로 |
|------|------|
| 기능 요구사항 | `docs/02_features/` |
| 아키텍처/스레딩 | `docs/03_architecture/` |
| 파일 구조/함수 | `docs/04_file_structure/` |
| UI/UX 화면 흐름 | `docs/06_ui_ux/` |
| DB 스키마/쿼리 | `docs/07_database/` |
| 패킷 명세 | `docs/08_api/` |
| Phase TODO | `docs/10_todo_list/` |

## 에이전트 작업 가이드라인

- 코드 생성 전 반드시 관련 설계 문서(`docs/`) 확인
- 새 파일 작성 시 `chat_program/src/common/protocol.h`, `types.h` 의존성 먼저 점검
- DB 접근 코드는 `server/db.c` 래퍼 함수만 사용
- 함수 한 개가 100줄 초과하면 분리 고려
- 메모리 누수: 모든 동적 할당에 대응하는 `free()` 명시
