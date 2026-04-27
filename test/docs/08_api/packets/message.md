# message 패킷 (방 메시지)

## 요청

```
CHAT|<room_id>|<content>                           # 일반
REPLY|<room_id>|<reply_to_msg_id>|<content>
WHISPER|<room_id>|<target_id>|<content>            # 귓속말
ME|<room_id>|<action>                              # "/me 춤춘다" → msg_type=3
MSG_EDIT|<msg_id>|<content>                        # 5분 내, 본인
MSG_DELETE|<msg_id>
MSG_SEARCH|<room_id>|<keyword>
HISTORY|<room_id>                                  # 최근 100
```

응답:

```
CHAT_RES|0|<msg_id>|<created_at>
REPLY_RES|0|<msg_id>
WHISPER_RES|<code>
ME_RES|<code>
MSG_EDIT_RES|<code>            # 5=권한, 6=시한초과/삭제됨
MSG_DELETE_RES|<code>
MSG_SEARCH_RES|0|<id>:<from_id>:<created_at>:<content>;...
HISTORY_RES|0|<id>:<from_id>:<reply_to>:<msg_type>:<is_deleted>:<created_at>:<edited_at>:<content>;...
```

## 서버 알림

```
CHAT_NOTIFY|<room_id>|<from_id>|<from_nick>|<msg_id>|<content>
REPLY_NOTIFY|<room_id>|<from_id>|<from_nick>|<msg_id>|<reply_to>|<content>
WHISPER_NOTIFY|<room_id>|<from_id>|<from_nick>|<to_id>|<content>   # from,to 둘에게만
ME_NOTIFY|<room_id>|<from_id>|<from_nick>|<msg_id>|<action>
MSG_EDITED_NOTIFY|<room_id>|<msg_id>|<content>
MSG_DELETED_NOTIFY|<room_id>|<msg_id>
```

## content 제약

- 길이 1~500자.
- WHISPER 는 저장하되(`msg_type=2`, `to_id` 설정) 브로드캐스트 대상은 보낸 사람 + 대상만.
- REPLY 의 `reply_to_msg_id` 가 같은 room_id 가 아니면 `INVALID_INPUT`.

## 관련 DB

- `messages`. `query_catalog.md` "메시지" 절.
