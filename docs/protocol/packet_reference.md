# 패킷 레퍼런스

전체 패킷 정의. `C→S` = 클라이언트→서버, `S→C` = 서버→클라이언트.

---

## 1. 인증 (Authentication)

| 방향 | 패킷 타입 | PAYLOAD 형식 | 설명 |
|------|-----------|-------------|------|
| C→S | `LOGIN_REQ` | `<id>:<pass>` | 로그인 요청 |
| S→C | `LOGIN_RES` | `<code>` | 0=OK, 1=WRONG_ID, 2=WRONG_PW, 3=ALREADY_ONLINE(P1) |
| C→S | `REGISTER_REQ` | `<id>:<pass>[:<nick>[:<status>]]` | 회원가입 요청 |
| S→C | `REGISTER_RES` | `<code>` | 1=OK, 2=DUPLICATE_ID |
| C→S | `LOGOUT_REQ` | (없음) | 로그아웃 요청 (P1 이후) |
| S→C | `LOGOUT_RES` | `OK` | 로그아웃 응답 (P1 이후) |

---

## 2. 친구 관리 (Friend)

| 방향 | 패킷 타입 | PAYLOAD 형식 | 설명 |
|------|-----------|-------------|------|
| C→S | `FRIEND_ADD_REQ` | `<target_id>` | 친구 추가 요청 |
| S→C | `FRIEND_ADD_RES` | `<code>` | 0=SENT, 1=NOT_FOUND, 2=BLOCKED, 3=ALREADY_FRIEND |
| S→C | `FRIEND_REQUEST_NOTIFY` | `<from_id>:<from_nick>` | 친구 요청 수신 알림 |
| C→S | `FRIEND_ACCEPT` | `<from_id>` | 친구 요청 수락 |
| S→C | `FRIEND_ACCEPT_NOTIFY` | `<user_id>:<nick>` | 수락 알림 → 요청 송신자에게 전달 |
| C→S | `FRIEND_REJECT` | `<from_id>` | 친구 요청 거절 |
| C→S | `FRIEND_DELETE` | `<target_id>` | 친구 삭제 |
| C→S | `FRIEND_BLOCK` | `<target_id>` | 친구 차단 |
| C→S | `FRIEND_LIST_REQ` | (없음) | 친구 목록 요청 |
| S→C | `FRIEND_LIST_RES` | `<count>:<id>:<nick>:<status>:<status_msg>;<id>:...` | 친구 목록 응답 (count 규칙 적용) |
| S→C | `FRIEND_STATUS_CHANGE` | `<id>:<nick>:<status>` | 친구 접속/해제 실시간 알림 |
| C→S | `USER_SEARCH` | `<keyword>` | 유저 검색 |
| S→C | `USER_SEARCH_RES` | `<count>:<id>:<nick>:<status_msg>;<id>:...` | 유저 검색 결과 (count 규칙 적용) |

---

## 3. DM (Direct Message)

| 방향 | 패킷 타입 | PAYLOAD 형식 | 설명 |
|------|-----------|-------------|------|
| C→S | `DM_SEND` | `<to_id>:<content>` | DM 전송 (content-last) |
| S→C | `DM_RECV` | `<from_id>:<from_nick>:<timestamp>:<msg_id>:<content>` | DM 수신 (content-last) |
| S→C | `DM_READ_NOTIFY` | `<reader_id>` | DM 읽음 알림 → 원래 송신자에게 전달 |
| C→S | `DM_HISTORY_REQ` | `<with_id>:<count>` | DM 히스토리 요청 |
| S→C | `DM_HISTORY_RES` | `<count>:<msg_id>:<from_id>:<timestamp>:<read>:<content>;<msg_id>:...` | DM 히스토리 응답 (count 규칙, content-last) |
| C→S | `DM_LIST_REQ` | (없음) | DM 목록 요청 (P1, FR-P04) |
| S→C | `DM_LIST_RES` | `<count>:<partner_id>:<partner_nick>:<timestamp>:<unread>:<last_msg>;<partner_id>:...` | DM 목록 응답 (count 규칙, last_msg는 content-last) |

---

## 4. 그룹 채팅방 / 오픈채팅 (Room)

