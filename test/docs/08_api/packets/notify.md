# notify 패킷 (일반 공지)

시스템/서버 이벤트 전용. 보통 다른 패킷(`FRIEND_*_NOTIFY`, `ROOM_*_NOTIFY`, `DM_NEW_NOTIFY` 등) 으로 커버되며, 범용 배너는 아래 타입 사용.

## 서버 → 클라

```
SYSTEM_NOTIFY|<level>|<content>
# level: 0=info, 1=warn, 2=urgent
# 클라는 상단 배너 또는 mainbar 에 표시
```

예:
```
SYSTEM_NOTIFY|2|서버 점검을 위해 5분 후 종료됩니다.
SYSTEM_NOTIFY|0|환영합니다, alice 님!
```

## 특수

- `dnd=1` 유저에게도 level=2 는 표시(중요 공지).

> **Out-of-Scope**: `ADMIN_BROADCAST` 패킷(관리자 전역 공지)은 구현 범위 외입니다.

## 관련 문서

- DND 동작: [../../02_features/notification.md](../../02_features/notification.md)
- 렌더링: [../../06_ui_ux/screens/main.md](../../06_ui_ux/screens/main.md) 의 배너 영역.
