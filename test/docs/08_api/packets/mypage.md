# mypage 패킷

```
MYPAGE
MYPAGE_RES|0|<id>|<nick>|<status_msg>|<created_at>|<last_seen>|<msg_count>|<room_count>|<friend_count>

PROFILE_UPDATE|<nick>|<status_msg>
PROFILE_UPDATE_RES|<code>     # 3=nick 중복, 1=입력 규칙 위반

STATUS_UPDATE|<online_status>
STATUS_UPDATE_RES|0           # 1=online, 2=busy, 0=invisible
```

## 다른 유저 프로필 조회

```
USER_VIEW|<id>
USER_VIEW_RES|0|<id>|<nick>|<status_msg>|<online_status>
USER_VIEW_RES|4               # 없음
```
- 차단 관계면 `online_status` 는 0 으로 마스킹.

## 관련 DB

- `users` (nickname/status_msg/online_status), `query_catalog.md` "마이페이지" 절.