| 방향 | 패킷 타입 | PAYLOAD 형식 | 설명 |
|------|-----------|-------------|------|
| C→S | `ROOM_CREATE` | `<name>:<max_users>[:<is_open>[:<password>[:<topic>]]]` | 방 생성 (topic은 content-last) |
| S→C | `ROOM_CREATE_RES` | `<code>:<room_id>` | 1=OK/room_id, 0=FAIL/room_id=0 |
| C→S | `ROOM_LIST_REQ` | `[<type>]` | 방 목록 요청 (type: group\|open, 생략 시 전체) |
| S→C | `ROOM_LIST_RES` | `<id>:<name>:<cur>:<max>:<has_pw>:<is_open>:<topic>;<id>:...` | 방 목록 응답 (topic은 content-last) |
| C→S | `ROOM_SEARCH` | `<type>:<keyword>` | 방 검색 (keyword는 content-last) |
| S→C | `ROOM_SEARCH_RES` | `<id>:<name>:<cur>:<max>:<has_pw>:<is_open>:<topic>;<id>:...` | 방 검색 결과 |
| C→S | `ROOM_JOIN` | `<room_id>[:<password>]` | 방 입장 |
| S→C | `ROOM_JOIN_RES` | `<code>:<room_id>:<room_name>` | 1=OK, 0=FAIL(방없음/인원초과/비번불일치) |
| S→C | `ROOM_NOTICE` | `<room_id>:<notice_text>` | 공지사항 (입장 시 서버 push) |
| S→C | `ROOM_PIN` | `<room_id>:<msg_id>:<from_nick>:<content>` | 핀 메시지 (입장 시 초기 상태, content-last) |
| C→S | `ROOM_MSG` | `<room_id>:<content>` | 채팅방 메시지 전송 (content-last) |
| S→C | `ROOM_MSG_RECV` | `<room_id>:<from_nick>:<timestamp>:<msg_id>:<reply_to_id>:<msg_type>:<content>` | 메시지 수신 브로드캐스트 (content-last) |
| C→S | `ROOM_HISTORY_REQ` | `<room_id>:<count>` | 히스토리 요청 (P1, FR-G09) |
| S→C | `ROOM_HISTORY_RES` | `<count>:<msg_id>:<from_nick>:<timestamp>:<reply_to_id>:<msg_type>:<content>;<msg_id>:...` | 히스토리 응답 (count 규칙, content-last) |
| C→S | `ROOM_LEAVE` | `<room_id>` | 채팅방 나가기 (응답 없음) |
| C→S | `ROOM_INVITE` | `<room_id>:<target_id>` | 멤버 초대 |
| S→C | `ROOM_INVITE_RES` | `<code>` | 0=SENT, 1=NOT_FOUND, 2=ALREADY_MEMBER, 3=ROOM_FULL |
| S→C | `ROOM_INVITE_NOTIFY` | `<room_id>:<room_name>:<inviter_nick>` | 초대 알림 → 초대받은 사용자에게 (P1) |
| C→S | `ROOM_KICK` | `<room_id>:<target_id>` | 멤버 강퇴 (방장/관리자 전용) |
| S→C | `ROOM_KICKED_NOTIFY` | `<room_id>:<by_nick>` | 강퇴 알림 → 강퇴당한 사용자에게 (P2) |
| C→S | `ROOM_DELETE` | `<room_id>` | 방 삭제 (방장 전용, P2) |
| S→C | `ROOM_DELETED_NOTIFY` | `<room_id>` | 방 삭제 알림 → 전체 멤버에게 (P2) |
| C→S | `ROOM_SET_NOTICE` | `<room_id>:<notice>` | 공지 등록 (방장/관리자, content-last) |
| C→S | `ROOM_GRANT_ADMIN` | `<room_id>:<target_id>` | 공동 방장 부여 (방장 전용) |
| C→S | `ROOM_REVOKE_ADMIN` | `<room_id>:<target_id>` | 공동 방장 해제 (방장 전용) |
| C→S | `ROOM_MEMBERS_REQ` | `<room_id>` | 멤버 목록 요청 |
| S→C | `ROOM_MEMBERS_RES` | `<room_id>:<id>:<nick>:<is_admin>:<online>;<id>:...` | 멤버 목록 응답 |

---

## 5. 메시지 부가기능 (Message)

| 방향 | 패킷 타입 | PAYLOAD 형식 | 설명 |
|------|-----------|-------------|------|
| C→S | `WHISPER` | `<to_nick>:<content>` | 귓속말 전송 (content-last) |
| S→C | `WHISPER_RECV` | `<from_nick>:<timestamp>:<content>` | 귓속말 수신 (content-last) |
| C→S | `MSG_DELETE` | `<room_id>:<msg_id>` | 메시지 삭제 (DM은 `room_id=0`) |
| S→C | `MSG_DELETED_NOTIFY` | `<room_id>:<msg_id>` | 삭제 알림. DM(`room_id=0`)은 두 당사자에게만, 그룹은 방 브로드캐스트 |
| C→S | `MSG_EDIT` | `<room_id>:<msg_id>:<new_content>` | 메시지 수정 (5분 이내, content-last, DM은 `room_id=0`) |
| S→C | `MSG_EDITED_NOTIFY` | `<room_id>:<msg_id>:<new_content>` | 수정 알림. DM은 두 당사자에게만, 그룹은 방 브로드캐스트 (content-last) |
| C→S | `MSG_REPLY` | `<room_id>:<reply_to_id>:<content>` | 답장 (content-last) |
| S→C | `ROOM_MSG_RECV` | (위 Room 항목 참조) | 답장도 ROOM_MSG_RECV로 브로드캐스트 (reply_to_id 포함) |
| C→S | `MSG_SEARCH` | `<room_id>:<keyword>` | 메시지 검색 (keyword는 content-last) |
| S→C | `MSG_SEARCH_RES` | `<count>:<msg_id>:<from_nick>:<timestamp>:<content>;<msg_id>:...` | 검색 결과 (count 규칙, content-last) |
| C→S | `MSG_PIN` | `<room_id>:<msg_id>` | 핀 메시지 설정 (방장/관리자 전용) |
| S→C | `MSG_PIN_NOTIFY` | `<room_id>:<msg_id>:<from_nick>:<content_preview>` | 핀 설정 알림 브로드캐스트 (content-last) |

