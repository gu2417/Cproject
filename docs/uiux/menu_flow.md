# 메뉴 흐름 다이어그램

---

## 1. 전체 화면 전환 흐름

```
[프로그램 시작]
      │
      ▼
┌─────────────────────────────────┐
│   초기 화면 (InitialMenu)        │
│   1.로그인  2.회원가입  3.종료   │
└─────────────────────────────────┘
      │ 1(로그인 성공)
      ▼
┌─────────────────────────────────────────────────────┐
│   메인 로비 (ShowMainMenu)   홍길동 [ONLINE]         │
│   1.친구목록  2.그룹채팅  3.오픈채팅                 │
│   4.DM       5.마이페이지  6.설정  7.로그아웃        │
└─────────────────────────────────────────────────────┘
  │1       │2        │3         │4      │5    │6    │7
  ▼        ▼         ▼          ▼       ▼     ▼     ▼
친구목록  그룹채팅  오픈채팅   DM목록  마이  설정  로그
화면     목록화면  목록화면   화면    페이지 화면  아웃
```

---

## 2. 로그인 / 회원가입 흐름

```
[초기 화면]
   │ 1              │ 2
   ▼                ▼
[로그인 화면]    [회원가입 화면]
ID / PW 입력     ID / PW / 닉네임 / 상태메시지 입력
   │                │
   │ RES|0 성공     │ RES|1 성공
   ▼                ▼
[로그인 후 처리]  [초기 화면으로 복귀]
  - pending 친구 요청 알림 표시
  - pending 채팅방 초대 알림 표시
  - 친구들에게 FRIEND_STATUS_CHANGE 전송
   │
   ▼
[메인 로비]
```

---

## 3. 채팅방 진입 흐름 (그룹 / 오픈채팅 공통)

```
[방 목록 화면]
      │ 번호 선택
      ▼
[비밀번호 있는 방?]
  YES │         │ NO
      ▼         ▼
[비번 입력]   [ROOM_JOIN 전송]
  │ 틀림          │
  ▼               │ ROOM_JOIN_RES|1:room_id:room_name
[오류 메시지]      ▼
목록 복귀    [오픈채팅방?]
              YES │   │ NO
                  ▼   ▼
           [오픈닉 입력]  [히스토리 요청]
           ROOM_SET_OPEN_NICK  ROOM_HISTORY_REQ|room_id:100
                  │             │
                  ▼             ▼
             [채팅방 내부 화면 ShowChatRoom]
             - ROOM_NOTICE 수신 → 공지 상단 표시
             - ROOM_PIN 수신 → 핀 상단 표시
             - ROOM_HISTORY_RES 수신 → 이전 메시지 출력
```

---

## 4. 채팅방 내부 흐름

```
[채팅방 내부]
      │
      ├── 일반 텍스트 입력    → ROOM_MSG 전송 → ROOM_MSG_RECV 브로드캐스트
      │
      ├── /leave             → ROOM_LEAVE 전송 → 메인 로비 복귀
      ├── /members           → ROOM_MEMBERS_REQ → 멤버 목록 화면
      ├── /invite <id>       → ROOM_INVITE 전송
      ├── /w <닉> <내용>     → WHISPER 전송
      ├── /me <동작>         → ROOM_MSG (msg_type=3)
      ├── /del <msg_id>      → MSG_DELETE 전송
      ├── /edit <id> <내용>  → MSG_EDIT 전송 (5분 이내)
      ├── /reply <id> <내용> → MSG_REPLY 전송
      ├── /pin <msg_id>      → MSG_PIN 전송 (방장/관리자)
      ├── /notice <내용>     → ROOM_SET_NOTICE (방장/관리자)
      ├── /kick <id>         → ROOM_KICK (방장/관리자)
      ├── /grant <id>        → ROOM_GRANT_ADMIN (방장 전용)
      ├── /revoke <id>       → ROOM_REVOKE_ADMIN (방장 전용)
      ├── /search <키워드>   → MSG_SEARCH → 검색 결과 화면
      ├── /open_nick <닉>    → ROOM_SET_OPEN_NICK (오픈채팅)
      └── /help              → 명령어 목록 출력
```

---

## 5. 채팅방 명령어 목록

| 명령어 | 형식 | 권한 | 우선순위 |
|--------|------|------|----------|
| `/leave` | `/leave` | 전체 | P0 |
| `/help` | `/help` | 전체 | P0 |
| `/invite` | `/invite <user_id>` | 전체 | P1 |
| `/members` | `/members` | 전체 | P2 |
| `/w` | `/w <닉네임> <내용>` | 전체 | P2 |
| `/del` | `/del <msg_id>` | 본인/방장 | P2 |
| `/edit` | `/edit <msg_id> <내용>` | 본인 (5분) | P2 |
| `/notice` | `/notice <내용>` | 방장/관리자 | P2 |
| `/kick` | `/kick <user_id>` | 방장/관리자 | P2 |
| `/grant` | `/grant <user_id>` | 방장 전용 | P2 |
| `/revoke` | `/revoke <user_id>` | 방장 전용 | P2 |
| `/open_nick` | `/open_nick <닉네임>` | 오픈채팅 | P2 |
| `/reply` | `/reply <msg_id> <내용>` | 전체 | P3 |
| `/pin` | `/pin <msg_id>` | 방장/관리자 | P3 |
| `/search` | `/search <키워드>` | 전체 | P3 |
| `/me` | `/me <동작>` | 전체 | P3 |

