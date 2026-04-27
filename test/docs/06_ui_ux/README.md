# 06 · UI / UX

Google Workspace(Chat, Gmail) · Material 3 · KakaoTalk PC(2025) 레퍼런스 기반의 **GTK4 GUI 디자인**.
핵심 원칙: **여백 중심 · 카드형 리스트 · 대비 낮은 중립 톤 + 1색 액센트 · 사이드바 레이아웃**.

---

## 문서 맵

| 문서 | 내용 |
|------|------|
| [design_tokens.md](./design_tokens.md) | 색·간격·타이포·아이콘·Z레이어·타이밍·폭 규칙 |
| [components.md](./components.md) | 재사용 컴포넌트 22종 (아바타·뱃지·카드·오버레이·자동완성 등) |
| [screen_flow.md](./screen_flow.md) | 화면 상태 전이도 + 13개 서브 플로우 상세 |
| [interaction_patterns.md](./interaction_patterns.md) | 로딩·빈 상태·오류·재연결·자동완성·친구요청 패턴 |
| [screens/login.md](./screens/login.md) | 로그인 · 회원가입 |
| [screens/main.md](./screens/main.md) | 메인 (사이드바 + 탭 리스트 + 방 생성 위저드) |
| [screens/chat.md](./screens/chat.md) | 채팅 (말풍선 · 오버레이 · 귓속말 · me액션 · DM모드) |
| [screens/open_chat.md](./screens/open_chat.md) | 오픈채팅 탐색 · 참여 · 비밀번호 방 · 방 생성 |
| [screens/mypage.md](./screens/mypage.md) | 마이페이지 (프로필 카드 · 인라인 편집) |
| [screens/settings.md](./screens/settings.md) | 설정 (세그먼트 컨트롤 · 색상 스와치 · 토글) |
| [screens/user_profile.md](./screens/user_profile.md) | 유저 프로필 팝오버 (프로필 카드 · 친구 추가 · 차단) |
| [screens/room_settings.md](./screens/room_settings.md) | 방 정보 및 관리 패널 |
| [screens/notifications.md](./screens/notifications.md) | 알림 히스토리 패널 |
| [screens/admin.md](./screens/admin.md) | 관리자 패널 *(범위 외)* |
| [commands.md](./commands.md) | GTK4 GUI 인터랙션 레퍼런스 (버튼, 메뉴, 단축키) |
| [low_spec_rendering.md](./low_spec_rendering.md) | GTK4 클라이언트 성능 가이드 |

---

## 화면 구성 요약

```
LOGIN ──→ MAIN ──→ CHAT (그룹방 / DM / 오픈채팅)
            │         ├──→ USER_PROFILE (팝오버)
            │         └──→ ROOM_SETTINGS (모달)
            ├──→ OPEN_CHAT (탐색 · 참여)
            ├──→ MYPAGE
            ├──→ SETTINGS
            ├──→ NOTIFICATIONS (팝오버)
            └──→ ADMIN (admin 전용, 범위 외)
```

---

## 컴포넌트 목록

| # | 이름 | 용도 |
|---|------|------|
| 1 | Avatar | 이니셜 원형 아바타 |
| 2 | StatusDot | 온라인 상태 점 |
| 3 | UnreadBadge | 미읽음 카운트 뱃지 |
| 4 | Chip/Tag | 멘션·주제·방 참조 태그 |
| 5 | FriendItem | 친구 목록 항목 |
| 6 | RoomItem | 채팅방 목록 항목 |
| 7 | Tabs | 사이드바 GtkNotebook 탭 |
| 8 | HeaderBar | 화면 헤더바 |
| 9 | Composer | 메시지 입력창 |
| 10 | NotificationBanner | 알림 배너 |
| 11 | MessageBubble | 좌/우 정렬 말풍선 |
| 12 | MessageDecorations | 메시지 장식 변형 |
| 13 | TypingIndicator | 타이핑 표시 |
| 14 | DateSeparator | 날짜 구분선 |
| 15 | SystemMessage | 시스템 이벤트 메시지 |
| 16 | MembersPanel | 멤버 목록 팝오버 |
| 17 | SearchOverlay | 메시지 검색 오버레이 |
| 18 | EmptyState | 빈 상태 안내 |
| 19 | RoomCard | 오픈채팅 방 카드 |
| 20 | Modal | 확인 다이얼로그 |
| 21 | Toast | 하단 스낵바 |
| 22 | PinNoticeBar | 공지/핀 표시 바 |

---

## 레퍼런스

- **Google Chat**: 좌측 내비 + 우측 대화 · 헤더 얇음 · 메시지 그룹화(동일 발신 3분 이내)
- **Gmail (Material 3)**: 중립 배경, 액센트 1색, pill-shape 뱃지, 여백과 정렬로 위계 표현
- **KakaoTalk PC**: 내 메시지 우측 정렬, 상대방 좌측 + 아바타, 시간/읽음 메시지 아래, 미읽음 수 카운터
- **공통**: 선 대신 여백으로 섹션 분리, 장식 최소, 색은 상태 전달용(성공/경고/오류/정보)
