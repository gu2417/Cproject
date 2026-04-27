# 클라이언트 에러 처리

## 1. 연결 끊김 감지

- recv thread 가 `recv()=0` 또는 오류 → `state = DISCONNECTED`.
- send thread 도 깨워 `write` 실패 시 조용히 종료.
- UI 는 상단에 빨간 배너 "서버 연결 끊김 — 재연결 중...".

## 2. 재연결 정책

- 지수 백오프: 1s → 2s → 4s → 8s (상한). 최대 5회.
- 실패 시 모달 "재연결 실패. 재시도 또는 종료를 선택해 주세요."
- 재연결 성공 시:
  - 기존 인증 토큰이 없으므로 **자동 재로그인 시도** — 저장된 비밀번호 없으므로 LOGIN 화면으로 복귀.
  - 이전 방/DM 컨텍스트는 메인화면에서 재선택.

## 3. 렌더 실패

- GTK4 CSS/cairo 렌더링 문제는 `G_MESSAGES_DEBUG=all` 또는 `GTK_DEBUG` 환경 변수로 진단.
- 창 최소 크기 미충족: `notify::default-width` / `notify::default-height` 핸들러에서 최소 600×400px 강제.
- GTK4 파일·리소스 로드 실패: `GError*` 체크 후 `g_warning()` 출력 및 사용자에게 오류 배너 표시.

## 4. 사용자 입력 에러

- GtkEntry 입력 유효성 검사 실패 → `gtk_entry_set_icon_from_icon_name()` 으로 오류 아이콘 표시.
- GtkRevealer 배너로 오류 메시지 표시 (2~3초 후 숨김).
- 서버가 `*_RES|<code>` 반환 → `08_api/error_codes.md` 매핑으로 한글 메시지 표시(2~3초 후 자동 소멸).

## 5. 전역 guard

- `SIGINT` (Ctrl-C) → `g_application_quit()` 호출(graceful 종료).
- `SIGTERM` → 동일.
- 소켓 close + `LOGOUT` 패킷 전송은 기존과 동일하게 수행.
- 필요 시 `atexit` 에 `g_object_unref` 등 GTK4 리소스 정리 등록.
