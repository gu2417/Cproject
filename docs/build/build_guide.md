# 빌드 가이드

## 1. 빌드 환경

| 항목 | 요구사항 |
|------|---------|
| OS | Windows 10 이상 |
| 컴파일러 | MinGW-w64 (gcc) 또는 MSVC |
| 의존성 | WinSock2 (`ws2_32.lib`), Windows API (`advapi32.lib`) |
| C 표준 | C11 (`-std=c11`) |

### MinGW-w64 설치 확인

```
gcc --version
> gcc (MinGW-w64) 13.x.x
```

---

## 2. 디렉토리 사전 준비

서버 실행 전 `data/` 폴더와 빈 txt 파일을 생성해야 한다.

```bat
REM Windows CMD
mkdir data
type nul > data\users.txt
type nul > data\rooms.txt
type nul > data\messages.txt
type nul > data\friends.txt
type nul > data\room_members.txt
type nul > data\dm_reads.txt
type nul > data\room_invites.txt
type nul > data\user_settings.txt
```

> **주의**: 빈 파일이라도 존재해야 `fopen()` NULL 체크가 안전하게 동작한다.  
> 파일이 없어도 서버는 정상 동작하지만(NULL 체크로 빈 배열 시작), `data/` 폴더는 반드시 존재해야 새 레코드를 append할 수 있다.

---

## 3. 서버 빌드

### MinGW gcc

```bash
gcc -std=c11 -Wall -Wextra -o server.exe \
    src/server/main.c \
    src/server/globals.c \
    src/server/client_handler.c \
    src/server/router.c \
    src/server/file_io.c \
    src/server/auth.c \
    src/server/user_store.c \
    src/server/friend.c \
    src/server/room.c \
    src/server/dm.c \
    src/server/message.c \
    src/server/broadcast.c \
    src/common/utils.c \
    -lws2_32 -ladvapi32
```

### MSVC (Developer Command Prompt)

```bat
cl /std:c11 /W3 /Fe:server.exe ^
    src\server\main.c ^
    src\server\globals.c ^
    src\server\client_handler.c ^
    src\server\router.c ^
    src\server\file_io.c ^
    src\server\auth.c ^
    src\server\user_store.c ^
    src\server\friend.c ^
    src\server\room.c ^
    src\server\dm.c ^
    src\server\message.c ^
    src\server\broadcast.c ^
    src\common\utils.c ^
    ws2_32.lib advapi32.lib
```

---

## 4. 클라이언트 빌드

### MinGW gcc

```bash
gcc -std=c11 -Wall -Wextra -o client.exe \
    src/client/main.c \
    src/client/state.c \
    src/client/net.c \
    src/client/packet.c \
    src/client/menu_initial.c \
    src/client/menu_main.c \
    src/client/menu_chat.c \
    src/client/menu_friend.c \
    src/client/menu_dm.c \
    src/client/menu_mypage.c \
    src/client/menu_settings.c \
    src/common/utils.c \
    -lws2_32
```

### MSVC

```bat
cl /std:c11 /W3 /Fe:client.exe ^
    src\client\main.c ^
    src\client\state.c ^
    src\client\net.c ^
    src\client\packet.c ^
    src\client\menu_initial.c ^
    src\client\menu_main.c ^
    src\client\menu_chat.c ^
    src\client\menu_friend.c ^
    src\client\menu_dm.c ^
    src\client\menu_mypage.c ^
    src\client\menu_settings.c ^
    src\common\utils.c ^
    ws2_32.lib
```

---

## 5. Makefile (server / client 타겟 분리)

```makefile
CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Isrc/server -Isrc/client -Isrc/common
SRVLIBS = -lws2_32 -ladvapi32
CLIBS   = -lws2_32

SRV_SRCS = src/server/main.c \
           src/server/globals.c \
           src/server/client_handler.c \
           src/server/router.c \
           src/server/file_io.c \
           src/server/auth.c \
           src/server/user_store.c \
           src/server/friend.c \
           src/server/room.c \
           src/server/dm.c \
           src/server/message.c \
           src/server/broadcast.c \
           src/common/utils.c

CLI_SRCS = src/client/main.c \
           src/client/state.c \
           src/client/net.c \
           src/client/packet.c \
           src/client/menu_initial.c \
           src/client/menu_main.c \
           src/client/menu_chat.c \
           src/client/menu_friend.c \
           src/client/menu_dm.c \
           src/client/menu_mypage.c \
           src/client/menu_settings.c \
           src/common/utils.c

.PHONY: all server client clean init

all: server client

server: $(SRV_SRCS)
	$(CC) $(CFLAGS) -o server.exe $(SRV_SRCS) $(SRVLIBS)
	@echo "Server build complete: server.exe"

client: $(CLI_SRCS)
	$(CC) $(CFLAGS) -o client.exe $(CLI_SRCS) $(CLIBS)
	@echo "Client build complete: client.exe"

init:
	@if not exist data mkdir data
	@type nul > data\users.txt
	@type nul > data\rooms.txt
	@type nul > data\messages.txt
	@type nul > data\friends.txt
	@type nul > data\room_members.txt
	@type nul > data\dm_reads.txt
	@type nul > data\room_invites.txt
	@type nul > data\user_settings.txt
	@echo "data/ directory initialized."

clean:
	del /Q server.exe client.exe 2>nul
	@echo "Cleaned."
```

---

## 6. 실행 방법

### 1단계: 데이터 디렉토리 초기화 (최초 1회)

```bat
make init
REM 또는 수동으로 data\ 폴더 생성
```

### 2단계: 서버 실행

```bat
server.exe
```

출력:
```
Server listening on port 55555...
```

### 3단계: 클라이언트 실행 (별도 터미널)

```bat
client.exe
```

여러 클라이언트를 테스트하려면 터미널을 여러 개 열어 각각 `client.exe`를 실행한다.

---

## 7. 포트 방화벽 설정

Windows 방화벽에서 포트 55555를 허용해야 다른 PC에서 접속 가능하다.

```bat
netsh advfirewall firewall add rule ^
    name="ChatServer" ^
    dir=in action=allow ^
    protocol=TCP localport=55555
```

같은 PC에서 테스트(`127.0.0.1`)하는 경우는 방화벽 설정 불필요.

---

## 8. 빌드 문제 해결

| 오류 | 원인 | 해결 |
|------|------|------|
| `undefined reference to WSAStartup` | ws2_32 링크 누락 | `-lws2_32` 추가 |
| `undefined reference to CryptCreateHash` | advapi32 링크 누락 | `-ladvapi32` 추가 |
| `warning: implicit declaration of 'getch'` | conio.h 미포함 | `#include <conio.h>` 추가 |
| `error: 'SOCKET' undeclared` | winsock2.h 미포함 | `#include <winsock2.h>` 추가 |
| `data/ 파일 쓰기 실패` | data 폴더 없음 | `make init` 실행 |
