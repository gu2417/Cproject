# 권한 (Authorization)

## 1. 권한 주체

| 주체 | 판별 |
|------|------|
| **서버 관리자** (`is_admin=1`) | *(Out-of-Scope)* |
| **방장** (owner) | `rooms.owner_id == 요청자 id` |
| **방 관리자** (admin) | `room_members.is_admin=1 AND user_id=요청자 id` |
| **방 멤버** | `room_members` 에 (room_id, user_id) 존재 |
| **일반 유저** | 인증된 세션 |
| **미인증** | `LOGIN`/`REGISTER`/`PING` 만 허용 |

## 2. 행위별 매트릭스

| 행위 | 멤버 | 방 관리자 | 방장 | 서버 관리자 |
|------|:----:|:--------:|:----:|:----------:|
| 방 메시지 전송 | ✅ | ✅ | ✅ | ✅ |
| 친구 초대 | ✅ | ✅ | ✅ | ✅ |
| 공지 등록 / 수정 | ❌ | ✅ | ✅ | ✅ |
| 핀 설정 / 해제 | ❌ | ✅ | ✅ | ✅ |
| 멤버 강퇴 | ❌ | ✅ | ✅ | ✅ |
| 공동 방장 부여/해제 | ❌ | ❌ | ✅ | ✅ |
| 방 삭제 | ❌ | ❌ | ✅ | ✅ |
| 자기 메시지 삭제/수정 | ✅ | ✅ | ✅ | ✅ |
| **타인 메시지** 삭제 | ❌ | ❌ | ❌ | ✅ |
| 서버 공지 브로드캐스트 | ❌ | ❌ | ❌ | ✅ |
| 유저 강제 로그아웃 | ❌ | ❌ | ❌ | ✅ |
| 유저 목록 조회(전체) | ❌ | ❌ | ❌ | ✅ |
| 방 강제 삭제 | ❌ | ❌ | ❌ | ✅ |

## 3. 검사 위치

권한 검사는 **각 핸들러 진입 직후** 수행한다. 클라이언트 메뉴 노출 여부와 무관하게 서버가 최종 판단.

```c
int handle_room_set_notice(ClientSession *s, const char *payload) {
    /* 파싱 */
    if (!room_is_admin_or_owner(s->db, room_id, s->user_id))
        return send_res(s, "ROOM_SET_NOTICE_RES", RES_FORBIDDEN);
    /* ... */
}
```

## 4. 권한 오류 응답

- 공통 코드: `RES_FORBIDDEN=10`.
- 클라이언트 표시: "권한이 없습니다."

