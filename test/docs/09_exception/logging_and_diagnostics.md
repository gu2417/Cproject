# 로깅 · 진단

## 1. 로그 레벨

| 레벨 | 용도 |
|------|------|
| `ERROR` | 세션/요청 실패를 초래한 예외 |
| `WARN` | 회복 가능한 이상(재시도, 드롭된 프레임) |
| `INFO` | 로그인/로그아웃/방 생성 등 주요 비즈니스 이벤트 |
| `DEBUG` | 개발 전용. 배포 시 `-DNDEBUG` 로 컴파일아웃 |

## 2. 포맷

```
YYYY-MM-DD HH:MM:SS [LEVEL] [tid=1234] [sid=alice fd=7] message
```
- 단일 라인, 파이프·탭 사용 금지(grep 친화).

## 3. 출력 대상

- 기본: `stderr`.
- 운영에서는 `./chat_server 2>> server.log` 로 append.
- 자체 파일 롤링은 out-of-scope (운영에서 `logrotate`).

## 4. 저사양 고려

- `DEBUG` 는 `NDEBUG` 로 빌드 타임 제거 (NFR-05 메모리 예산 보호).
- `INFO` 도 옵션(`-v`). 기본은 `WARN` 이상.
- 로그 포맷은 `snprintf` 한 번 + 단일 `write` — stdio 버퍼링 우회(스레드 인터리빙 방지).

## 5. 민감 정보

- 비밀번호·비밀번호 해시·DM content 는 **절대** 로그에 남기지 않음.
- 유저 아이디·방 id 는 남겨도 됨.

## 6. 진단 도구

- `ADMIN_STATUS` 패킷으로 런타임 현황 조회(uptime, 접속자, 방/메시지 수).
- 추가 진단(접속자 IP 목록 등) 은 v2.1.
