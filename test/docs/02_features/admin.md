# 관리자 기능 (FR-ADM) — **Out-of-Scope**

> 관리자(전역 서버 관리자) 기능은 이 프로젝트 구현 범위에 포함되지 않습니다.  
> `users.is_admin` 컬럼 및 `ADMIN_*` 패킷은 사용하지 않습니다.

- 방장(room moderator) 권한은 `room_members.is_admin` 으로 별도 관리됩니다.  
  → [`02_features/room.md`](room.md), [`08_api/packets/room.md`](../08_api/packets/room.md) 참조.
