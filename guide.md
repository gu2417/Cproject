# 빌드 및 실행 가이드

## 디렉토리 구조

```
Cproject/                 ← 프로젝트 루트 (모든 명령은 여기서 실행)
├── Makefile
├── guide.md
├── data/                 ← make init 으로 생성 (서버 데이터 저장소)
├── server.exe            ← 빌드 후 생성
├── client.exe            ← 빌드 후 생성
└── chat_program/
    └── src/
        ├── server/
        ├── client/
        └── common/
```

**모든 `make` 명령과 실행 파일은 `Cproject/` (루트) 에서 수행한다.**

---

## 요구사항

| 항목 | 내용 |
|------|------|
| OS | Windows 10 이상 |
| 컴파일러 | MinGW-w64 (gcc) |
| Make | GNU Make (MinGW 포함) |
| 표준 | C11 |

설치 확인:

```cmd
gcc --version
make --version
```

---

## 빌드

`Cproject\` 루트에서 실행한다.

```cmd
cd Cproject

:: 서버 + 클라이언트 모두 빌드
make

:: 서버만 빌드
make server

:: 클라이언트만 빌드
make client
```

빌드 성공 시 `Cproject\server.exe`, `Cproject\client.exe`가 생성된다.

---

## 초기 데이터 파일 생성 (최초 1회)

`Cproject\` 루트에서 실행한다.

```cmd
make init
```

`Cproject\data\` 폴더 아래 9개의 빈 txt 파일이 생성된다.

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

> `make init`은 Windows CMD 명령(`type nul >`)을 사용하므로 **cmd.exe** 또는 **PowerShell**에서 실행한다. Git Bash에서는 `mkdir -p data && touch data/users.txt data/rooms.txt data/messages.txt data/friends.txt data/room_members.txt data/dm_reads.txt data/room_invites.txt data/user_settings.txt data/room_reads.txt`로 대체한다.

---

## 실행

터미널을 두 개 열고 **모두 `Cproject\` 루트**에서 실행한다.

**터미널 1 — 서버 먼저 실행:**

```cmd
cd Cproject
server.exe
```

**터미널 2 — 클라이언트 (여러 개 동시 실행 가능):**

```cmd
cd Cproject
client.exe
```

기본 접속: `127.0.0.1:55555`

---

## 정리

`Cproject\` 루트에서 실행한다.

```cmd
:: 빌드 결과물 삭제 (data\ 파일은 유지)
make clean
```

---

## 전체 순서 요약

```cmd
cd Cproject        (1) 루트로 이동
make init          (2) 데이터 파일 초기화 (최초 1회)
make               (3) 빌드
server.exe         (4) 서버 실행 (터미널 1)
client.exe         (5) 클라이언트 실행 (터미널 2)
```

---

## 주의사항

- 서버를 먼저 실행한 뒤 클라이언트를 실행한다.
- `data\` 폴더가 없거나 비어 있으면 서버가 레코드를 저장하지 못한다. `make init`을 먼저 수행한다.
- 방화벽에서 TCP 55555 포트를 허용해야 한다.
- 재빌드 시 `make clean` 후 `make` 순서로 실행한다.
- `chat_program\` 하위로 들어가서 명령을 실행하면 경로 오류가 발생한다. 반드시 루트에서 실행한다.
