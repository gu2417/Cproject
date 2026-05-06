# 프로젝트 파일 구조

GTK4/MySQL 제거, 콘솔 UI + 텍스트 파일 I/O 반영한 최종 파일 구조.

```
C_ChatProgram/
├── chat_program/
│   └── src/
│       ├── server/
│       │   ├── main.c              # 서버 진입점, accept 루프, 파일 로드, mutex 초기화
│       │   ├── config.h            # 포트(55555), MAX_CLIENTS(256), MAX_ROOMS(100), 파일 경로 상수
│       │   ├── globals.c           # g_sessions[], g_rooms[], g_sessions_mutex, g_file_mutex 정의
│       │   ├── globals.h           # globals.c 외부 선언 헤더
│       │   ├── client_handler.c    # HandleClient 스레드 — per-client recv 루프
│       │   ├── client_handler.h
│       │   ├── router.c            # MsgChecker — 패킷 TYPE별 핸들러 함수 라우팅
│       │   ├── router.h
│       │   ├── file_io.c           # txt 파일 읽기/쓰기 헬퍼 (MySQL 대체)
│       │   ├── file_io.h           # load_*/save_*/append_* 함수 선언
│       │   ├── auth.c              # 회원가입, 로그인, SHA-256 해시, 금지문자 검사
│       │   ├── auth.h
│       │   ├── user_store.c        # 유저·설정 CRUD (users.txt, user_settings.txt)
│       │   ├── user_store.h
│       │   ├── friend.c            # 친구 요청/수락/거절/삭제/차단, 목록, 유저 검색
│       │   ├── friend.h
│       │   ├── room.c              # 채팅방 생성/참여/퇴장/초대/강퇴/삭제/공지/핀/관리자
│       │   ├── room.h
│       │   ├── dm.c                # 1:1 DM 전송, 히스토리, 읽음 처리
│       │   ├── dm.h
│       │   ├── message.c           # 메시지 저장·삭제·수정·답장·검색·이모티콘·시스템메시지
│       │   ├── message.h
│       │   ├── broadcast.c         # broadcast_to_room, broadcast_to_all, send_to_user, 상태알림
│       │   └── broadcast.h
│       │
│       ├── client/
│       │   ├── main.c              # 클라이언트 진입점, WSAStartup, connect, RecvMsg 스레드 시작
│       │   ├── state.c             # ClientState 전역 구조체 정의
│       │   ├── state.h             # g_state 선언 (sock, user_id, nickname, 현재방, 설정)
│       │   ├── net.c               # connect_to_server, send_packet, RecvMsg 스레드
│       │   ├── net.h
│       │   ├── packet.c            # packet_build, packet_parse (직렬화/역직렬화)
│       │   ├── packet.h
│       │   ├── menu_initial.c      # InitialMenu — 로그인/회원가입/종료
│       │   ├── menu_initial.h
│       │   ├── menu_main.c         # ShowMainMenu — 메인 로비
│       │   ├── menu_main.h
│       │   ├── menu_chat.c         # ShowChatRoom — 채팅방 내부, 슬래시 명령어 처리
│       │   ├── menu_chat.h
│       │   ├── menu_friend.c       # ShowFriendMenu — 친구 목록/추가/수락/검색
│       │   ├── menu_friend.h
│       │   ├── menu_dm.c           # ShowDMMenu — DM 목록, DM 대화 화면
│       │   ├── menu_dm.h
│       │   ├── menu_mypage.c       # ShowMyPageMenu — 프로필/통계/참여방/비번변경
│       │   ├── menu_mypage.h
│       │   ├── menu_settings.c     # ShowSettingsMenu — 색상/테마/타임스탬프/DND/상태
│       │   └── menu_settings.h
│       │
│       └── common/
│           ├── protocol.h          # 패킷 타입 문자열 상수, 구분자, 응답 코드 #define
│           ├── types.h             # 공통 구조체 (UserRecord, RoomInfo, MessageRecord 등)
│           ├── utils.c             # get_current_timestamp, format_timestamp, parse_timestamp
│           └── utils.h             # 타임스탬프, 이모티콘 변환, 문자열 유틸 선언
│
├── data/                           # 런타임 데이터 파일 (서버 실행 디렉토리)
│   ├── users.txt                   # 유저 정보 (id//pw_hash//nickname//...)
│   ├── rooms.txt                   # 채팅방 정보 (id//name//topic//...)
│   ├── messages.txt                # 메시지 (id//room_id//from_id//...//content)
│   ├── friends.txt                 # 친구 관계 (id//user_id//friend_id//status//...)
│   ├── room_members.txt            # 방 멤버 (room_id//user_id//open_nick//is_admin//...)
│   ├── dm_reads.txt                # DM 읽음 기록 (msg_id//reader_id//read_at)
│   ├── room_invites.txt            # 방 초대 기록 (id//room_id//inviter_id//invitee_id//...)
│   └── user_settings.txt           # 유저 설정 (user_id//msg_color//nick_color//theme//...)
│
├── docs/                           # 설계 문서 (현재 폴더)
│   ├── README.md
│   ├── 00_overview/
│   ├── 01_protocol/
│   ├── 02_data/
│   ├── 03_server/
│   ├── 04_client/
│   ├── 05_features/
│   ├── 06_file_structure/
│   ├── 07_build/
│   ├── 08_nonfunctional/
│   └── 09_development/
│
├── reference/                      # 원본 참조 구현
│   ├── server_main.c               # 참조용 서버 구현 (WinSock2, users.txt)
│   └── client_main.c               # 참조용 클라이언트 구현 (콘솔 메뉴, getch)
│
├── Makefile                        # server / client 빌드 타겟 분리
└── requirements.md                 # 기능 요구사항 명세
```

