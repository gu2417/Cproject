# Requirements Specification
# C GTK4 GUI Chat Application — KakaoTalk/Google Chat Style

**Version**: 2.0.0  
**Date**: 2026-04-20  
**Language**: C (C11)  
**Architecture**: TCP Server-Client, Multi-threaded + MySQL 영속 저장

---

## 1. 프로젝트 개요

GTK4 GUI 환경에서 동작하는 실시간 채팅 어플리케이션.  
카카오톡의 1:1 채팅 / 오픈채팅방 구조와 Google Chat의 Space(채널) 개념을 GTK4 GUI로 구현한다.  
모든 데이터(유저, 메시지, 채팅방, 친구 관계)는 MySQL에 영속 저장하며, 서버 재시작 후에도 기록이 유지된다.

---

## 2. 시스템 아키텍처

```
┌──────────────────────────────────────────────────────────┐
│                        SERVER                            │
│  ┌────────────┐  ┌──────────────┐  ┌──────────────────┐ │
│  │  Accept    │  │  Router      │  │  Broadcast       │ │
│  │  Thread    │→ │  (per client │→ │  Manager         │ │
│  │            │  │   thread)    │  │  (active session │ │
│  └────────────┘  └──────────────┘  │   array + mutex) │ │
│                        │           └──────────────────┘ │
│  ┌─────────────────────▼──────────────────────────────┐ │
│  │              In-Memory Session Store               │ │
│  │  ClientSession[MAX_CLIENTS] — 접속 중인 유저만     │ │
│  └─────────────────────┬──────────────────────────────┘ │
│                        │ SQL                            │
│  ┌─────────────────────▼──────────────────────────────┐ │
│  │              MySQL  (chat_db)                      │ │
│  │  users | rooms | room_members | messages           │ │
│  │  friends | room_invites | dm_reads | user_settings │ │
│  └────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────┘
               ▲  TCP Socket  ▼
┌──────────────────────────────────────────────────────────┐
│                        CLIENT                            │
│  ┌──────────────┐         ┌──────────────────────────┐  │
│  │  GTK 이벤트  │         │  Receive Thread          │  │
│  │  루프        │         │  (socket → msg queue)    │  │
│  │              │         │                          │  │
│  └──────────────┘         └──────────────────────────┘  │
│  ┌──────────────────────────────────────────────────┐    │
│  │  GTK4 위젯 렌더러                                │    │
│  │  window: LOGIN | MAIN | CHAT | MYPAGE | SETTINGS  │    │
│  └──────────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────────┘
```

### 2-1. 채팅방 유형 구분

| 유형 | 설명 | 참여 방식 |
|------|------|-----------|
| **그룹 채팅방** | 초대 기반 닫힌 방. 카카오톡 단톡방 | 방장 또는 멤버가 친구 초대 |
| **오픈채팅방** | 누구나 목록에서 발견·참여 가능. 선택적 비밀번호 설정 가능 | 목록 조회 후 자유 참여 |
| **1:1 DM** | 친구 또는 ID 직접 입력으로 개설 | 자동 개설 |

---

## 3. 기능 요구사항

### 3-1. 계정 관리

| ID | 기능 | 설명 |
|----|------|------|
| FR-A01 | 회원가입 | ID(최대 20자), Password(최대 19자), 닉네임(최대 20자, 미입력 시 ID로 대체), 상태메시지(최대 100자, 선택 입력) 등록. 비밀번호 입력 필드는 `*`로 마스킹. ID·닉네임·비밀번호에 `:` · `;` · `\|` 문자 사용 불가 (프로토콜 구분자 충돌 방지) |
| FR-A02 | 로그인 | ID + Password 검증. 중복 로그인 차단. 비밀번호 입력 필드는 `*`로 마스킹 |
| FR-A03 | 로그아웃 | 접속 종료 (P0: 소켓 종료로 대체), last_seen 갱신·온라인 상태 해제·LOGOUT_REQ 패킷은 P1 |
| FR-A04 | 프로필 수정 | 닉네임, 상태메시지 변경 |
| FR-A05 | 비밀번호 변경 | 현재 PW 확인 후 새 PW 설정 |
| FR-A06 | 마지막 접속 시간 | 오프라인 유저 프로필에 `마지막 접속: N분 전` 표시 |

---

### 3-2. 친구 관리 (카카오톡 스타일)

| ID | 기능 | 설명 |
|----|------|------|
| FR-F01 | 친구 추가 | 상대방 ID 검색 후 친구 요청 전송 |
| FR-F02 | 친구 요청 수락/거절 | 수신된 요청 목록에서 처리 |
| FR-F03 | 친구 목록 조회 | 온라인/오프라인 상태 표시 포함 |
| FR-F04 | 친구 삭제 | 친구 목록에서 제거 |
| FR-F05 | 친구 차단 | 차단 시 메시지 수신 차단, 목록에서 숨김 |
| FR-F06 | 온라인 상태 표시 | `[ON]` / `[OFF]` / `[바쁨]` 3가지 표시. `invisible(3)` 설정자는 타인에게 `[OFF]`로 표시되며 서버는 `FRIEND_STATUS_CHANGE` 알림을 offline으로 전송 |
| FR-F07 | 유저 검색 | ID 또는 닉네임으로 전체 유저 검색 |

---

### 3-3. 1:1 채팅 (다이렉트 메시지)

| ID | 기능 | 설명 |
|----|------|------|
| FR-D01 | DM 시작 | 친구 목록에서 선택하거나 ID 직접 입력으로 DM 채팅방 개설 |
| FR-D02 | 메시지 전송 | 텍스트 메시지 전송 (최대 500자) |
| FR-D03 | 읽음 확인 | 상대방이 읽으면 `[읽음]` 표시 (카카오톡 스타일) |
| FR-D04 | 메시지 히스토리 | 입장 시 최근 50개 메시지 출력 (MySQL 조회) |
| FR-D05 | 안읽은 메시지 수 | 채팅 목록에 미읽 메시지 수 배지 표시 |