---

## 6. 친구 관리 흐름

```
[친구 목록 화면]
   │ a              │ b              │ c              │ d
   ▼                ▼                ▼                ▼
[친구 추가]      [요청 목록]      [유저 검색]      [삭제/차단]
ID 입력          수락 / 거절      ID/닉 키워드     번호 선택
FRIEND_ADD_REQ   FRIEND_ACCEPT    USER_SEARCH      → 삭제: FRIEND_DELETE
                 FRIEND_REJECT                     → 차단: FRIEND_BLOCK
   │                │                │
   ▼                ▼                ▼
[결과 출력]      [목록 갱신]      [결과 목록]
                                 번호 선택
                                   │ 1           │ 2
                                   ▼             ▼
                                [친구 추가]   [DM 시작]

[친구 항목 선택 (번호)]
      │
      ▼
[항목 액션 메뉴]
  1.DM 보내기  2.채팅방 초대  3.프로필 보기
  4.친구 삭제  5.차단
```

---

## 7. 오픈채팅방 흐름

```
[오픈채팅 목록 화면]
   │ 번호         │ c           │ s
   ▼              ▼             ▼
[비번 확인]    [오픈채팅    [오픈채팅
   │           방 생성]      방 검색]
   ▼           방이름/인원  키워드 입력
[비번 입력?]   /비번/주제   → 결과 목록
  YES│  NO│       │             │
     ▼   ▼        ▼             ▼
[비번입력] →  [오픈닉      [번호 선택]
              입력 화면]     │
                │             ▼
                ▼         [비번 확인]
            [채팅방          │
             내부]          [오픈닉 입력]
                              │
                              ▼
                          [채팅방 내부]
```

---

## 8. DM 흐름

```
[DM 목록 화면]
   │ 번호          │ n (새 DM)
   ▼               ▼
[DM 대화 화면]  [새 DM 시작]
                1.친구 목록   2.ID 직접 입력
                   │              │
                   ▼              ▼
                [친구 선택]   [ID 입력]
                   │              │
                   └──────┬───────┘
                          ▼
                    [DM 대화 화면]
                    DM_HISTORY_REQ 자동 전송 (최근 50개)

[DM 대화 화면 내부]
   ├── 일반 텍스트 → DM_SEND|<to_id>:<content>
   ├── 0 또는 /leave → DM 목록 복귀
   └── 메시지 수신 → DM_RECV → 화면 출력
                           → DM_READ_NOTIFY 자동 전송 (읽음 처리)
```

---

## 9. 마이페이지 흐름

```
[마이페이지 화면]
   │1              │2               │3               │4
   ▼               ▼                ▼                ▼
[프로필 수정]  [비밀번호 변경]  [참여 채팅방    [최근 DM 목록]
닉/상태메시지  현재PW 확인      목록]           → DM 목록 화면
입력           새PW 입력        번호 선택
PROFILE_UPDATE PASS_CHANGE      → 채팅방 입장
   │               │
   ▼               ▼
[결과 출력]     [결과 출력]

[참여 채팅방 목록]
   │ 번호 선택
   ▼
[채팅방 비번 확인] → [채팅방 내부]
```

---

## 10. 설정 변경 흐름

```
[설정 화면]
   │1              │2              │3             │4
   ▼               ▼               ▼              ▼
[메시지 색상   [닉네임 색상   [테마 선택]   [타임스탬프
선택]          선택]          dark/light    형식 선택]
cyan/green     yellow/green                HH:MM
/yellow/red    /cyan/red                   HH:MM:SS
/magenta/white /magenta/white              MM-DD HH:MM
   │5              │6
   ▼               ▼
[DND 모드 토글] [온라인 상태
ON ↔ OFF        변경]
                online/busy
                /invisible

[0 저장 후 나가기]
   │
   SETTINGS_UPDATE 전송 → SETTINGS_UPDATE_RES|0
   ▼
[메인 로비]
```

---

## 11. 방장 권한 흐름