> **Out-of-Scope**: `MSG_REACT` / `MSG_REACT_NOTIFY` — FR-M05, v2.1 이후 구현

---

## 6. 커스터마이징 / 설정 (Settings)

| 방향 | 패킷 타입 | PAYLOAD 형식 | 설명 |
|------|-----------|-------------|------|
| C→S | `SETTINGS_REQ` | (없음) | 설정 조회 요청 |
| S→C | `SETTINGS_RES` | `<msg_color>:<nick_color>:<theme>:<ts_format>:<dnd>` | 설정 응답 |
| C→S | `SETTINGS_UPDATE` | `<msg_color>:<nick_color>:<theme>:<ts_format>:<dnd>` | 설정 변경 |
| S→C | `SETTINGS_UPDATE_RES` | `<code>` | 0=OK, 1=FAIL |
| C→S | `STATUS_CHANGE` | `<status>` | 상태 변경 (online\|busy\|invisible) |
| C→S | `PROFILE_UPDATE` | `<nickname>:<status_msg>` | 프로필 수정 (status_msg는 content-last) |
| S→C | `PROFILE_UPDATE_RES` | `<code>` | 0=OK, 1=DUPLICATE_NICK |
| C→S | `PASS_CHANGE` | `<old_pass>:<new_pass>` | 비밀번호 변경 |
| S→C | `PASS_CHANGE_RES` | `<code>` | 0=OK, 1=WRONG_PW |
| C→S | `ROOM_SET_OPEN_NICK` | `<room_id>:<nick>` | 오픈채팅 닉네임 설정 |
| S→C | `ROOM_SET_OPEN_NICK_RES` | `<code>` | 0=OK, 1=FAIL |
| C→S | `ROOM_MUTE_TOGGLE` | `<room_id>` | 방 알림 무음 토글 (P3) |
| S→C | `ROOM_MUTE_TOGGLE_RES` | `<room_id>:<is_muted>` | 변경 후 현재 상태 (0=unmuted, 1=muted) |

---

## 7. 마이페이지 (MyPage)

| 방향 | 패킷 타입 | PAYLOAD 형식 | 설명 |
|------|-----------|-------------|------|
| C→S | `MYPAGE_REQ` | (없음) | 마이페이지 요청 |
| S→C | `MYPAGE_RES` | `<id>:<nick>:<created_at>:<last_seen>:<msg_count>:<room_count>:<friend_count>:<status_msg>` | 마이페이지 응답 (status_msg는 content-last) |
| C→S | `MY_ROOMS_REQ` | (없음) | 내 채팅방 목록 요청 (P1, FR-P03) |
| S→C | `MY_ROOMS_RES` | `<id>:<name>:<cur>:<max>:<has_pw>:<is_open>:<topic>;<id>:...` | 내 채팅방 목록 (ROOM_LIST_RES와 동일 포맷) |

---

## 8. 타이핑 인디케이터 (Typing)

| 방향 | 패킷 타입 | PAYLOAD 형식 | 설명 |
|------|-----------|-------------|------|
| C→S | `TYPING_START` | `<room_id>` | 타이핑 시작 알림 (P3) |
| C→S | `TYPING_STOP` | `<room_id>` | 타이핑 종료 알림 (P3) |
| S→C | `TYPING_NOTIFY` | `<room_id>:<nick>:<is_typing>` | 타이핑 상태 브로드캐스트 (0 또는 1) |

---

## 9. 알림 (Notification)

| 방향 | 패킷 타입 | PAYLOAD 형식 | 설명 |
|------|-----------|-------------|------|
| S→C | `NOTIFY` | `<type>:<content>` | 범용 알림 (content-last) |

**type 값**:

| type | 설명 |
|------|------|
| `MENTION` | @멘션 알림 |
| `FRIEND_REQ` | 친구 요청 알림 |
| `SERVER` | 서버 공지 |
| `DM` | DM 수신 알림 |

---

## 10. 연결 유지 (Keep-Alive)

| 방향 | 패킷 타입 | PAYLOAD 형식 | 설명 |
|------|-----------|-------------|------|
| C→S | `PING` | (없음) | 연결 확인 (P2 이후) |
| S→C | `PONG` | (없음) | 연결 확인 응답 (P2 이후) |

---

## 11. 응답 코드 요약

| 코드 | 의미 |
|------|------|
| 0 | OK (LOGIN_RES), 또는 FAIL (ROOM_CREATE_RES) — 패킷마다 의미 다름 |
| 1 | OK (REGISTER_RES, ROOM_CREATE_RES), 또는 WRONG_ID (LOGIN_RES) |
| 2 | DUPLICATE_ID, WRONG_PW, BLOCKED, ALREADY_MEMBER |
| 3 | ALREADY_ONLINE, ALREADY_FRIEND, ROOM_FULL |

> 각 패킷별 정확한 코드 의미는 위 표 참조.
