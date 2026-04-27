# 타깃 플랫폼 · 지원 매트릭스

## 1. 요약

`requirements.md` NFR-06 에 따라 **Linux 와 Windows(MSYS2/MinGW-w64)** 두 플랫폼을
지원한다. 클라이언트 GUI 는 GTK4 기반이며, 동일 소스 트리로 양 환경에서 빌드·실행
가능해야 한다.

## 2. 지원 매트릭스

| 구분 | Linux (glibc) | Windows (MSYS2/MinGW-w64) |
|------|---------------|---------------------------|
| 서버 | ✅ 지원 | ✅ 지원 |
| 클라이언트 | ✅ 지원 | ✅ 지원 |
| 컴파일러 | gcc 9+ / clang 10+ | mingw-w64 gcc 8+ |
| C 표준 | C11 | C11 |
| 스레드 | `pthread` | `pthread-win32` (MinGW 번들) |
| 소켓 | `<sys/socket.h>` | `<winsock2.h>` + `ws2_32` 링크 |
| Sleep | `nanosleep` | `Sleep` |
| 파일 경로 | `/` | `\` (설계상 경로 하드코딩 금지) |
| MySQL 클라이언트 | `libmysqlclient-dev` | MySQL Connector/C (win) |

### GTK4 빌드 환경

| 항목 | Linux | Windows (MSYS2) |
|------|-------|-----------------|
| GTK4 패키지 | `libgtk-4-dev` | `mingw-w64-x86_64-gtk4` |
| pkg-config | `gtk4` | `gtk4` |
| 최소 GTK4 버전 | 4.6 이상 | 4.6 이상 |

## 3. 저사양 기준선

**최소 사양**(클라이언트 실행 기준):

| 항목 | 최소값 |
|------|--------|
| CPU | 듀얼코어 1.6GHz |
| RAM | 2GB (OS 포함) — 클라이언트 자체는 ≤20MB 상주 |
| 최소 창 크기 | 600×400px |
| 네트워크 | 1Mbps |

서버는 NFR-05 에 따라 MySQL 제외 **100MB 이하** 상주.

## 4. 이식성 원칙

1. 플랫폼 분기는 **전부 `chat_program/src/common/` 또는 `chat_program/src/client/platform.h` 에 집중**한다.
   기능 코드(auth, room, message)에는 `#ifdef _WIN32` 를 쓰지 않는다.
2. 경로 구분자·파일 I/O 는 추상 헬퍼를 통해서만 접근.
3. 빌드는 단일 `Makefile` 과 선택적 `Makefile.mingw` 로 관리.
4. 바이트 오더는 전송 시 항상 network byte order 가정. 패킷은 텍스트이므로 문제 되지 않음.

## 5. 확인된 제약

- Windows 에서 GTK4 는 MSYS2 환경의 `mingw-w64-x86_64-gtk4` 패키지를 사용.
  `pkg-config --libs gtk4` 로 링크 플래그를 획득한다.
