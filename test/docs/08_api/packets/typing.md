# typing 패킷

타이핑 인디케이터 — DB 저장 없음, 메모리 전용.

## 요청

```
TYPING|<scope>|<target>          # scope: "room" | "dm",  target: room_id 또는 peer_id
```
- 클라는 3초마다 재전송(사용자가 키 입력 중일 때).

## 서버 알림

```
TYPING_NOTIFY|<scope>|<context>|<from_id>|<from_nick>
# scope="room": context=room_id, 해당 방 멤버 전원에게
# scope="dm":   context=from_id, 상대(peer)에게만
```

## 규칙

- 서버는 5초 동안 후속 `TYPING` 이 없으면 자동으로 `TYPING_STOP_NOTIFY` 발송:
  ```
  TYPING_STOP_NOTIFY|<scope>|<context>|<from_id>
  ```
- 저사양 절약: `TYPING_NOTIFY` 는 rate-limit(동일 from→동일 context 에 대해 1초에 최대 1회).
- dnd=1 유저는 TYPING_NOTIFY 수신 제외.
