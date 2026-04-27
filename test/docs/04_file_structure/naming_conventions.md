# 네이밍 컨벤션

## 1. 파일

- 소문자 + 언더스코어: `client_handler.c`, `screen_mypage.h`.
- 쌍으로 관리: 각 `.c` 는 같은 이름의 `.h` 를 가진다(내부 전용은 예외).

## 2. 함수

| 스코프 | 규칙 | 예 |
|--------|------|----|
| 공개(헤더 노출) | `<module>_<action>` | `db_connect`, `room_add_member` |
| 핸들러 | `handle_<cmd>` | `handle_login`, `handle_room_msg` |
| 스크린 | `screen_<name>_<hook>` | `screen_chat_enter` |
| `static` 내부 함수 | 자유(하지만 snake_case) | `parse_kv` |

## 3. 타입

- 구조체: PascalCase + `_t` 금지(POSIX 예약). 예: `ClientSession`, `UiEvent`.
- enum 값: UPPER_SNAKE, 공용 prefix. 예: `LC_OK`, `RES_ROOM_FULL`.

## 4. 매크로 / 상수

- `#define` 매크로: UPPER_SNAKE. 예: `MAX_CLIENTS`, `PKT_MAX`.
- 함수형 매크로는 최대한 피하고 `static inline` 선호.

## 5. 전역

- 전역 변수는 `g_` 접두: `g_sessions`, `g_state`.
- 뮤텍스: `<target>_mutex` 형식. 예: `g_sessions_mutex`, `tx_mutex`.

## 6. 헤더 가드

```c
#ifndef CHAT_SERVER_ROOM_H
#define CHAT_SERVER_ROOM_H
/* ... */
#endif
```
경로 기반 prefix (`CHAT_SERVER_`, `CHAT_CLIENT_`, `CHAT_COMMON_`).

## 7. 주석

- 한국어 주석 허용. 공용 API 에는 간단한 목적·인자·반환 설명.
- 자명한 코드에 주석 금지(CLAUDE.md 규칙 준수).

## 8. 코드 포맷

- 들여쓰기: 스페이스 4칸.
- 한 줄 최대 100자.
- 중괄호는 K&R 스타일 (함수 정의는 다음 줄에 `{`).

## 9. 에러 반환 규약

- 성공 0, 실패 음수(`-1`, `-2`...). 의미는 헤더에 enum 으로 명시.
- 세션 종료가 필요한 치명 오류는 `-100` 이하 값으로 표현(라우터가 감지).
