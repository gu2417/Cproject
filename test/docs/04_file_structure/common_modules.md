# 공통 모듈 (chat_program/src/common/)

| 파일 | 역할 | 주요 심볼 |
|------|------|-----------|
| `protocol.h` | 패킷 TYPE 상수, 구분자, 코드 | `P_LOGIN_REQ`, `P_LOGIN_RES`, ..., `FIELD_SEP='|'`, `MULTI_SEP=':'`, `LIST_SEP=';'`, `PKT_MAX=2048` |
| `types.h` | 클라이언트·서버 공용 구조체 | `UserSettings`, `FriendEntry`, `RoomSummary`, `UiEvent`(클라전용이나 여기에) |
| `utils.c/h` | 문자열·시간·이모티콘 변환 | `strlcpy_safe`, `fmt_timestamp`, `emoticon_expand`, `split_fields` |
| `net_compat.h` | 플랫폼 소켓 차이 흡수 | `net_startup`, `net_cleanup`, `net_close_sock`, 타입 `sock_t` |
| `net_posix.c` | POSIX 구현 | 위 API |
| `net_win.c` | Winsock2 구현 | 위 API |

## `protocol.h` 설계 원칙

- **문자열 상수** 로 TYPE 정의 (파싱 단순). 예: `#define P_LOGIN_REQ "LOGIN_REQ"`.
- enum 으로 응답 코드 정의. 예:
  ```c
  enum LoginCode { LC_OK=0, LC_WRONG_ID=1, LC_WRONG_PW=2, LC_ALREADY_ONLINE=3 };
  ```
- 구분자는 `#define`. `\n` 은 `PKT_TERM`.
- `PKT_MAX=2048` 은 송수신 양쪽에서 동일 상수 사용.

## `utils` 필수 함수

| 함수 | 용도 |
|------|------|
| `int split_fields(char *buf, char sep, char **out, int max)` | `\0` 치환으로 in-place 파싱. 반환: 실제 토큰 수 |
| `void fmt_timestamp(time_t t, int fmt, char *out, size_t n)` | `ts_format` 에 따라 `HH:MM` 등 변환 |
| `size_t emoticon_expand(const char *in, char *out, size_t n)` | `:smile:`→`(^_^)` 등 치환 |
| `void strlcpy_safe(char *dst, const char *src, size_t n)` | 항상 `\0` 종료 |
| `int validate_id(const char *s)` | 길이 + 허용 문자(영숫자, `_`) |
| `int validate_nick(const char *s)` | 길이만(한글 허용) |
