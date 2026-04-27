# auth 패킷

## REGISTER (C→S)

```
REGISTER|<id>|<password>|<nickname>
```
- `id`: 3~20 영숫자/_. `nickname`: 1~20 utf8. `password`: 6~50.

응답:
```
REGISTER_RES|0                 # 성공
REGISTER_RES|3                 # 아이디 중복
REGISTER_RES|8                 # 닉네임 중복 (UNIQUE 제약 위반)
REGISTER_RES|1                 # 입력값 규칙 위반
```

## LOGIN (C→S)

```
LOGIN|<id>|<password>
```

응답:
```
LOGIN_RES|0|<id>|<nickname>
LOGIN_RES|2                    # 실패
LOGIN_RES|6                    # 이미 접속 중(중복 로그인 차단)
```
- 성공 후 서버는 다른 유저들에게 `FRIEND_ONLINE_NOTIFY|<id>` 발송(친구 대상).

## LOGOUT (C→S)

```
LOGOUT
```
- 응답 없음(소켓 close). 서버는 친구들에게 `FRIEND_OFFLINE_NOTIFY|<id>` 발송.

## PASSWORD_CHANGE (C→S)

```
PASSWORD_CHANGE|<old>|<new>
PASSWORD_CHANGE_RES|0
PASSWORD_CHANGE_RES|2          # old 불일치
```

## 관련 DB

- `users` (id, password_hash, online_status, last_seen)
- `query_catalog.md` "인증" 절 참조.
