# 오픈채팅 화면

누구나 발견·참여할 수 있는 오픈채팅방 탐색·참여 전용 화면.
메인 사이드바에서 `오픈채팅` 탭(`Alt+3`) 또는 MAIN에서 해당 탭 클릭으로 진입.

---

## 1. 탐색(Browse) 패널 — 레이아웃 (GTK4 위젯 트리)

```
GtkApplicationWindow
└── GtkHeaderBar
    ├── GtkLabel "오픈채팅" (title)
    └── GtkButton "+ 오픈채팅 만들기" (header-end)

GtkBox (vertical)
├── GtkSearchEntry (방 이름/주제 검색, placeholder="방 이름 또는 주제로 검색...")
│   └── 입력 변경 시 ROOM_SEARCH 패킷 전송 (디바운스 300ms)
├── GtkScrolledWindow
│   └── GtkListBox (오픈채팅방 카드 목록)
│       └── GtkListBoxRow (각 방)
│           ├── GtkLabel 방 이름 (bold)
│           ├── GtkLabel 주제 (fg=secondary, max 50자 ellipsize)
│           ├── GtkLabel "cur/max" (인원)
│           └── GtkButton "참여하기"
└── (검색 결과 없음: GtkLabel + GtkButton "+ 오픈채팅 만들기")
```

### 방 카드 상태 표시

- 비밀번호 방: 방 이름 옆 `GtkImage`(자물쇠 아이콘).
- 만원(cur == max): 인원 `GtkLabel`을 error 색상으로 표시, `GtkButton "참여하기"` 비활성화.
- 내가 이미 참여 중인 방: `GtkButton "열기"` (참여하기 대신 표시).

---

## 2. 실시간 검색

`GtkSearchEntry` 입력 변경 시 `ROOM_SEARCH` 패킷 전송. 디바운스 **300ms**.

검색 결과는 `GtkListBox`를 갱신하여 표시. 검색어를 지우면 전체 목록으로 복귀.

검색 결과가 없을 때:

- `GtkLabel`: "검색 결과가 없습니다. 다른 키워드로 검색하거나 직접 만들어보세요."
- `GtkButton "+ 오픈채팅 만들기"` → 방 생성 다이얼로그 열기.

---

## 3. 방 상세 → 참여 플로우

`GtkListBoxRow`에서 `GtkButton "참여하기"` 클릭:

### 3-1. 공개방 참여 확인

`GtkAlertDialog` 표시:

- 제목: 방 이름
- 본문: 주제, 멤버 수
- `GtkEntry` 오픈채팅 닉네임 (선택, placeholder="기본 닉네임 사용")
- `GtkButton "취소"` / `GtkButton "참여하기"`

### 3-2. 비밀번호 방 참여

`GtkAlertDialog` 표시:

- 제목: 방 이름
- 본문: "이 방은 비밀번호로 보호되어 있습니다."
- `GtkEntry` 비밀번호 (password 모드)
- 실패 시 `GtkLabel "✗ 비밀번호가 맞지 않습니다."` 표시 (다이얼로그 유지)
- `GtkButton "취소"` / `GtkButton "확인"`

### 3-3. 만원 방

`GtkAlertDialog` 표시:

- 본문: "최대 인원에 도달했습니다."
- `GtkButton "확인"`

참여 성공 → `GtkStack.set_visible_child_name("chat")` 으로 CHAT 화면 전환 + 입장 시스템 메시지 렌더.

---

## 4. 오픈채팅방 생성 (헤더바 버튼)

`GtkHeaderBar`의 `GtkButton "+ 오픈채팅 만들기"` 클릭 → 별도 `GtkWindow` 다이얼로그:

| 필드 | 위젯 | 제약 |
|------|------|------|
| 방 이름 * | `GtkEntry` | 최대 30자, 필수 |
| 주제 | `GtkEntry` | 최대 100자, 선택 |
| 최대 인원 | `GtkSpinButton` | 기본 30, 최대 500 |
| 비밀번호 | `GtkEntry` (password 모드) | 선택, 비우면 공개방 |

- `GtkButton "취소"` / `GtkButton "만들기"`

성공 시 `ROOM_CREATE_RES` 수신 → 생성된 방 CHAT 화면으로 바로 진입.

---

## 5. 오픈채팅 닉네임 변경 (채팅 중)

CHAT 화면 GtkHeaderBar의 `⋯` GtkButton → `GtkPopoverMenu` → "닉네임 변경":

- `GtkAlertDialog`: `GtkEntry` (기존 닉네임 pre-fill)
- 비워두면 기본 닉네임 사용
- 성공 시 Toast "✓ 닉네임이 변경되었습니다". GtkHeaderBar 닉 업데이트.

---

## 6. GTK4 단축키

| 키 | 동작 |
|----|------|
| `Alt+3` | 오픈채팅 탭 포커스 |
| `Ctrl+N` | 오픈채팅방 만들기 다이얼로그 |
| `Ctrl+F` | GtkSearchEntry 포커스 |
| `Escape` | 검색어 초기화 / 다이얼로그 닫기 |