---

### 3-4. 그룹 채팅방

| ID | 기능 | 설명 |
|----|------|------|
| FR-G01 | 채팅방 생성 | 방 이름(최대 30자, `:` · `;` · `\|` 사용 불가), 주제(최대 100자), 최대 인원(기본 10명, 최대 64명), 비밀번호(최대 10자, `:` · `;` · `\|` 사용 불가) 선택 설정 |
| FR-G02 | 멤버 초대 | 방장 또는 멤버가 친구를 채팅방에 초대 |
| FR-G03 | 메시지 전송 | 입장한 방의 전체 참여자에게 브로드캐스트 |
| FR-G04 | 멘션(@) | `@닉네임` 으로 특정 사용자 언급, 해당 사용자에게 알림 |
| FR-G05 | 채팅방 나가기 | 퇴장 메시지 브로드캐스트 후 방에서 제거 |
| FR-G06 | 방장 권한 | 멤버 강퇴, 방 삭제, 공지 등록, 핀 메시지 설정 |
| FR-G07 | 공동 방장 | 방장이 특정 멤버에게 관리자 권한 부여/해제 |
| FR-G08 | 공지사항 등록/조회 | 공지 등록 시 입장할 때마다 상단 표시 |
| FR-G09 | 메시지 히스토리 | 입장 시 최근 100개 메시지 출력 (MySQL 조회) |
| FR-G10 | 멤버 목록 조회 | `/members` 로 현재 방 참여자 및 온라인 상태 확인 |

---

### 3-5. 오픈채팅방

| ID | 기능 | 설명 |
|----|------|------|
| FR-O01 | 오픈채팅방 생성 | 방 이름(최대 30자, `:` · `;` · `\|` 사용 불가), 주제(최대 100자), 최대 인원(기본 10명, 최대 64명), 비밀번호(최대 10자, `:` · `;` · `\|` 사용 불가, 선택) 설정. 누구나 발견 가능 |
| FR-O02 | 목록 조회 | 전체 오픈채팅방 목록 표시 (방이름, 현재인원/최대인원, 주제) |
| FR-O03 | 방 검색 | 방 이름 또는 주제 키워드로 검색 |
| FR-O04 | 자유 참여 | 목록에서 선택 후 참여. 비밀번호 방은 입력 후 참여 |
| FR-O05 | 오픈채팅 닉네임 | → FR-C07 참조 |

---

### 3-6. 메시지 기능

| ID | 기능 | 설명 |
|----|------|------|
| FR-M01 | 귓속말 | `/w <닉네임> <내용>` — 특정 사용자에게만 전달 |
| FR-M02 | 메시지 삭제 | `/del <msg_id>` — 내 메시지 삭제. 상대에겐 `삭제된 메시지` 표시 |
| FR-M03 | 메시지 수정 | `/edit <msg_id> <새내용>` — 전송 후 5분 이내 수정 가능. `(수정됨)` 표시 |
| FR-M04 | 답장(인용) | `/reply <msg_id> <내용>` — 특정 메시지 인용 후 답장. 인용 원문 축약 표시 |
| FR-M05 | 리액션 *(Out-of-Scope)* | ~~`/react <msg_id> <이모지>` — 메시지에 텍스트 이모지 반응 추가/취소. 집계 표시~~ |
| FR-M06 | 이모티콘 변환 | `:smile:` → `(^_^)`, `:heart:` → `<3` 등 텍스트 이모지 세트 |
| FR-M07 | 시스템 메시지 | 입/퇴장, 초대, 강퇴, 공지 등 이벤트를 구분된 색상으로 표시 |
| FR-M08 | 메시지 검색 | `/search <키워드>` — 현재 채팅방 내 메시지 전문 검색 |
| FR-M09 | 타임스탬프 | 모든 메시지에 `[HH:MM]` 형식 시간 표시 (설정에서 형식 변경 가능) |
| FR-M10 | 핀 메시지 | `/pin <msg_id>` — 방장이 방 상단에 중요 메시지 고정 |
| FR-M11 | /me 액션 | `/me <동작>` — 이탤릭 스타일의 액션 메시지 (예: `* 홍길동 손을 흔든다`) |

---

### 3-7. 알림

| ID | 기능 | 설명 |
|----|------|------|
| FR-N01 | 메시지 알림 | 현재 보고 있지 않은 채팅방/DM 메시지 수신 시 알림 팝업(GtkRevealer) 표시 |
| FR-N02 | 친구 요청 알림 | 로그인 시 및 실시간으로 알림 |
| FR-N03 | 멘션 알림 | `@나` 언급 시 강조 알림 (DND 상태에서도 표시) |
| FR-N04 | DND 모드 | GUI 토글 버튼으로 알림 무음 설정 (멘션 제외) |
| FR-N05 | 타이핑 표시 | 상대방이 입력 중일 때 `홍길동 님이 입력 중...` 표시 |
| FR-N06 | 방 알림 무음 | GUI 메뉴에서 특정 채팅방 알림만 무음 처리 |

---

### 3-8. 유저 커스터마이징

| ID | 기능 | 설명 |
|----|------|------|
| FR-C01 | 내 메시지 색상 | 내가 보낸 메시지의 텍스트 색상 선택 (GTK4 색상 선택기) |
| FR-C02 | 닉네임 색상 | 채팅창에 표시되는 내 닉네임 색상 선택 (GTK4 색상 선택기) |
| FR-C03 | 테마 | dark / light GTK4 테마 선택 |
| FR-C04 | 타임스탬프 형식 | `HH:MM` / `HH:MM:SS` / `MM-DD HH:MM` 중 선택 |
| FR-C05 | 상태메시지 | 친구 목록에 표시되는 한 줄 상태메시지 수정 |
| FR-C06 | 온라인 상태 | `online` / `busy` / `invisible` 수동 설정 |
| FR-C07 | 오픈채팅 닉네임 | 오픈채팅방별 별도 닉네임 설정 (최대 20자, `:` · `;` · `\|` 사용 불가) |

