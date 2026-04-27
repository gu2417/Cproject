# 마이페이지

Gmail 계정 페이지의 **아바타 + 메타 + 통계 카드** 레이아웃.
한 페이지에 profile / security / stats / actions 4 영역을 여백으로 구분.

## 레이아웃 (GTK4 위젯 트리)

```
GtkScrolledWindow
└── GtkBox (vertical, spacing=16, margin=24)
    ├── Hero 영역: GtkBox (horizontal, spacing=16)
    │   ├── GtkImage (아바타 placeholder, 80×80)
    │   ├── GtkBox (vertical, spacing=4)
    │   │   ├── GtkLabel (닉네임, font-weight=bold, font-size=large)
    │   │   ├── GtkLabel (상태 메시지, fg=secondary)
    │   │   └── GtkLabel (마지막 접속, fg=tertiary)
    │   └── GtkButton "프로필 수정" (halign=end)
    ├── 통계 카드: GtkGrid (2열, row-spacing=8, column-spacing=24)
    │   ├── GtkLabel "보낸 메시지" + GtkLabel count (bold)
    │   └── GtkLabel "참여 채팅방" + GtkLabel count (bold)
    ├── GtkSeparator (horizontal)
    ├── GtkLabel "참여 중인 채팅방" (섹션 헤더) + GtkListBox
    └── GtkLabel "최근 DM" (섹션 헤더) + GtkListBox
```

## 구성 요소

| 영역 | 위젯 |
|------|------|
| Hero | `GtkImage`(80×80) + `GtkLabel`(닉네임, bold) + `GtkLabel`(상태메시지) + `GtkLabel`(ID) |
| 통계 | `GtkGrid` 2열: 보낸 메시지 수, 참여 채팅방 수 |
| 보안 | `GtkLabel` 테이블: 마지막 접속 · 가입일 (상대 시간) |
| 액션 | `GtkButton` 4개: 프로필 수정 · 비밀번호 변경 · 설정 · 로그아웃 |

## 프로필 수정 (다이얼로그)

`GtkButton "프로필 수정"` 클릭 시 `GtkAlertDialog` 또는 별도 `GtkWindow`:

- `GtkEntry` 닉네임 (기존 값 pre-fill)
- `GtkEntry` 상태 메시지 (기존 값 pre-fill)
- `GtkButton "취소"` / `GtkButton "저장"`

`저장` 클릭 → `PROFILE_UPDATE` 전송 + 성공 시 Toast "✓ 저장되었습니다".

단축키 `Ctrl+E` → 프로필 수정 다이얼로그 열기.

## 비밀번호 변경 (다이얼로그)

`GtkButton "비밀번호 변경"` 클릭 시 `GtkAlertDialog`:

- `GtkEntry` 현재 비밀번호 (password 모드)
- `GtkEntry` 새 비밀번호 (password 모드)
- `GtkEntry` 새 비밀번호 확인 (password 모드)
- `GtkButton "취소"` / `GtkButton "변경"`

## 데이터 소스

- 진입 시 `MYPAGE` 1회 요청. 응답으로 Hero + 통계 + 보안 영역 채움.
- `마지막 접속`은 `last_seen` 기반 상대 시간:

| 차이 | 표시 |
|------|------|
| <1분 | `방금 전` |
| <60분 | `N분 전` |
| <24시간 | `N시간 전` |
| 그 외 | `YYYY. M. D` |

## GTK4 가속키

| 키 | 동작 |
|----|------|
| `Ctrl+E` | 프로필 수정 다이얼로그 열기 |
| `Escape` | 이전 화면 복귀 |