```
[채팅방 내부 — 방장/관리자]
      │ /members
      ▼
[멤버 목록 (방장 뷰)]
      │ 번호 선택
      ▼
[멤버 액션 메뉴]
   ├── 1.강퇴       → ROOM_KICK|room_id:target_id
   │                  → ROOM_KICKED_NOTIFY → 대상에게 전달
   ├── 2.공동방장부여→ ROOM_GRANT_ADMIN|room_id:target_id
   │                  → room_members.txt is_admin=1
   ├── 3.공동방장해제→ ROOM_REVOKE_ADMIN|room_id:target_id
   └── 4.DM 보내기  → DM 대화 화면

[방 삭제 흐름]
채팅방 내부 → /help → 채팅방 설정 메뉴 → 5.방 삭제
   │
   ▼
"정말 삭제하시겠습니까? (y/n):"
   │ y
   ▼
ROOM_DELETE → ROOM_DELETED_NOTIFY (모든 멤버)
모든 멤버: 채팅방에서 자동 퇴장 → 메인 로비
```

---

## 12. 비동기 수신 이벤트 처리 (RecvMsg 스레드)

```
[RecvMsg 스레드 루프]
      │
      ▼ recv() 수신
[패킷 타입 판별]
   │
   ├── ROOM_MSG_RECV      → current_room_id 일치? 화면 출력 : 알림 출력
   ├── DM_RECV            → DM 화면 중? 화면 출력 : 알림 출력 + 안읽음 증가
   ├── DM_READ_NOTIFY     → 해당 메시지에 [읽음] 표시
   ├── FRIEND_REQUEST_NOTIFY → "[친구 요청] ..." 알림 출력
   ├── FRIEND_ACCEPT_NOTIFY  → "[친구] ... 수락" 알림 출력
   ├── FRIEND_STATUS_CHANGE  → "[상태] ..." 알림 출력
   ├── ROOM_INVITE_NOTIFY    → "[초대] ..." 알림 출력 (P1)
   ├── ROOM_KICKED_NOTIFY    → "[강퇴] ..." 출력 + 채팅방 퇴장 처리
   ├── ROOM_DELETED_NOTIFY   → "[알림] 방 삭제" + 채팅방 퇴장 처리
   ├── ROOM_NOTICE           → 채팅방 공지 갱신
   ├── ROOM_PIN / MSG_PIN_NOTIFY → 핀 메시지 갱신
   ├── MSG_DELETED_NOTIFY    → "[삭제된 메시지]" 처리
   ├── MSG_EDITED_NOTIFY     → "내용 (수정됨)" 처리
   ├── TYPING_NOTIFY         → "... 님이 입력 중..." 표시 (P3)
   ├── NOTIFY                → MENTION / FRIEND_REQ / SERVER / DM 타입별 처리
   ├── SETTINGS_RES          → g_state 설정 갱신
   └── PONG                  → 연결 유지 확인 (P2)

알림 출력 공통 패턴:
   printf("\n[알림] %s\n> ", message);  // 현재 프롬프트 재출력
   fflush(stdout);
```

---

## 13. 상태별 화면 전환 규칙

| 현재 상태 | 이벤트 | 전환 |
|-----------|--------|------|
| 초기 화면 | 로그인 성공 | 메인 로비 |
| 초기 화면 | 회원가입 성공 | 초기 화면 (로그인 유도) |
| 메인 로비 | 로그아웃 | 초기 화면 |
| 채팅방 내부 | `/leave` | 메인 로비 |
| 채팅방 내부 | `ROOM_KICKED_NOTIFY` | 메인 로비 (강퇴 메시지 출력) |
| 채팅방 내부 | `ROOM_DELETED_NOTIFY` | 메인 로비 (방 삭제 메시지 출력) |
| 어디서든 | 서버 연결 끊김 | 오류 출력 후 종료 |
| DM 대화 | `0` 입력 | DM 목록 |
| 마이페이지 | 참여채팅방 번호 선택 | 채팅방 내부 |

---

## 14. 로그인 후 pending 처리 흐름

```
[로그인 성공 직후 서버 처리]
      │
      ├─ 1. pending 친구 요청 확인 (friends.txt, status=0, friend_id=나)
      │         → FRIEND_REQUEST_NOTIFY 순차 전송
      │
      ├─ 2. pending 채팅방 초대 확인 (room_invites.txt, status=0, invitee_id=나)
      │         → ROOM_INVITE_NOTIFY 순차 전송 (P1)
      │
      ├─ 3. online_status 갱신 (users.txt, → 1)
      │
      └─ 4. 친구들에게 FRIEND_STATUS_CHANGE 전송

[클라이언트 처리]
      │
      ▼
LOGIN_RES|0 수신
      │
      ▼
SETTINGS_REQ 자동 전송 → SETTINGS_RES 수신 → g_state 설정 동기화
      │
      ▼
FRIEND_LIST_REQ 전송 → FRIEND_LIST_RES 수신 → 친구 목록 캐시
      │
      ▼
[메인 로비 표시]
      │ (이후 비동기로)
      ├── FRIEND_REQUEST_NOTIFY 수신 → 알림 출력
      └── ROOM_INVITE_NOTIFY 수신 → 알림 출력
```