---

## 파일별 역할 요약

### server/

| 파일 | 역할 |
|------|------|
| `main.c` | WSAStartup, CreateMutex, 파일 로드, accept 루프, _beginthreadex |
| `config.h` | 포트·상수·파일경로 #define 한곳 관리 |
| `globals.c/h` | 전역 배열(g_sessions, g_rooms)과 mutex 정의 |
| `client_handler.c/h` | 클라이언트별 스레드 — recv 루프, leftClient |
| `router.c/h` | 패킷 TYPE 파싱 후 핸들러 위임 (MsgChecker 역할) |
| `file_io.c/h` | txt 파일 로드/저장/append 헬퍼, fopen NULL 체크 |
| `auth.c/h` | 로그인·회원가입·SHA-256·금지문자 검사 |
| `user_store.c/h` | 유저 정보 조회·수정, 설정 upsert, last_seen 갱신 |
| `friend.c/h` | 친구 CRUD, 목록 조회, 유저 검색, 상태 알림 |
| `room.c/h` | 방 생성·입장·퇴장·초대·강퇴·삭제·공지·핀 |
| `dm.c/h` | DM 전송·수신·히스토리·읽음 처리 |
| `message.c/h` | 메시지 삭제·수정·답장·검색·이모티콘·시스템메시지 |
| `broadcast.c/h` | broadcast_to_room, broadcast_to_all, send_to_user |

### client/

| 파일 | 역할 |
|------|------|
| `main.c` | WSAStartup, connect, RecvMsg 스레드 시작, InitialMenu 호출 |
| `state.c/h` | ClientState g_state — 소켓·로그인정보·현재방·설정 |
| `net.c/h` | 소켓 연결, send_packet 헬퍼, RecvMsg 스레드 |
| `packet.c/h` | packet_parse — 수신 패킷 TYPE 분류 및 콘솔 출력 |
| `menu_initial.c/h` | 초기 화면: 로그인/회원가입/종료, getch 비밀번호 마스킹 |
| `menu_main.c/h` | 메인 로비: 서브메뉴 진입점 |
| `menu_chat.c/h` | 채팅방 내부: 메시지 입력, 슬래시 명령어 |
| `menu_friend.c/h` | 친구 관리 화면 |
| `menu_dm.c/h` | DM 목록 및 대화 화면 |
| `menu_mypage.c/h` | 마이페이지 화면 |
| `menu_settings.c/h` | 설정 화면 |

### common/

| 파일 | 역할 |
|------|------|
| `protocol.h` | `#define LOGIN_REQ "LOGIN_REQ"` 등 패킷 타입 상수 |
| `types.h` | UserRecord, RoomInfo, MessageRecord 등 공통 구조체 |
| `utils.c/h` | 타임스탬프 포맷, 이모티콘 변환, 문자열 유틸리티 |