---

### 3-9. 마이페이지

| ID | 기능 | 설명 |
|----|------|------|
| FR-P01 | 프로필 조회 | ID, 닉네임, 상태메시지, 가입일, 마지막 접속 시간 |
| FR-P02 | 활동 통계 | 총 전송 메시지 수, 참여 중인 채팅방 수, 친구 수 |
| FR-P03 | 참여 채팅방 목록 | 현재 참여 중인 그룹/오픈채팅방 목록 |
| FR-P04 | 최근 DM 목록 | 최근 대화한 상대 목록 (마지막 메시지, 안읽은 수 포함) |
| FR-P05 | 프로필 수정 | → FR-A04 참조 (마이페이지 탭에서의 진입점) |
| FR-P06 | 비밀번호 변경 | → FR-A05 참조 (마이페이지 탭에서의 진입점) |

---

### 3-10. 관리자 기능 *(Out-of-Scope)*

> 아래 기능은 현재 범위에서 제외되었습니다.

| ID | 기능 | 설명 |
|----|------|------|
| FR-ADM01 | ~~전체 공지~~ | ~~서버 전체 접속 유저에게 공지 브로드캐스트~~ |
| FR-ADM02 | ~~유저 강제 로그아웃~~ | ~~특정 유저 강제 접속 해제~~ |
| FR-ADM03 | ~~서버 상태 조회~~ | ~~접속 인원, 활성 채팅방 수, DB 레코드 수 등 통계~~ |
| FR-ADM04 | ~~유저 목록 조회~~ | ~~전체 가입 유저 목록 (온/오프라인 포함)~~ |
| FR-ADM05 | ~~채팅방 강제 삭제~~ | ~~특정 채팅방 강제 종료 및 삭제~~ |

---

## 4. 메시지 프로토콜

### 4-1. 패킷 형식
```
<TYPE>|<PAYLOAD>\n
```
- 필드 구분자: `|`
- 각 필드 내부 다중값 구분자: `:`
- 리스트 항목 구분자: `;`
- 종단 문자: `\n`
- 최대 패킷 길이: 10240 bytes (목록 응답 포함)
- **content 필드 파싱 규칙**: content(메시지 본문)·status_msg·topic 등 자유 텍스트 필드는 항상 해당 항목의 마지막 필드로 배치. 파서는 고정 필드 N개 파싱 후 나머지를 자유 텍스트로 처리. 자유 텍스트 내 `:` 허용. 자유 텍스트 내 `\n` · `;` · `|` 포함 불가 (`\n`은 패킷 종단자, `;`는 리스트 구분자, `|`는 패킷 타입 구분자)
- **리스트 패킷 count 규칙**: 자유 텍스트 필드를 포함하는 리스트 패킷(`FRIEND_LIST_RES`, `USER_SEARCH_RES`, `DM_HISTORY_RES`, `ROOM_HISTORY_RES`, `MSG_SEARCH_RES`, `DM_LIST_RES`)은 PAYLOAD 첫 필드로 `<count>`(항목 수)를 포함. 파서는 count 값으로 루프 횟수를 결정하며 `;`는 보조 구분자로만 사용.

### 4-2. 패킷 정의

**인증**
```
C→S  LOGIN_REQ|<id>:<pass>
S→C  LOGIN_RES|<code>                    # code: 0=OK, 1=WRONG_ID, 2=WRONG_PW
                                         # 3=ALREADY_ONLINE — P1 이후 구현 (P0에서는 중복 로그인 허용)

C→S  REGISTER_REQ|<id>:<pass>[:<nick>[:<status>]]
S→C  REGISTER_RES|<code>                 # code: 1=OK, 2=DUPLICATE_ID

C→S  LOGOUT_REQ|          # P1 이후 구현 (P0에서는 소켓 종료로 대체)
S→C  LOGOUT_RES|OK        # P1 이후 구현
```

**친구**
```
C→S  FRIEND_ADD_REQ|<target_id>
S→C  FRIEND_ADD_RES|<code>               # 0=SENT, 1=NOT_FOUND, 2=BLOCKED, 3=ALREADY_FRIEND
S→C  FRIEND_REQUEST_NOTIFY|<from_id>:<from_nick>

C→S  FRIEND_ACCEPT|<from_id>
S→C  FRIEND_ACCEPT_NOTIFY|<user_id>:<nick>   # 수락한 상대(from_id)의 ID·닉네임 → 요청 송신자에게 전달
C→S  FRIEND_REJECT|<from_id>
C→S  FRIEND_DELETE|<target_id>
C→S  FRIEND_BLOCK|<target_id>

C→S  FRIEND_LIST_REQ|
S→C  FRIEND_LIST_RES|<count>:<id>:<nick>:<status>:<status_msg>;<id>:...

S→C  FRIEND_STATUS_CHANGE|<id>:<nick>:<status>   # 친구 접속/해제 실시간 알림

C→S  USER_SEARCH|<keyword>
S→C  USER_SEARCH_RES|<count>:<id>:<nick>:<status_msg>;<id>:...
```

