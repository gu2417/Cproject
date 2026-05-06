# 개발 단계

## Phase 0 — MVP (필수)

**목표**: 동작하는 최소 구현. 2개 클라이언트로 채팅방 생성-입장-메시지 교환 가능.

### 구현 범위

| FR | 기능 | 파일 |
|----|------|------|
| FR-A01 | 회원가입 (users.txt 저장) | auth.c, file_io.c |
| FR-A02 | 로그인 (SHA-256 해시 비교) | auth.c |
| FR-A03 (P0) | 로그아웃 (소켓 종료) | client_handler.c, leftClient() |
| FR-G01 | 그룹채팅방 생성 | room.c |
| FR-G03 | 메시지 브로드캐스트 (전체 전송) | broadcast.c |
| FR-G05 | 채팅방 나가기 | room.c |
| FR-O01 | 오픈채팅방 생성 (is_open=1) | room.c |
| FR-O02 | 오픈채팅방 목록 조회 | room.c |
| FR-O04 | 오픈채팅방 자유 참여 | room.c |
| ROOM_JOIN | 방 입장 (비밀번호 검증 포함) | room.c |

### P0 브로드캐스트 방식

```c
/* P0: 전체 접속자에게 브로드캐스트, 클라이언트가 room_id로 필터링 */
void broadcast_to_all(const char *packet) {
    WaitForSingleObject(g_sessions_mutex, INFINITE);
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (g_sessions[i].active)
            send(g_sessions[i].fd, packet, (int)strlen(packet), 0);
    ReleaseMutex(g_sessions_mutex);
}
```

### 저장 파일 (P0)

- `users.txt` — 로그인/회원가입
- `rooms.txt` — 방 생성
- `room_members.txt` — 방 입장/퇴장

### 검증 기준

```
터미널 1: server.exe 실행
터미널 2: client.exe → 회원가입(alice) → 로그인 → 방 생성(개발팀)
터미널 3: client.exe → 회원가입(bob) → 로그인 → 방 입장(개발팀) → 메시지 전송
터미널 2: alice 화면에 bob의 메시지 출력 확인
```

---

## Phase 1 — 소셜 기능 (중요)

**목표**: 친구·DM·초대·히스토리·마이페이지·정식 로그아웃 구현.

### 구현 범위

| FR | 기능 |
|----|------|
| FR-A03 (P1) | LOGOUT_REQ 패킷, last_seen 갱신 |
| FR-A04 | 프로필 수정 (PROFILE_UPDATE) |
| FR-A05 | 비밀번호 변경 (PASS_CHANGE) |
| FR-A06 | 마지막 접속 시간 표시 |
| FR-F01~F07 | 친구 전체 (추가/수락/거절/삭제/차단/목록/검색) |
| FR-D01~D05 | DM 전체 (개설/전송/읽음/히스토리/안읽은수) |
| FR-G02 | 멤버 초대 (ROOM_INVITE, room_invites.txt) |
| FR-G09 | 메시지 히스토리 (ROOM_HISTORY_REQ) |
| FR-P01~P06 | 마이페이지 전체 |
| ALREADY_ONLINE | 중복 로그인 차단 (LOGIN_RES\|3) |

### 추가 저장 파일 (P1)

- `friends.txt` — 친구 관계
- `messages.txt` — DM 및 채팅 메시지
- `dm_reads.txt` — DM 읽음 상태
- `room_invites.txt` — 방 초대 기록

### P1 브로드캐스트 방식 전환

```c
/* P1부터: 방별 격리 브로드캐스트 */
void broadcast_to_room(int room_id, const char *packet) {
    RoomInfo *r = find_room_by_id(room_id);
    WaitForSingleObject(g_sessions_mutex, INFINITE);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!g_sessions[i].active) continue;
        for (int j = 0; j < r->member_count; j++) {
            if (strcmp(r->member_ids[j], g_sessions[i].user_id) == 0) {
                send(g_sessions[i].fd, packet, (int)strlen(packet), 0);
                break;
            }
        }
    }
    ReleaseMutex(g_sessions_mutex);
}
```

