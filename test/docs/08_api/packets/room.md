# room 패킷

## 요청

```
ROOM_CREATE|<name>|<topic>|<password>|<max_users>|<is_open>
ROOM_LIST                                  # 오픈방 목록
ROOM_JOIN|<room_id>|<password>
ROOM_LEAVE|<room_id>
ROOM_INFO|<room_id>
ROOM_MEMBERS|<room_id>
ROOM_INVITE|<room_id>|<target_id>         # 그룹방(is_open=0) 전용
ROOM_INVITE_ACCEPT|<room_id>              # 초대 수락
ROOM_INVITE_REJECT|<room_id>              # 초대 거절
ROOM_KICK|<room_id>|<target_id>           # 방장 전용
ROOM_TRANSFER|<room_id>|<target_id>       # 방장 위임
ROOM_NOTICE|<room_id>|<content>
ROOM_PIN|<room_id>|<msg_id>               # msg_id=0 → 핀 해제
ROOM_DELETE|<room_id>                     # 방장 전용
```

응답:

```
ROOM_CREATE_RES|0|<room_id>
ROOM_LIST_RES|0|<id>:<name>:<topic>:<max>:<cur>:<has_pw>;...
ROOM_JOIN_RES|0|<room_id>|<name>|<topic>|<notice>|<pinned_msg_id>
ROOM_JOIN_RES|2   # password mismatch
ROOM_JOIN_RES|6   # full
ROOM_JOIN_RES|7   # banned(차단된 유저) — v2.1
ROOM_LEAVE_RES|0
ROOM_INFO_RES|0|<room_id>|<name>|<topic>|<notice>|<owner_id>|<is_open>|<max>|<pinned_msg_id>
ROOM_MEMBERS_RES|0|<id>:<nick>:<online>:<open_nick>;...
ROOM_INVITE_RES|<code>
ROOM_INVITE_ACCEPT_RES|<code>             # 0=성공, 5=초대 없음
ROOM_INVITE_REJECT_RES|<code>             # 0=성공, 5=초대 없음
ROOM_KICK_RES|<code>
ROOM_TRANSFER_RES|<code>
ROOM_NOTICE_RES|<code>
ROOM_PIN_RES|<code>
ROOM_DELETE_RES|<code>
```

## 서버 알림

```
ROOM_MEMBER_JOINED_NOTIFY|<room_id>|<user_id>|<nick>
ROOM_MEMBER_LEFT_NOTIFY|<room_id>|<user_id>
ROOM_KICKED_NOTIFY|<room_id>|<user_id>        # 대상에게도 별도로
ROOM_NOTICE_NOTIFY|<room_id>|<content>
ROOM_PIN_NOTIFY|<room_id>|<msg_id>
ROOM_DELETED_NOTIFY|<room_id>
ROOM_OWNER_CHANGED_NOTIFY|<room_id>|<new_owner_id>
ROOM_INVITED_NOTIFY|<room_id>|<from_id>|<room_name>   # 초대받은 사람에게
```

## 권한 요약

| 작업 | 요구 권한 |
|------|-----------|
| 공지/핀/위임/삭제/강퇴/초대 | 방장 전용 (삭제·위임은 방장만) |
| 참여·퇴장 | 인증된 모든 유저 |
| 오픈방 참여 | 인증 + `is_open=1` |

자세한 매트릭스: [../05_security/authorization.md](../../05_security/authorization.md)

## 오프라인 초대 흐름

1. 방장이 `ROOM_INVITE|<room_id>|<target_id>` 전송
2. 서버가 대상 온라인 여부 확인:
   - 온라인: `ROOM_INVITED_NOTIFY` 즉시 전송
   - 오프라인: `room_invites` 테이블에 `status=0`(pending)으로 저장
3. 대상 로그인 시: pending 초대를 `ROOM_INVITED_NOTIFY`로 재전송
4. 대상이 수락: `ROOM_INVITE_ACCEPT|<room_id>` → `status=1`, `room_members`에 추가, `ROOM_MEMBER_JOINED_NOTIFY` 브로드캐스트
5. 대상이 거절: `ROOM_INVITE_REJECT|<room_id>` → `status=2`

## 관련 DB

- `rooms`, `room_members`, `room_invites`. `query_catalog.md` "방" 절.
