# 빌드 및 실행 가이드 (Windows)

## 프로젝트 개요

C11 기반 Windows 전용 콘솔 채팅 프로그램.  
WinSock2 TCP 소켓 + 멀티스레드(`_beginthreadex`) + 텍스트 파일 영속 구조.

| 항목 | 내용 |
|------|------|
| 언어 | C11 |
| 플랫폼 | Windows 10 이상 (WinSock2, Win32 API 전용) |
| 기본 포트 | 55555 |
| 최대 동시 접속 | 256 클라이언트 |
| 데이터 저장 | `data/*.txt` 9개 텍스트 파일 |

---

## 디렉토리 구조

```
Cproject/                    ← 루트 Makefile 위치 (CMD 빌드 기준)
├── Makefile                 ← Windows CMD 전용 Makefile
├── guide.md
├── server.exe               ← 빌드 후 생성 (루트 기준 빌드 시)
├── client.exe               ← 빌드 후 생성 (루트 기준 빌드 시)
├── data/                    ← 서버 데이터 파일 저장소 (make init 생성)
│   ├── users.txt
│   ├── rooms.txt
│   ├── messages.txt
│   ├── friends.txt
│   ├── room_members.txt
│   ├── dm_reads.txt
│   ├── room_invites.txt
│   ├── user_settings.txt
│   └── room_reads.txt
└── chat_program/
    ├── Makefile             ← Git Bash / MSYS2 전용 Makefile
    └── src/
        ├── server/          ← 서버 소스 (12개 .c)
        ├── client/          ← 클라이언트 소스 (11개 .c)
        └── common/          ← 공통 소스 (utils.c, protocol.h, types.h)
```

> **실행 파일과 `data/` 폴더는 반드시 같은 위치에 있어야 한다.**  
> 서버가 `"data/users.txt"` 등 상대 경로로 파일을 읽기 때문이다.

---

## 1. 사전 요구사항

### MinGW-w64 설치 (MSYS2 권장)