**DM**
```
C→S  DM_SEND|<to_id>:<content>
S→C  DM_RECV|<from_id>:<from_nick>:<timestamp>:<msg_id>:<content>
S→C  DM_READ_NOTIFY|<reader_id>            # DM을 읽은 수신자 ID → 원래 송신자에게 전달
C→S  DM_HISTORY_REQ|<with_id>:<count>
S→C  DM_HISTORY_RES|<count>:<msg_id>:<from_id>:<timestamp>:<read>:<content>;<msg_id>:...
C→S  DM_LIST_REQ|                          # P1 이후 구현 (FR-P04)
S→C  DM_LIST_RES|<count>:<partner_id>:<partner_nick>:<timestamp>:<unread>:<last_msg>;<partner_id>:...
     # last_msg는 자유 텍스트(마지막 필드), count 규칙 적용
```

**그룹 채팅방**
```
C→S  ROOM_CREATE|<name>:<max_users>[:<is_open>[:<password>[:<topic>]]]
     # is_open 기본값=0(그룹), password 기본값=""(없음), topic 기본값=""
S→C  ROOM_CREATE_RES|<code>:<room_id>        # code: 1=OK, 0=FAIL (MAX_ROOMS 초과 등). FAIL 시 room_id=0

C→S  ROOM_LIST_REQ[|<type>]          # type: group | open (생략 시 전체 반환, P0 기본값)
S→C  ROOM_LIST_RES|<id>:<name>:<cur>:<max>:<has_pw>:<is_open>:<topic>;<id>:...

C→S  ROOM_SEARCH|<type>:<keyword>           # type: group | open | all. keyword는 마지막 필드(자유 텍스트)
S→C  ROOM_SEARCH_RES|<id>:<name>:<cur>:<max>:<has_pw>:<is_open>:<topic>;<id>:...

C→S  ROOM_JOIN|<room_id>[:<password>]   # 비밀번호 없는 방은 room_id만 전송
S→C  ROOM_JOIN_RES|<code>:<room_id>:<room_name>  # code: 1=OK, 0=FAIL (방 없음·인원 초과·비밀번호 불일치)
S→C  ROOM_NOTICE|<room_id>:<notice_text>
S→C  ROOM_PIN|<room_id>:<msg_id>:<from_nick>:<content>
     # 입장 시 현재 핀 메시지 상태 전달 (ROOM_JOIN_RES 직후 서버가 push)

C→S  ROOM_MSG|<room_id>:<content>
S→C  ROOM_MSG_RECV|<room_id>:<from_nick>:<timestamp>:<msg_id>:<reply_to_id>:<msg_type>:<content>
     # msg_type: 0=normal, 1=system, 2=whisper, 3=me-action
     # reply_preview 제거 — 클라이언트가 reply_to_id로 로컬 캐시에서 인용문 생성

C→S  ROOM_HISTORY_REQ|<room_id>:<count>   # P1 이후 구현 (FR-G09). count: 요청 메시지 수 (기본 100)
S→C  ROOM_HISTORY_RES|<count>:<msg_id>:<from_nick>:<timestamp>:<reply_to_id>:<msg_type>:<content>;<msg_id>:...
     # content-last 규칙 적용. from_nick은 오픈채팅 시 open_nick 우선

C→S  ROOM_LEAVE|<room_id>
     # 응답 패킷 없음 — 퇴장은 항상 성공으로 간주. 다른 멤버 알림은 ROOM_MSG_RECV(msg_type=1, 시스템 메시지)로 브로드캐스트
C→S  ROOM_INVITE|<room_id>:<target_id>
S→C  ROOM_INVITE_RES|<code>                   # 0=SENT, 1=NOT_FOUND, 2=ALREADY_MEMBER, 3=ROOM_FULL
S→C  ROOM_INVITE_NOTIFY|<room_id>:<room_name>:<inviter_nick>   # 초대받은 사용자에게 전달 (P1)
C→S  ROOM_KICK|<room_id>:<target_id>          # 방장/관리자 전용
S→C  ROOM_KICKED_NOTIFY|<room_id>:<by_nick>  # 강퇴당한 사용자에게 전달 (P2, FR-G06)
C→S  ROOM_DELETE|<room_id>                   # 방장 전용 (P2, FR-G06)
S→C  ROOM_DELETED_NOTIFY|<room_id>           # 방 삭제 시 전체 멤버에게 전달 (P2, FR-G06)
C→S  ROOM_SET_NOTICE|<room_id>:<notice>       # 방장/관리자 전용
C→S  ROOM_GRANT_ADMIN|<room_id>:<target_id>   # 방장 전용
C→S  ROOM_REVOKE_ADMIN|<room_id>:<target_id>  # 방장 전용
C→S  ROOM_MEMBERS_REQ|<room_id>
S→C  ROOM_MEMBERS_RES|<room_id>:<id>:<nick>:<is_admin>:<online>;<id>:...
```

**메시지 부가기능**
```
C→S  WHISPER|<to_nick>:<content>
S→C  WHISPER_RECV|<from_nick>:<timestamp>:<content>

C→S  MSG_DELETE|<room_id>:<msg_id>
S→C  MSG_DELETED_NOTIFY|<room_id>:<msg_id>

C→S  MSG_EDIT|<room_id>:<msg_id>:<new_content>
S→C  MSG_EDITED_NOTIFY|<room_id>:<msg_id>:<new_content>

C→S  MSG_REPLY|<room_id>:<reply_to_id>:<content>
     # 처리 후 ROOM_MSG_RECV 로 브로드캐스트 (reply_to_id 포함)

C→S  MSG_REACT|<room_id>:<msg_id>:<emoji>     # Out-of-Scope (FR-M05)
S→C  MSG_REACT_NOTIFY|<room_id>:<msg_id>:<emoji>:<count>:<user_list>  # Out-of-Scope

C→S  MSG_SEARCH|<room_id>:<keyword>
S→C  MSG_SEARCH_RES|<count>:<msg_id>:<from_nick>:<timestamp>:<content>;<msg_id>:...

C→S  MSG_PIN|<room_id>:<msg_id>               # 방장/관리자 전용
S→C  MSG_PIN_NOTIFY|<room_id>:<msg_id>:<from_nick>:<content_preview>
     # 핀 동작 발생 시 실시간 브로드캐스트 (ROOM_PIN과 구분: ROOM_PIN은 입장 시 초기 상태)
```

