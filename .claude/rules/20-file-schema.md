# File Schema Rules

## 필드 구분자
- 필드 사이: `//` (정확히 슬래시 2개)
- 빈 필드: 양쪽 `//` 사이에 아무것도 없음 → 결과는 정확히 `////` (슬래시 4개)
- ★ 5슬래시 (`/////`) 작성 금지. 빈 필드 자리에 `/`가 들어가는 버그.

## fprintf 패턴
✅ `fprintf(fp, "%s//%s//%s//%s\n", id, hash, nick, "")`
❌ `fprintf(fp, "%s//%s//%s/////%s\n", ...)` // 5슬래시 버그

## NULL 체크
모든 `fopen` 후:
```c
if (!fp) return;
```
빈 파일이어도 서버는 정상 시작해야 한다.

## 9개 데이터 파일
users.txt, rooms.txt, messages.txt, friends.txt, room_members.txt,
dm_reads.txt, room_invites.txt, user_settings.txt, room_reads.txt