1. [https://www.msys2.org](https://www.msys2.org) 에서 MSYS2 설치 프로그램을 다운로드하여 실행한다.
2. MSYS2 터미널을 열고 아래 명령어로 툴체인을 설치한다:

```bash
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make
```

3. 시스템 환경 변수 `PATH`에 MinGW bin 경로를 추가한다:
   - 예: `C:\msys64\ucrt64\bin`

4. 설치 확인:

```cmd
gcc --version
mingw32-make --version
```

> **`make` 명령어가 없는 경우:**  
> MSYS2 환경에서는 `make` 대신 `mingw32-make`를 사용하거나,  
> `C:\msys64\ucrt64\bin\mingw32-make.exe`를 `make.exe`로 복사한다.

---

## 2. 빌드

빌드 환경에 따라 두 가지 방법 중 하나를 선택한다.

---

### 방법 A — Windows CMD / PowerShell (루트 Makefile)

`Cproject\` 루트에서 실행. `server.exe`, `client.exe`가 루트에 생성된다.

```cmd
cd C:\Project\Cproject

:: 서버 + 클라이언트 동시 빌드
make all

:: 서버만 빌드
make server

:: 클라이언트만 빌드
make client
```

링크 플래그 (Makefile 자동 적용):

| 실행 파일 | 링크 라이브러리 |
|-----------|----------------|
| `server.exe` | `-lws2_32 -ladvapi32` (WinSock2 + WinCrypt SHA-256) |
| `client.exe` | `-lws2_32` (WinSock2) |

---

### 방법 B — Git Bash / MSYS2 Shell (chat_program 내부 Makefile)

`chat_program\` 디렉토리에서 실행. `server.exe`, `client.exe`가 `chat_program\` 에 생성된다.

```bash
cd /c/Project/Cproject/chat_program

# 서버 + 클라이언트 동시 빌드
make all

# 서버만
make server.exe

# 클라이언트만
make client.exe
```

> 방법 B를 사용할 경우, 이후 모든 실행 명령도 `chat_program\` 디렉토리에서 수행해야 한다.

---

## 3. 데이터 파일 초기화 (최초 1회)

서버는 시작 시 `data/` 폴더의 9개 파일을 읽는다.  
파일이 없으면 서버가 정상 시작되지 않으므로 **빌드 전 또는 첫 실행 전에** 반드시 수행한다.

### 방법 A (CMD — 루트 기준)

```cmd
cd C:\Project\Cproject
make init
```

생성되는 파일:

```
data\users.txt
data\rooms.txt
data\messages.txt
data\friends.txt
data\room_members.txt
data\dm_reads.txt
data\room_invites.txt
data\user_settings.txt
data\room_reads.txt
```

### 방법 B (Git Bash — chat_program 기준)

```bash
cd /c/Project/Cproject/chat_program
make data
```

---

## 4. 실행

터미널을 **두 개** 열고 순서대로 실행한다.

### 터미널 1 — 서버 먼저 실행

**방법 A (CMD, 루트):**
```cmd
cd C:\Project\Cproject
server.exe
```

**방법 B (Git Bash, chat_program):**
```bash
cd /c/Project/Cproject/chat_program
./server.exe
```

서버 정상 기동 출력:
```
[서버] 데이터 로드 중...
[서버] 로드 완료: 유저=0, 방=0, 친구=0, 초대=0
[서버] 포트 55555에서 대기 중...
```

### 터미널 2 (이상) — 클라이언트 실행

클라이언트는 동시에 여러 개 실행할 수 있다.

**방법 A (CMD, 루트):**
```cmd
cd C:\Project\Cproject
client.exe
```

**방법 B (Git Bash, chat_program):**
```bash
cd /c/Project/Cproject/chat_program
./client.exe
```

클라이언트는 자동으로 `127.0.0.1:55555`로 접속한다.

---

## 5. 빌드 결과물 삭제

```cmd
:: 방법 A (CMD)
make clean
```

```bash
# 방법 B (Git Bash)
make clean
```

`data\` 폴더와 텍스트 파일은 삭제되지 않는다.

---

## 6. 전체 순서 요약

```
[1] make init        데이터 파일 초기화 (최초 1회)
[2] make all         서버 + 클라이언트 빌드
[3] server.exe       터미널 1에서 서버 실행
[4] client.exe       터미널 2(+)에서 클라이언트 실행
```

재빌드 시:
```
make clean && make all
```

---

## 7. 주의사항

### 실행 환경
- 서버를 먼저 실행한 뒤 클라이언트를 실행한다. 순서가 바뀌면 클라이언트가 즉시 종료된다.
- `server.exe` / `client.exe`는 반드시 `data/` 폴더가 있는 디렉토리에서 실행한다.
- `chat_program\` 하위에서 루트 Makefile을 사용하거나 반대로 하면 경로 오류가 발생한다.

### 방화벽
- Windows 방화벽에서 TCP **55555** 포트를 허용해야 한다.
- 첫 실행 시 Windows 보안 경고가 뜨면 "액세스 허용"을 선택한다.

### 한글 출력
- CMD에서 한글이 깨질 경우 아래 명령을 먼저 실행한다:
  ```cmd
  chcp 65001
  ```
- Windows Terminal 사용 시 자동으로 UTF-8이 적용된다.

### WSL 환경
- WSL(Linux) 내부에서는 WinSock2 API를 사용할 수 없어 빌드 및 실행이 불가능하다.
- 반드시 **Windows 네이티브** CMD, PowerShell, 또는 MSYS2 Shell에서 실행한다.

---

## 8. 트러블슈팅

| 증상 | 원인 | 해결 |
|------|------|------|
| `gcc: command not found` | PATH 미설정 | MinGW bin 경로를 환경변수 PATH에 추가 |
| `make: command not found` | make 미설치 | `mingw32-make`로 대체하거나 make.exe 복사 |
| `[net] 서버 연결 실패` | 서버 미실행 또는 포트 충돌 | 서버를 먼저 실행, `netstat -ano \| findstr 55555`로 포트 확인 |
| `bind() 실패` | 55555 포트 이미 사용 중 | 기존 서버 프로세스 종료 후 재실행 |
| `data\users.txt: 열기 실패` | data/ 폴더 없음 | `make init` 실행 후 서버 재시작 |
| 빌드 후 한글 깨짐 | 콘솔 코드페이지 불일치 | `chcp 65001` 후 재실행 |
| `-ladvapi32` 링크 오류 | MinGW 구버전 | MSYS2 최신 ucrt64 툴체인으로 재설치 |