**커스터마이징 / 설정**
```
C→S  SETTINGS_REQ|
S→C  SETTINGS_RES|<msg_color>:<nick_color>:<theme>:<ts_format>:<dnd>

C→S  SETTINGS_UPDATE|<msg_color>:<nick_color>:<theme>:<ts_format>:<dnd>
S→C  SETTINGS_UPDATE_RES|<code>              # 0=OK, 1=FAIL

C→S  STATUS_CHANGE|<status>                   # online | busy | invisible
C→S  PROFILE_UPDATE|<nickname>:<status_msg>
S→C  PROFILE_UPDATE_RES|<code>               # 0=OK, 1=DUPLICATE_NICK

C→S  PASS_CHANGE|<old_pass>:<new_pass>
S→C  PASS_CHANGE_RES|<code>                  # 0=OK, 1=WRONG_PW

C→S  ROOM_SET_OPEN_NICK|<room_id>:<nick>
S→C  ROOM_SET_OPEN_NICK_RES|<code>           # 0=OK, 1=FAIL

C→S  ROOM_MUTE_TOGGLE|<room_id>             # P3 이후 구현 (FR-N06). is_muted 토글
S→C  ROOM_MUTE_TOGGLE_RES|<room_id>:<is_muted>   # 변경 후 현재 상태 반환 (0=unmuted, 1=muted)
```

**마이페이지**
```
C→S  MYPAGE_REQ|
S→C  MYPAGE_RES|<id>:<nick>:<created_at>:<last_seen>:<msg_count>:<room_count>:<friend_count>:<status_msg>
C→S  MY_ROOMS_REQ|                         # P1 이후 구현 (FR-P03)
S→C  MY_ROOMS_RES|<id>:<name>:<cur>:<max>:<has_pw>:<is_open>:<topic>;<id>:...
     # ROOM_LIST_RES와 동일한 포맷. `;` 구분자 사용 (topic 내 `;` 포함 불가)
```

**타이핑 인디케이터**
```
C→S  TYPING_START|<room_id>
C→S  TYPING_STOP|<room_id>
S→C  TYPING_NOTIFY|<room_id>:<nick>:<is_typing>   # 0 or 1
```

**알림**
```
S→C  NOTIFY|<type>:<content>    # type: MENTION | FRIEND_REQ | SERVER | DM
                                # REACTION은 FR-M05 Out-of-Scope — v2.1 이후 추가
```

**관리자** *(Out-of-Scope)*
```
# C→S  ADMIN_CMD|<cmd>:<args>
#      cmd: broadcast, kick_user, server_stat, user_list, delete_room
# S→C  ADMIN_RES|<code>:<data>
```

**연결 유지** *(P2 이후 구현 — 수신 스레드 활성화 이후 적용)*
```
C→S  PING|
S→C  PONG|
```

---

## 5. 데이터베이스 스키마

