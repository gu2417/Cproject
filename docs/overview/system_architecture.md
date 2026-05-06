# 시스템 아키텍처

## 1. 전체 구조 다이어그램

```
+----------------------------------------------------------+
|                        SERVER                            |
|                                                          |
|  +------------+    +---------------+   +--------------+ |
|  | Accept     |    | HandleClient  |   | Broadcast    | |
|  | Thread     |--->| Thread        |-->| Manager      | |
|  | (main)     |    | (per client)  |   | (broadcast   | |
|  +------------+    +---------------+   |  + notify)   | |
|                           |            +--------------+ |
|                           v                    |        |
|  +------------------------------------------------+     |
|  |           In-Memory Session Store              |     |
|  |   ClientSession g_sessions[MAX_CLIENTS]        |     |
|  |   RoomInfo      g_rooms[MAX_ROOMS]             |     |
|  |   (접속 중인 유저 + 활성 채팅방만 유지)        |     |
|  +------------------------------------------------+     |
|                           |                             |
|                    mutex 보호                           |
|                           |                             |
|  +------------------------------------------------+     |
|  |              File I/O Layer                    |     |
|  |   users.txt | rooms.txt | messages.txt         |     |
|  |   friends.txt | room_members.txt               |     |
|  |   dm_reads.txt | room_invites.txt              |     |
|  |   user_settings.txt                            |     |
|  +------------------------------------------------+     |
+----------------------------------------------------------+
               ^  TCP Socket (port 55555)  v
+----------------------------------------------------------+
|                        CLIENT                            |
|                                                          |
|  +-----------------+      +---------------------------+ |
|  | 메뉴 루프       |      | RecvMsg Thread            | |
|  | (메인 스레드)   |      | (소켓 수신 -> 콘솔 출력)  | |
|  | InitialMenu()   |      |                           | |
|  | ShowMainMenu()  |      | recv() -> 비동기 출력     | |
|  +-----------------+      +---------------------------+ |
|           |                           |                 |
|           v                           v                 |
|  +-------------------------------------------------+    |
|  |            콘솔 터미널 (stdout/stdin)           |    |
|  |  번호 메뉴 선택 | 슬래시 명령어 | 메시지 입력   |    |
|  +-------------------------------------------------+    |
+----------------------------------------------------------+
```

## 2. 서버 컴포넌트

### 2-1. Accept Thread (main 스레드)

- `WSAStartup` → 소켓 생성 → `bind` → `listen` → `accept` 루프
- 클라이언트 접속 시 `g_sessions[]`에 등록
- `_beginthreadex`로 `HandleClient` 스레드 생성
- 서버 시작 시 모든 txt 파일 로드 (`file_io.c`)

### 2-2. HandleClient Thread (클라이언트당 1개)

- `recv()` 루프로 패킷 수신
- `MsgChecker()` → `router.c`로 패킷 타입 분류
- `recv()` 반환값 0 또는 음수: `leftClient()` 호출 후 스레드 종료
- 스레드 핸들: `ClientSession.hThread`

### 2-3. Router (router.c)

- 패킷 타입 문자열로 핸들러 함수 분기
- `strtok(str, "|")` 또는 직접 파싱으로 TYPE 추출
- 각 핸들러 모듈로 위임: `auth.c`, `friend.c`, `room.c`, `dm.c`, `message.c`

### 2-4. Broadcast Manager (broadcast.c)

- `broadcast_to_room()`: 특정 방 멤버에게만 전송
- `broadcast_to_all()`: 전체 접속 클라이언트에게 전송 (P0)
- `send_to_user()`: 특정 user_id에게 전송
- 모든 전송 함수는 `g_sessions_mutex` 보호 하에 실행

### 2-5. File I/O Layer (file_io.c)

- 서버 시작 시 전체 txt 파일을 인메모리 배열에 로드
- 변경 발생 시 파일에 즉시 반영
- 구분자 `//` 사용 (reference server 준수)
- `fopen()` NULL 체크 필수 (파일 없으면 빈 배열로 시작)

## 3. 클라이언트 컴포넌트

### 3-1. 메인 스레드 (메뉴 루프)

- `WSAStartup` → 소켓 생성 → `connect`
- `InitialMenu()`: 로그인 / 회원가입 / 종료
- `ShowMainMenu()`: 친구, 채팅방, 오픈채팅, DM, 마이페이지, 설정
- 채팅방 입장 후: 메시지 입력 루프 + 슬래시 명령어 처리

### 3-2. RecvMsg Thread

- `_beginthreadex`로 생성
- `recv()` 루프로 서버 패킷 수신
- `packet_parse()`로 TYPE 분류 후 콘솔에 비동기 출력
- 수신된 메시지: 현재 보는 방 메시지만 표시, 나머지는 알림으로 출력

### 3-3. SendMsg (메인 스레드에서 호출)

- 사용자 입력 수집 후 `packet_build()`로 패킷 구성
- `send()`로 서버에 전송

## 4. 스레드 모델

```
main() [Accept Loop]
  |
  +-- _beginthreadex --> HandleClient[0]  (client 0)
  |                          |
  |                          +-- recv() loop
  |                          +-- router --> auth/room/friend/dm/message
  |
  +-- _beginthreadex --> HandleClient[1]  (client 1)
  |
  ...
  +-- _beginthreadex --> HandleClient[N]  (client N)
```

## 5. Mutex 보호 범위

| 공유 자원 | Mutex | 접근 패턴 |
|-----------|-------|-----------|
| `g_sessions[]` | `g_sessions_mutex` | 모든 HandleClient 스레드 + Accept 스레드 |
| `g_rooms[]` | `g_sessions_mutex` (동일) | 방 생성/삭제/참여/퇴장 |
| txt 파일 쓰기 | `g_file_mutex` | file_io.c의 모든 쓰기 연산 |

```c
// Mutex 사용 패턴
WaitForSingleObject(g_sessions_mutex, INFINITE);
// --- 임계 구역 ---
g_sessions[i].active = 1;
// --- 임계 구역 끝 ---
ReleaseMutex(g_sessions_mutex);
```

## 6. 데이터 흐름

```
[클라이언트 입력]
    |
    v
packet_build() --> send() --> [TCP 소켓] --> recv() [서버]
                                                  |
                                                  v
                                          MsgChecker() / router()
                                                  |
                                     +------------+------------+
                                     |            |            |
                                auth.c        room.c       friend.c ...
                                     |            |            |
                                     +------------+------------+
                                                  |
                                          broadcast / send_to_user
                                                  |
                                                  v
                                    recv() [클라이언트 RecvMsg Thread]
                                                  |
                                          packet_parse()
                                                  |
                                          콘솔 출력 (stdout)
```
