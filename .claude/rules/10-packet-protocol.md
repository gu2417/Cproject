# Packet Protocol Rules

## 형식
```
<TYPE>|<PAYLOAD>\n
```
- 필드 구분자: `:` (payload 내부)
- 리스트 항목 구분: `;`
- **content-last 규칙**: 자유 텍스트 필드(content, status_msg, topic, notice, keyword)는 반드시 마지막에 위치. 파서가 `strtok(NULL, "")` 사용.

## 응답 코드 (패킷마다 다름!)
| 패킷 | 성공 코드 | 실패 코드 |
|------|----------|----------|
| LOGIN_RES | **0** = OK | 1=WRONG_ID, 2=WRONG_PW, 3=ALREADY_ONLINE |
| REGISTER_RES | **1** = OK | 2=DUPLICATE_ID, 3=기타 |
| ROOM_CREATE_RES | **1** = OK | 0=FAIL |
| ROOM_JOIN_RES | **0** = OK | 1=NOT_FOUND, 2=WRONG_PW, 3=FULL |

## 중요 패킷명
- `ROOM_KICKED_NOTIFY` (피동태) ← 존재함
- `ROOM_KICK_NOTIFY` ← 존재하지 않음!
- `MSG_EDITED_NOTIFY`, `MSG_DELETED_NOTIFY` ← 성공 알림
- `MSG_EDIT_RES`, `DM_SEND_RES` ← 존재하지 않음! (실패 시 NOTIFY|SERVER 사용)

## DM 식별
`room_id == 0` = DM. `MSG_DELETE|0:<msg_id>` 등.

## 리스트 패킷 count-first 규칙
FRIEND_LIST_RES, USER_SEARCH_RES, DM_HISTORY_RES, ROOM_HISTORY_RES 등은 첫 필드가 `<count>`.