```sql
-- 유저
CREATE TABLE users (
    id            VARCHAR(20)  PRIMARY KEY,
    password_hash VARCHAR(64)  NOT NULL,        -- SHA2(pass, 256)
    nickname      VARCHAR(20)  NOT NULL UNIQUE,  -- 닉네임 중복 불허 (귓속말 대상 식별)
    status_msg    VARCHAR(100) DEFAULT '',
    online_status TINYINT      DEFAULT 0,        -- 0=offline, 1=online, 2=busy, 3=invisible
    is_admin      TINYINT      DEFAULT 0,
    last_seen     DATETIME     DEFAULT NULL,
    created_at    DATETIME     DEFAULT CURRENT_TIMESTAMP
);

-- 유저 설정 (커스터마이징)
CREATE TABLE user_settings (
    user_id    VARCHAR(20) PRIMARY KEY,
    msg_color  VARCHAR(15) DEFAULT 'cyan',
    nick_color VARCHAR(15) DEFAULT 'yellow',
    theme      VARCHAR(10) DEFAULT 'dark',
    ts_format  TINYINT     DEFAULT 0,           -- 0=HH:MM, 1=HH:MM:SS, 2=MM-DD HH:MM
    dnd        TINYINT     DEFAULT 0,           -- 1=방해금지 모드
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- 친구 관계
CREATE TABLE friends (
    id           INT          AUTO_INCREMENT PRIMARY KEY,
    user_id      VARCHAR(20)  NOT NULL,
    friend_id    VARCHAR(20)  NOT NULL,
    status       TINYINT      DEFAULT 0,       -- 0=pending, 1=accepted, 2=blocked
    created_at   DATETIME     DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uniq_pair (user_id, friend_id),
    FOREIGN KEY (user_id)   REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (friend_id) REFERENCES users(id) ON DELETE CASCADE
);

-- 채팅방
CREATE TABLE rooms (
    id           INT          AUTO_INCREMENT PRIMARY KEY,
    name         VARCHAR(30)  NOT NULL,
    topic        VARCHAR(100) DEFAULT '',
    password_hash VARCHAR(64) DEFAULT '',      -- 빈 문자열 = 공개
    max_users    INT          DEFAULT 10,
    owner_id     VARCHAR(20)  NOT NULL,
    notice       VARCHAR(255) DEFAULT '',
    is_open      TINYINT      DEFAULT 0,       -- 0=그룹(초대), 1=오픈채팅
    pinned_msg_id INT         DEFAULT NULL,
    created_at   DATETIME     DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (owner_id) REFERENCES users(id)
);

-- 채팅방 멤버
CREATE TABLE room_members (
    room_id      INT          NOT NULL,
    user_id      VARCHAR(20)  NOT NULL,
    open_nick    VARCHAR(20)  DEFAULT '',      -- 오픈채팅 전용 닉네임
    is_admin     TINYINT      DEFAULT 0,
    is_muted     TINYINT      DEFAULT 0,       -- 방 알림 무음
    joined_at    DATETIME     DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (room_id, user_id),
    FOREIGN KEY (room_id)  REFERENCES rooms(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id)  REFERENCES users(id) ON DELETE CASCADE
);

-- 메시지
CREATE TABLE messages (
    id           INT          AUTO_INCREMENT PRIMARY KEY,
    room_id      INT          DEFAULT NULL,    -- NULL 이면 DM
    from_id      VARCHAR(20)  NOT NULL,
    to_id        VARCHAR(20)  DEFAULT NULL,    -- DM 수신자
    content      VARCHAR(500) NOT NULL,
    reply_to     INT          DEFAULT NULL,    -- 답장 원본 msg_id
    msg_type     TINYINT      DEFAULT 0,       -- 0=normal, 1=system, 2=whisper, 3=me-action
    is_deleted   TINYINT      DEFAULT 0,
    created_at   DATETIME     DEFAULT CURRENT_TIMESTAMP,
    edited_at    DATETIME     DEFAULT NULL,
    FOREIGN KEY (from_id)  REFERENCES users(id),
    FOREIGN KEY (reply_to) REFERENCES messages(id) ON DELETE SET NULL
);

-- DM 읽음 상태
CREATE TABLE dm_reads (
    msg_id       INT          NOT NULL,
    reader_id    VARCHAR(20)  NOT NULL,
    read_at      DATETIME     DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (msg_id, reader_id),
    FOREIGN KEY (msg_id)    REFERENCES messages(id) ON DELETE CASCADE,
    FOREIGN KEY (reader_id) REFERENCES users(id)    ON DELETE CASCADE
);

-- 채팅방 초대 (오프라인 사용자 초대 보관)
CREATE TABLE room_invites (
    id         INT          AUTO_INCREMENT PRIMARY KEY,
    room_id    INT          NOT NULL,
    inviter_id VARCHAR(20)  NOT NULL,
    invitee_id VARCHAR(20)  NOT NULL,
    status     TINYINT      NOT NULL DEFAULT 0,         -- 0=pending, 1=수락, 2=거절
    created_at DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uq_invite (room_id, invitee_id),
    INDEX      idx_invitee (invitee_id),
    FOREIGN KEY (room_id)    REFERENCES rooms(id)   ON DELETE CASCADE,
    FOREIGN KEY (inviter_id) REFERENCES users(id)   ON DELETE CASCADE,
    FOREIGN KEY (invitee_id) REFERENCES users(id)   ON DELETE CASCADE
);

-- 메시지 리액션 (Out-of-Scope: FR-M05 — v2.1 이후 별도 마이그레이션으로 추가)
-- CREATE TABLE reactions ( ... );
```

---

## 6. 서버 인메모리 세션 구조체

```c
/* 서버 전역 상수 */
#define MAX_CLIENTS      256   /* 최대 동시 접속 세션 */
#define MAX_ROOMS        100   /* 최대 채팅방 수 */
#define MAX_ROOM_MEMBERS  64   /* 방당 최대 멤버 수 (in-memory 배열 크기, FR-G01 최대 64명 기준) */
#define DEFAULT_PORT   55555   /* 기본 포트 번호 */

/* 접속 중인 클라이언트 세션 (DB에 저장 안 함) */
typedef struct {
    int      fd;
    char     user_id[21];
    char     nickname[21];
    int      online_status;   /* 0=offline, 1=online, 2=busy, 3=invisible */
    int      dnd;
    int      current_room_id; /* 0 = 채팅방 밖 */
    int      muted_rooms[32]; /* 알림 무음 room_id 목록 */
    int      muted_count;
    int      is_admin;
    int      active;          /* 1 = 유효한 세션 */
    MYSQL   *db;              /* 스레드 전용 MySQL 연결 */
    HANDLE   hThread;         /* Windows 스레드 핸들 (_beginthreadex) */
} ClientSession;

extern ClientSession g_sessions[MAX_CLIENTS]; /* 전역, mutex 보호 */
extern HANDLE g_sessions_mutex;               /* Windows Mutex 핸들 (CreateMutex) */
```

---

## 7. 클라이언트 GTK4 GUI 화면 설계

### 로그인 창 (GtkWindow)
- `GtkEntry` — ID 입력
- `GtkEntry` (visibility=false) — 비밀번호 입력 (`*` 마스킹, `gtk_entry_set_visibility(FALSE)`)
- `GtkButton` — 로그인 / 회원가입 버튼
- 오류 메시지는 `GtkLabel` (빨간색)로 창 하단 표시

### 메인 창 — 탭 구조 (GtkNotebook)
- **탭 1 — 친구목록**: `GtkListBox`에 온라인/오프라인 친구 항목 (상태 아이콘 + 닉네임 + 상태메시지)
- **탭 2 — 채팅**: 참여 중인 채팅방 목록, 미읽 메시지 수 배지
- **탭 3 — 오픈채팅**: 오픈채팅방 목록 및 검색
- **탭 4 — 마이페이지**: 프로필, 통계, 참여 방 목록

### 채팅 창 (GtkWindow 또는 GtkNotebook 탭)
- 상단 `GtkHeaderBar`: 방 이름, 참여 인원, 공지사항 표시
- 핀 메시지 `GtkRevealer` (접을 수 있음)
- 메시지 영역 `GtkScrolledWindow` + `GtkListBox`
  - 내 메시지: 오른쪽 정렬 말풍선
  - 상대 메시지: 왼쪽 정렬 말풍선 (닉네임 + 타임스탬프 + [읽음])
  - 시스템 메시지: 중앙 정렬, 회색 텍스트