---

## Phase 2 — 완성도 향상 (권장)

**목표**: 메시지 편집, 방장 권한, 알림, 커스터마이징, 연결 유지.

### 구현 범위

| FR | 기능 |
|----|------|
| FR-M01 | 귓속말 (/w) |
| FR-M02 | 메시지 삭제 (/del) |
| FR-M03 | 메시지 수정 (/edit, 5분 이내) |
| FR-M07 | 시스템 메시지 (입/퇴장, 강퇴, 공지) |
| FR-M09 | 타임스탬프 형식 설정 |
| FR-G06 | 방장 권한 (강퇴/삭제/공지/핀) |
| FR-G07 | 공동 방장 (/grant, /revoke) |
| FR-G08 | 공지사항 등록/조회 |
| FR-G10 | 멤버 목록 (/members) |
| FR-C01~C07 | 커스터마이징 전체 (색상/테마/타임스탬프/상태/오픈닉) |
| FR-N01 | 메시지 알림 (다른 방 수신 시 콘솔 알림) |
| FR-N02 | 친구 요청 알림 (로그인 시 + 실시간) |
| PING/PONG | 연결 유지 |

### 추가 저장 파일 (P2)

- `user_settings.txt` — 커스터마이징 설정

---

## Phase 3 — 풍부한 경험 (선택)

**목표**: 답장, 이모티콘, 검색, 핀, /me, 타이핑, DND, 멘션.

### 구현 범위

| FR | 기능 |
|----|------|
| FR-M04 | 답장/인용 (/reply) |
| FR-M06 | 이모티콘 변환 (`:smile:` → `(^_^)`) |
| FR-M08 | 메시지 검색 (/search) |
| FR-M10 | 핀 메시지 (/pin) |
| FR-M11 | /me 액션 |
| FR-O03 | 오픈채팅 방 검색 |
| FR-N03 | 멘션 알림 (@닉네임) |
| FR-N04 | DND 모드 |
| FR-N05 | 타이핑 표시 (TYPING_START/STOP) |
| FR-N06 | 방 알림 무음 (ROOM_MUTE_TOGGLE) |

---

## Out-of-Scope (구현 없음)

| FR | 이유 |
|----|------|
| FR-M05 리액션 | v2.1 이후 별도 구현 |
| FR-ADM01~ADM05 관리자 기능 | 현재 범위 제외 |

---

## 단계별 체크리스트

### Phase 0 완료 기준

- [ ] 서버 실행 시 포트 55555 listen 확인
- [ ] 클라이언트 회원가입 → `users.txt` 저장 확인
- [ ] 클라이언트 로그인 성공/실패 응답 확인
- [ ] 방 생성 → `rooms.txt` 저장 확인
- [ ] 방 입장 → `room_members.txt` 저장 확인
- [ ] 2개 클라이언트 간 메시지 교환 확인
- [ ] 클라이언트 비정상 종료 시 서버 크래시 없음 확인

### Phase 1 완료 기준

- [ ] 친구 추가/수락/목록 동작 확인
- [ ] DM 전송/수신/히스토리 동작 확인
- [ ] 방 초대 및 오프라인 초대 보관 확인
- [ ] 마이페이지 통계 정확성 확인
- [ ] 중복 로그인 차단 확인
- [ ] 서버 재시작 후 데이터 유지 확인

### Phase 2 완료 기준

- [ ] 메시지 삭제/수정 (5분 이내) 동작 확인
- [ ] 방장 강퇴/공지/공동방장 동작 확인
- [ ] 설정 저장/로드 확인
- [ ] PING/PONG 연결 유지 확인

### Phase 3 완료 기준

- [ ] 이모티콘 변환 확인 (`:smile:` → `(^_^)`)
- [ ] 타이핑 표시 동작 확인
- [ ] 멘션 알림 DND 무시 여부 확인
