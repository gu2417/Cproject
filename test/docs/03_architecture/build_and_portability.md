# 빌드 및 이식성

## 1. Makefile 구조

최상위 `Makefile` 하나로 두 플랫폼을 처리. 플랫폼 감지는 `uname -s` (Windows 는 MSYS/MinGW 쉘 기준).

```make
UNAME := $(shell uname -s 2>/dev/null || echo Windows)

CC       ?= gcc
CSTD     := -std=c11 -Wall -Wextra -Wpedantic
OPT      := -O2
INCLUDES := -Ichat_program/src/common
SRV_SRC  := $(wildcard chat_program/src/server/*.c) $(wildcard chat_program/src/common/*.c)
CLI_SRC  := $(wildcard chat_program/src/client/*.c) $(wildcard chat_program/src/common/*.c)

ifeq ($(UNAME),Linux)
    LDFLAGS_SRV := -lpthread -lmysqlclient
    LDFLAGS_CLI := -lpthread $(shell pkg-config --cflags --libs gtk4)
endif
ifneq (,$(findstring MINGW,$(UNAME))$(findstring Windows,$(UNAME)))
    LDFLAGS_SRV := -lpthread -lmysql -lws2_32
    LDFLAGS_CLI := -lpthread -lws2_32 $(shell pkg-config --cflags --libs gtk4)
    EXE         := .exe
endif

all: chat_program/src/server/chat_server$(EXE) chat_program/src/client/chat_client$(EXE)
```

## 2. 플랫폼 분기 규칙

분기는 다음 파일로 **국한**한다:

| 위치 | 목적 |
|------|------|
| `chat_program/src/common/net_compat.h` + `net_posix.c` / `net_win.c` | `socket`/`close`/`WSAStartup` 차이 |

GTK4가 플랫폼 간 UI 추상화를 담당하므로 별도 console/TUI 추상화 레이어 불필요.
기능 코드(auth/room/message 등)에는 `#ifdef` 가 없어야 한다.

## 3. 외부 라이브러리

| OS | 설치 명령 |
|----|-----------|
| Ubuntu/Debian | `apt install build-essential libmysqlclient-dev libgtk-4-dev` |
| Windows (MSYS2) | `pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-mysql-connector-c mingw-w64-x86_64-gtk4` |

## 4. 산출물

| 타깃 | 경로 |
|------|------|
| 서버 | `chat_program/src/server/chat_server` (Win: `.exe`) |
| 클라이언트 | `chat_program/src/client/chat_client` (Win: `.exe`) |

## 5. 디버그 빌드

```bash
make DEBUG=1    # -O0 -g -fsanitize=address,undefined
```

Windows(MinGW)는 ASan 미지원 — `DEBUG=1` 시 `-O0 -g` 만 적용.

## 6. 컴파일러 경고 정책

`-Wall -Wextra -Wpedantic` 는 모든 환경에서 **경고 0개** 유지가 목표.
`-Werror` 는 CI(P4) 도입 시 활성화.