- 하단 입력 영역: `GtkEntry` + 전송 `GtkButton`
- 타이핑 표시: 입력 영역 위 `GtkLabel` (`홍길동 님이 입력 중...`)

### 마이페이지 (GtkGrid 레이아웃)
- ID, 닉네임, 상태메시지, 가입일, 마지막 접속 시간 표시
- 활동 통계 (총 메시지, 참여 방, 친구 수)
- 프로필 수정 / 비밀번호 변경 `GtkButton`

### 설정 창 (GtkWindow)
- 내 메시지 색상: `GtkColorDialogButton`
- 닉네임 색상: `GtkColorDialogButton`
- 테마: `GtkDropDown` (dark / light)
- 타임스탬프 형식: `GtkDropDown`
- DND 모드: `GtkSwitch`

---

## 8. 클라이언트 GUI 메뉴/버튼

| 기능 | GUI 요소 |
|------|----------|
| 도움말 | `GtkAboutDialog` 또는 인앱 도움말 버튼 |
| 귓속말 (`/w`) | 채팅 창 우클릭 컨텍스트 메뉴 → "귓속말 보내기" |
| 메시지 삭제 (`/del`) | 메시지 말풍선 호버 시 삭제 버튼 표시 |
| 메시지 수정 (`/edit`) | 메시지 말풍선 호버 시 수정 버튼 (5분 이내) |
| 답장 인용 (`/reply`) | 메시지 말풍선 호버 시 "답장" 버튼 |
| 메시지 핀 고정 (`/pin`) | 방장/관리자: 컨텍스트 메뉴 → "핀 고정" |
| 메시지 검색 (`/search`) | 채팅 창 상단 검색 아이콘 버튼 |
| 친구 초대 (`/invite`) | 채팅 창 멤버 패널 → "초대" 버튼 |
| 멤버 강퇴 (`/kick`) | 멤버 목록 컨텍스트 메뉴 → "강퇴" (방장/관리자) |
| 공지 등록 (`/notice`) | 채팅 창 상단 ⚙ 버튼 → "공지 설정" (방장/관리자) |
| 공동 방장 부여/해제 (`/grant`, `/revoke`) | 멤버 목록 컨텍스트 메뉴 (방장 전용) |
| 현재 방 멤버 목록 (`/members`) | 채팅 창 우측 멤버 패널 (토글) |
| 방 알림 무음 (`/mute`) | 채팅 목록 항목 우클릭 → "알림 무음 토글" |
| 채팅방 나가기 (`/leave`) | 채팅 창 상단 ⚙ 버튼 → "채팅방 나가기" |
| 액션 메시지 (`/me`) | 입력창 `/me` 접두사 입력 시 액션 모드로 전송 |
| 오픈채팅 닉네임 (`/open_nick`) | 오픈채팅 참여 시 닉네임 입력 다이얼로그 |
| 친구 추가 | 메인 창 친구목록 탭 → "+" 버튼 |
| 친구 목록 | 메인 창 친구목록 탭 |
| 친구 차단 | 친구 항목 우클릭 → "차단" |
| 유저/닉네임 검색 | 메인 창 검색창 |
| 채팅방 목록 (그룹/오픈) | 메인 창 채팅/오픈채팅 탭 |
| 채팅방 검색 | 오픈채팅 탭 상단 검색창 |
| 상태 변경 | 상단 바 아바타 클릭 → 상태 드롭다운 (online / busy / invisible) |
| 프로필 수정 | 마이페이지 탭 → "프로필 수정" 버튼 |
| DND 모드 | 상단 바 또는 설정 창 `GtkSwitch` |
| 마이페이지 | 메인 창 마이페이지 탭 |
| 설정 | 상단 바 ⚙ 버튼 → 설정 `GtkWindow` (모달) |

---

## 9. 비기능 요구사항

| ID | 항목 | 목표 |
|----|------|------|
| NFR-01 | 동시 접속 | 최대 256명 동시 접속 지원 (MAX_CLIENTS=256) |
| NFR-02 | 응답 지연 | 메시지 전달 지연 50ms 이하 (로컬 네트워크 기준) |
| NFR-03 | 안정성 | 클라이언트 비정상 종료 시 서버 크래시 없음 |
| NFR-04 | 보안 | 비밀번호 SHA-256 해시 저장 (MySQL `SHA2()` 활용). 평문 전송은 로컬 환경 한정 |
| NFR-05 | 경량성 | 서버 메모리 사용 100MB 이하 (MySQL 제외) |
| NFR-06 | 이식성 | Windows(WinSock2, MinGW/MSVC) 단일 빌드. GTK4 4.0+ 필요 |
| NFR-07 | 확장성 | 서버/클라이언트 분리 구조. GTK4 GUI 레이어 기반 |
| NFR-08 | 영속성 | 서버 재시작 후 메시지·유저·채팅방 데이터 유지 (MySQL) |
| NFR-09 | 스레드 안전 | DB 연결은 스레드 전용. 세션 배열 접근은 Windows Mutex(CreateMutex/WaitForSingleObject/ReleaseMutex) 보호 |

---

## 10. 예상 파일 구조

