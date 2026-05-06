# 서버 아키텍처

## 1. 서버 파일 구조

```
src/server/
├── main.c              # 진입점: WSAStartup, bind, listen, accept 루프, 파일 로드
├── config.h            # 포트, MAX_CLIENTS, MAX_ROOMS, 파일 경로 상수
├── globals.c           # g_sessions[], g_rooms[], g_sessions_mutex 정의
├── globals.h           # globals.c 선언 헤더
├── client_handler.c    # HandleClient 스레드 (per-client)
├── client_handler.h
├── router.c            # 패킷 TYPE → 핸들러 함수 라우팅 (MsgChecker)
├── router.h
├── file_io.c           # txt 파일 읽기/쓰기 헬퍼 (MySQL 대체)
├── file_io.h
├── auth.c              # 회원가입, 로그인 처리
├── auth.h
├── user_store.c        # 유저·설정 CRUD (users.txt, user_settings.txt)
├── user_store.h
├── friend.c            # 친구 요청/수락/거절/삭제/차단
├── friend.h
├── room.c              # 채팅방 생성/참여/퇴장/관리 (rooms.txt, room_members.txt)
├── room.h
├── dm.c                # 1:1 DM 처리 (messages.txt, dm_reads.txt)
├── dm.h
├── message.c           # 메시지 저장·삭제·수정·검색·핀 (messages.txt)
├── message.h
└── broadcast.c         # 브로드캐스트, 알림 전송
    broadcast.h
```

## 2. 모듈별 책임

| 모듈 | 파일 | 주요 책임 |
|------|------|-----------|
| main | main.c | 서버 소켓 초기화, accept 루프, 파일 데이터 로드, 스레드 생성 |
| client_handler | client_handler.c/h | per-client recv 루프, 패킷 수신 후 router 호출, leftClient 처리 |
| router | router.c/h | 수신 패킷의 TYPE 분류 후 적절한 핸들러 함수로 위임 |
| file_io | file_io.c/h | txt 파일 파싱/저장 헬퍼, NULL 체크, mutex 보호 하 쓰기 |
| auth | auth.c/h | 로그인 검증(SHA256 비교), 회원가입(중복 ID 체크, users.txt 저장) |
| user_store | user_store.c/h | 유저 정보 조회/수정, 설정 조회/수정, 마지막 접속 시간 갱신 |
| friend | friend.c/h | 친구 요청 전송/수락/거절/삭제/차단, 친구 목록 조회, 유저 검색 |
| room | room.c/h | 방 생성/입장/퇴장/초대/강퇴/삭제, 공지·핀 설정, 멤버 목록 |
| dm | dm.c/h | DM 전송, DM 히스토리 조회, 읽음 처리 |
| message | message.c/h | 메시지 저장·삭제·수정(5분 이내)·답장·검색·핀·이모티콘 변환 |
| broadcast | broadcast.c/h | 방 브로드캐스트, 전체 브로드캐스트, 특정 유저 전송, 상태 알림 |

## 3. 스레드 모델

```
main() [메인 스레드 — Accept Loop]
  |
  | accept() → clientSock
  |
  +-- _beginthreadex --> HandleClient(clientSock)  [스레드 #0]
  |                           |
  |                           +-- recv() loop
  |                           |      |
  |                           |      v
  |                           |  router() [MsgChecker]
  |                           |      |
  |                           |      +-- handle_login()
  |                           |      +-- handle_register()
  |                           |      +-- handle_room_create()
  |                           |      +-- handle_room_msg()
  |                           |      +-- handle_friend_add()
  |                           |      +-- ... (기타 핸들러)
  |                           |
  |                           +-- recv() == 0 또는 음수 → leftClient()
  |
  +-- _beginthreadex --> HandleClient(clientSock)  [스레드 #1]
  |
  ...
  +-- _beginthreadex --> HandleClient(clientSock)  [스레드 #255]
```

## 4. main.c 흐름

```c
int main(int argc, char *argv[]) {
    // 1. Mutex 초기화
    g_sessions_mutex = CreateMutex(NULL, FALSE, NULL);
    g_file_mutex     = CreateMutex(NULL, FALSE, NULL);

    // 2. 파일에서 데이터 로드 (파일 없으면 빈 배열로 시작)
    g_user_count   = load_users(FILE_USERS, g_users, MAX_CLIENTS);
    g_room_count   = load_rooms(FILE_ROOMS, g_rooms_data, MAX_ROOMS);
    // ... 기타 파일 로드

    // 3. WinSock 초기화
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // 4. 서버 소켓 생성 및 바인딩
    serverSock = socket(PF_INET, SOCK_STREAM, 0);
    serverAddr.sin_family      = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port        = htons(DEFAULT_PORT);
    bind(serverSock, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    listen(serverSock, 10);

    printf("Server listening on port %d...\n", DEFAULT_PORT);

    // 5. Accept 루프
    while (1) {
        clientSock = accept(serverSock, (SOCKADDR*)&clientAddr, &addrSize);

        WaitForSingleObject(g_sessions_mutex, INFINITE);
        // 빈 슬롯에 소켓 등록
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!g_sessions[i].active) {
                g_sessions[i].fd     = clientSock;
                g_sessions[i].active = 1;
                g_client_count++;
                hThread = (HANDLE)_beginthreadex(
                    NULL, 0, HandleClient,
                    (void*)&g_sessions[i], 0, NULL);
                g_sessions[i].hThread = hThread;
                break;
            }
        }
        ReleaseMutex(g_sessions_mutex);

        printf("Connected: %s\n", inet_ntoa(clientAddr.sin_addr));
    }

    closesocket(serverSock);
    WSACleanup();
    return 0;
}
```

## 5. Mutex 보호 범위

| 공유 자원 | Mutex | 위험 연산 |
|-----------|-------|-----------|
| `g_sessions[]` | `g_sessions_mutex` | active 읽기/쓰기, fd 접근, 세션 정보 갱신 |
| `g_rooms[]` (인메모리) | `g_sessions_mutex` | 방 생성/삭제, 멤버 추가/삭제 |
| `g_client_count` | `g_sessions_mutex` | 카운터 증감 |
| txt 파일 쓰기 | `g_file_mutex` | fopen, fprintf, fclose |

## 6. leftClient 처리 흐름

```c
void leftClient(ClientSession *sess) {
    printf("Client disconnected: %s\n", sess->user_id);

    // 1. 참여 중이던 방에서 퇴장 처리
    if (sess->current_room_id > 0) {
        handle_room_leave_internal(sess->current_room_id, sess->user_id);
    }

    // 2. 온라인 상태 → offline 갱신 (users.txt)
    update_user_online_status(sess->user_id, 0);
    update_user_last_seen(sess->user_id);

    // 3. 친구들에게 오프라인 상태 변경 알림
    notify_friend_status_change(sess->user_id, 0);

    // 4. 세션 슬롯 해제
    WaitForSingleObject(g_sessions_mutex, INFINITE);
    closesocket(sess->fd);
    memset(sess, 0, sizeof(ClientSession));
    sess->fd = INVALID_SOCKET;
    g_client_count--;
    ReleaseMutex(g_sessions_mutex);
}
```
