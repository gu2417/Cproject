# settings 패킷

## 요청·응답

```
SETTINGS_GET
SETTINGS_GET_RES|0|<msg_color>|<nick_color>|<theme>|<ts_format>|<dnd>

SETTINGS_UPDATE|<msg_color>|<nick_color>|<theme>|<ts_format>|<dnd>
SETTINGS_UPDATE_RES|<code>
```

## 값 화이트리스트

| 필드 | 값 |
|------|----|
| `msg_color`, `nick_color` | `red`, `green`, `yellow`, `blue`, `magenta`, `cyan`, `white` |
| `theme` | `dark`, `light` |
| `ts_format` | `0`, `1`, `2` |
| `dnd` | `0`, `1` |

잘못된 값은 `INVALID_INPUT`(1).

## 상태 변경 알림

- `dnd` 는 `user_settings` 가 아닌 `users.dnd` 에 저장(실시간 브로드캐스트 필터링용).
- `dnd=1` 설정 시 친구들에게 특별한 알림을 보내지 않음(본인만 적용).

## 관련 DB

- `user_settings`, `users.dnd`. `query_catalog.md` — 별도 작성 없음(단순 UPDATE).