```
C_ChatProgram/
├── chat_program/
│   └── src/
│       ├── server/
│       │   ├── main.c              # 서버 진입점, accept loop
│       │   ├── config.h            # DB 접속 정보, 포트, MAX_CLIENTS 상수
│       │   ├── globals.c / globals.h  # g_sessions[], g_sessions_mutex 선언·정의
│       │   ├── client_handler.c/h  # 클라이언트별 스레드
│       │   ├── router.c / router.h # 패킷 타입별 핸들러 라우팅
│       │   ├── db.c / db.h         # MySQL 연결 래퍼, 쿼리 헬퍼
│       │   ├── auth.c / auth.h     # 회원가입, 로그인
│       │   ├── user_store.c/h      # 유저·설정 CRUD
│       │   ├── friend.c / friend.h # 친구 요청/수락/차단
│       │   ├── room.c / room.h     # 채팅방 생성/참여/관리
│       │   ├── dm.c / dm.h         # 1:1 DM 처리
│       │   ├── message.c / .h      # 메시지 저장·삭제·수정·검색
│       │   ├── broadcast.c / .h    # 룸 브로드캐스트, 알림 전송
│       │   └── admin.c / admin.h   # 관리자 명령 처리 (Out-of-Scope)
│       │
│       ├── client/
│       │   ├── main.c              # GTK 앱 진입점, css 로드, app_window 생성
│       │   ├── state.c / state.h   # 전역 클라이언트 상태 (소켓, 현재 화면, 설정)
│       │   ├── net.c / net.h       # 소켓 연결, recv 스레드, send 함수
│       │   ├── packet.c / packet.h # packet_build / packet_parse 직렬화
│       │   ├── app_window.c/h      # GtkApplicationWindow, GtkStack 화면 전환
│       │   ├── notify.c / notify.h # 알림 GtkRevealer 큐, 표시 처리
│       │   ├── screen_login.c/h    # 로그인·회원가입 화면
│       │   ├── screen_main.c/h     # 메인(친구·채팅 목록) 화면
│       │   ├── screen_chat.c/h     # 채팅방 화면
│       │   ├── screen_mypage.c/h   # 마이페이지 화면
│       │   ├── screen_settings.c/h # 설정 화면
│       │   └── css/
│       │       ├── theme-dark.css
│       │       ├── theme-light.css
│       │       ├── components.css
│       │       ├── chat.css
│       │       └── login.css
│       │
│       └── common/
│           ├── protocol.h          # 패킷 타입 상수, 구분자, 응답 코드
│           ├── types.h             # 공통 구조체 (UserSettings 등)
│           ├── utils.c / utils.h   # 타임스탬프, 이모티콘 변환, 문자열 유틸
│           └── net_win.c           # Windows(Winsock2) 소켓 구현
│
├── sql/
│   └── schema.sql              # DB 생성·테이블 DDL, 시드 데이터
│
├── docs/                       # 설계 문서 (10개 섹션)
├── Makefile
└── requirements.md
```

---

## 11. 개발 우선순위

| 우선순위 | FR 범위 | 핵심 이유 |
|----------|---------|-----------|
| P0 (필수) | FR-A01~A03 · FR-G01,G03,G05 · FR-O01,O02,O04 · ROOM_JOIN(방 입장) | 동작하는 MVP (로그인 + 방 생성/입장/퇴장 + 그룹/오픈채팅 브로드캐스트) |
| P1 (중요) | FR-A03(LOGOUT_REQ) · FR-A04~A06 · FR-F01~F07 · FR-D01~D05 · FR-G02(멤버 초대) · FR-G09(메시지 히스토리) · FR-P01~P06 · ALREADY_ONLINE(중복 로그인 차단) | 핵심 소셜 기능 (친구·DM·초대·히스토리·마이페이지·정식 로그아웃) |
| P2 (권장) | FR-M01~M03,M07,M09 · FR-G06~G08,G10 · FR-C01~C07 · FR-N01,N02 · PING/PONG(연결 유지) | 완성도 향상 (메시지 편집·알림·설정·연결 유지) |
| P3 (선택) | FR-M04,M06,M08,M10,M11 · FR-O03 · FR-N03~N06 | 풍부한 경험 (답장·검색·타이핑·DND) |
| Out-of-Scope | FR-M05(리액션) · FR-ADM01~ADM05 | v2.1 이후 별도 구현 |

> **Phase 0 구현 노트**
> - MySQL 환경이 없는 경우 사용자 데이터를 `users.txt` 파일(`id//pw` 포맷)로 대체 가능. 서버 기동 시 파일에서 로드하고, 신규 가입 시 메모리 배열에만 저장한다. Phase 1 전환 시 MySQL로 교체.
> - 서버 기본 포트는 `DEFAULT_PORT 55555`. 클라이언트는 `127.0.0.1:55555`로 접속.
> - 클라이언트 방 입장(`ROOM_JOIN`) 화면은 P0 구현 대상이나, 방 초대(`ROOM_INVITE`)는 P1 이후 구현.
> - `gets()` 대신 `fgets()` 또는 `scanf("%Ns", ...)` 사용 필수 (보안, NFR-04).
> - **P0 브로드캐스트**: 전체 접속자 배열(`clientSocks[]`)에 일괄 전송. 방별 격리 브로드캐스트는 P1 이후 구현. 클라이언트가 수신 메시지의 `room_id`를 확인해 현재 방 메시지만 화면에 표시.
> - **비정상 종료 처리**: 클라이언트 소켓 종료 시 `clientSocks[]`에서만 제거. P0에서 방 멤버 배열 정리는 생략 (전체 브로드캐스트 구조에서 문제 없음).
> - **`users.txt` 미존재 처리**: 서버 기동 시 파일이 없으면 빈 사용자 목록으로 시작해야 한다 (`fopen()` 반환값 NULL 체크 필수). 파일 없이 `feof(NULL)` 호출 시 세그폴트 발생.
> - **비밀번호 마스킹 (콘솔 Phase 0)**: 콘솔 클라이언트에서 비밀번호 입력 시 `<conio.h>`의 `getch()`로 문자를 한 자씩 읽어 `*`를 에코한다. Enter(`\r`, 아스키 13) 입력 시 루프 종료. GTK4 전환 후에는 `gtk_entry_set_visibility(entry, FALSE)` 로 대체.
