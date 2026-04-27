# friend 패킷

## 요청

```
FRIEND_ADD|<target_id>
FRIEND_ACCEPT|<requester_id>
FRIEND_REJECT|<requester_id>
FRIEND_REMOVE|<friend_id>
FRIEND_BLOCK|<target_id>
FRIEND_UNBLOCK|<target_id>
FRIEND_LIST
FRIEND_SEARCH|<keyword>
```

응답:

```
FRIEND_ADD_RES|<code>
FRIEND_ACCEPT_RES|<code>
FRIEND_REJECT_RES|<code>
FRIEND_REMOVE_RES|<code>
FRIEND_BLOCK_RES|<code>
FRIEND_UNBLOCK_RES|<code>

# 목록 (accepted)
FRIEND_LIST_RES|0|<id>:<nick>:<online>:<status_msg>;...

# 받은 pending
FRIEND_PENDING_RES|0|<id>:<nick>;...

# 검색
FRIEND_SEARCH_RES|0|<id>:<nick>:<status_msg>;...
```

## 서버 알림 (상대방에게 push)

```
FRIEND_REQUEST_NOTIFY|<from_id>|<from_nick>
FRIEND_ACCEPT_NOTIFY|<from_id>|<from_nick>
FRIEND_ONLINE_NOTIFY|<id>
FRIEND_OFFLINE_NOTIFY|<id>
```

## 에러

| code | 상황 |
|-----:|------|
| 3 | 이미 친구 / 이미 pending |
| 4 | 대상 유저 없음 |
| 7 | 차단 관계 |
| 1 | 자기 자신을 친구 추가 시도 |

## 관련 DB

- `friends` 테이블. `query_catalog.md` "친구" 절.
