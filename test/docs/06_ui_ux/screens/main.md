# 메인 화면

Google Chat의 **사이드바 + 콘텐츠 영역** 2-pane 레이아웃을 GTK4으로 구현.
좌측은 GtkNotebook 탭(친구/채팅/오픈채팅/마이페이지), 우측은 선택 항목에 따라 바뀌는 GtkStack.

---

## 1. GtkApplicationWindow 구조

### 윈도우 속성

| 속성 | 값 |
|------|-----|
| 위젯 타입 | `GtkApplicationWindow` |
| 기본 크기 | 1000 × 700 px |
| 최소 크기 | 800 × 600 px |
| CSS class | `main-window` |

### 최상위 위젯 트리

```
GtkApplicationWindow
  GtkBox (vertical)
    GtkHeaderBar                  — 상단 헤더바
    GtkRevealer + GtkBox          — 알림 바 (initially hidden)
    GtkPaned (horizontal)         — 사이드바 + 콘텐츠 분할
      [좌] GtkBox (vertical, 220px)  — 사이드바
             GtkBox (identity 카드)
             GtkNotebook (4 탭)
      [우] GtkStack               — 콘텐츠 영역
```

---

## 2. GtkHeaderBar

```
GtkHeaderBar
  title:         "C Chat"
  [좌 버튼들]   [친구 추가 GtkButton] [방 만들기 GtkButton]
  [우 버튼들]   [설정 GtkButton]
```

- `gtk_header_bar_set_show_title_buttons(headerbar, TRUE)`
- **친구 추가** 버튼: 아이콘 `list-add-symbolic`, 툴팁 `"친구 추가"`
  → 클릭 시 친구 추가 GtkWindow (모달) 열기
- **방 만들기** 버튼: 아이콘 `chat-symbolic`, 툴팁 `"새 채팅방 만들기"`
  → 클릭 시 채팅방 생성 GtkWindow (모달) 열기
- **설정** 버튼: 아이콘 `preferences-system-symbolic`, 툴팁 `"설정"`
  → 클릭 시 설정 GtkWindow (모달) 열기

---

## 3. GtkRevealer (알림 바)

`GtkRevealer + GtkBox`는 HeaderBar 바로 아래에 위치하며 기본적으로 숨겨져 있음.

- 친구 요청 알림: CSS class `notification-info`
  - 메시지: `"홍길동 님이 친구 요청을 보냈습니다"`
  - 액션 버튼: `[수락]` `[거절]`
- 멘션 알림: CSS class `notification-other`
  - 메시지: `"#컴공 스터디그룹 에서 당신을 멘션했습니다"`
- 연결 끊김: CSS class `notification-error`
  - 메시지: `"서버와의 연결이 끊어졌습니다. 재연결 중..."`
  - `gtk_revealer_set_reveal_child(revealer, FALSE)` 로만 숨김 — 해제 불가
- 최대 3개 알림을 GtkBox (vertical) 안에 GtkRevealer로 표시.
- 일반 알림은 5초 후 `g_timeout_add`로 자동 숨김.

---

## 4. 사이드바 (GtkBox 220px)

### 4-1. Identity 카드 (내 프로필)

```
GtkBox (horizontal, spacing=8, margin=12)
  GtkLabel (이니셜, css: avatar-label)   — 아바타 대용
  GtkBox (vertical)
    GtkLabel (닉네임, bold)
    GtkBox (horizontal)
      GtkLabel (● 상태점, css: status-dot-online/busy/offline)
      GtkLabel (상태 메시지, css: dim-label)
```

- 아바타 GtkLabel: 닉네임 첫 글자, CSS로 원형 배경 + 해시 기반 색상.
- 우클릭 또는 클릭 → 마이페이지 탭으로 이동.

### 4-2. GtkNotebook (4 탭)

```
GtkNotebook (tab-pos=GTK_POS_TOP, scrollable=TRUE)
  탭 0: "친구"      → 친구 목록 GtkListBox
  탭 1: "채팅"      → 채팅 목록 GtkListBox (미읽음 뱃지 포함)
  탭 2: "오픈채팅"  → GtkSearchEntry + 방 목록 GtkListBox
  탭 3: "마이페이지" → 프로필 정보 패널
```

탭 레이블에 미읽음 수 뱃지 표시:
```c
GtkWidget *tab_label = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
GtkWidget *label     = gtk_label_new("채팅");
GtkWidget *badge     = gtk_label_new("3");  /* 미읽음 수 */
gtk_style_context_add_class(gtk_widget_get_style_context(badge), "badge");
```

---

## 5. 탭별 콘텐츠

### 5-1. 친구 탭 (GtkListBox)

```
GtkScrolledWindow
  GtkListBox (selection-mode=SINGLE, css: friend-list)
    [섹션 헤더] GtkLabel "받은 요청 · 2"   (미처리 요청 있을 때만)
    GtkListBoxRow  — FriendItem (요청)  : 수락/거절 GtkButton 포함
    [섹션 헤더] GtkLabel "온라인 · 3"
    GtkListBoxRow  — FriendItem (online)
    GtkListBoxRow  — FriendItem (online)
    [섹션 헤더] GtkLabel "오프라인 · 2"
    GtkListBoxRow  — FriendItem (offline)
```

**FriendItem (GtkListBoxRow 내부)** — `components.md` FriendItem 참고:

```
GtkBox (horizontal, spacing=8)
  GtkLabel (이니셜, css: avatar-label)
  GtkBox (vertical)
    GtkBox (horizontal)
      GtkLabel (● css: status-dot)
      GtkLabel (닉네임)
    GtkLabel (상태 메시지, css: dim-label caption)
  [오른쪽 끝] GtkLabel (마지막 활동, css: meta-label)
```

- 항목 클릭 → 오른쪽 GtkStack에서 DM 채팅 페이지 표시.
- 우클릭 → GtkPopoverMenu: `[DM 보내기]` `[프로필 보기]` `[친구 삭제]`.

**빈 상태 위젯:**
```
GtkBox (vertical, halign=CENTER, valign=CENTER)
  GtkLabel "아직 친구가 없습니다."
  GtkLabel "친구 추가 버튼으로 첫 친구를 추가해보세요." (css: dim-label)
```

### 5-2. 채팅 탭 (GtkListBox)

그룹방 + 최근 DM 혼합, 최근 메시지 시간 내림차순.

```
GtkScrolledWindow
  GtkListBox (selection-mode=SINGLE, css: chat-list)
    GtkListBoxRow  — RoomItem (그룹채팅)
    GtkListBoxRow  — RoomItem (DM)
    GtkListBoxRow  — RoomItem (오픈채팅)
    ...
```

**RoomItem (GtkListBoxRow 내부)** — `components.md` RoomItem 참고:

```
GtkBox (horizontal, spacing=8, margin=8)
  GtkLabel (이니셜/타입 아이콘, css: avatar-label)
  GtkBox (vertical, hexpand=TRUE)
    GtkBox (horizontal)
      GtkLabel (방 이름, bold)         [hexpand=TRUE]
      GtkLabel (마지막 메시지 시간, css: meta-label)
    GtkBox (horizontal)
      GtkLabel (마지막 메시지 미리보기, css: dim-label) [hexpand=TRUE]
      GtkLabel (미읽음 수, css: unread-badge)  [initially hidden if 0]
```

- 방 타입에 따른 아이콘: 그룹 `#`, DM `✉`, 오픈 `#[오픈]`.
- 미읽음 뱃지: `css: unread-badge` (둥근 배경 + accent 색).

### 5-3. 오픈채팅 탭

```
GtkBox (vertical)
  GtkSearchEntry (placeholder: "방 이름 검색...")
  GtkScrolledWindow
    GtkListBox (css: open-chat-list)
      GtkListBoxRow  — RoomItem (오픈채팅 방)
      ...
```

- `GtkSearchEntry`의 `search-changed` 시그널 → 300ms 디바운스 후 서버 검색 요청.
- 비밀번호 방 아이템: 자물쇠 아이콘(`GtkImage: changes-prevent-symbolic`) 표시.
- 인원 가득 찬 방: `[FULL]` GtkLabel + 비활성화.

### 5-4. 마이페이지 탭

```
GtkBox (vertical, spacing=16, margin=24)
  GtkLabel (이니셜, css: avatar-large)
  GtkLabel (닉네임, css: title)
  GtkLabel (아이디, css: dim-label)
  GtkLabel (상태 메시지)
  GtkSeparator
  GtkButton "상태 메시지 변경"
  GtkButton "닉네임 변경"
  GtkButton "비밀번호 변경"
```

---

## 6. 오른쪽 콘텐츠 영역 (GtkStack)

사이드바에서 항목 선택 시 `gtk_stack_set_visible_child_name()`으로 전환.

| 상태 | GtkStack 자식 페이지 |
|------|---------------------|
| 아무것도 선택 안 됨 | `welcome-page` — 환영 메시지 |
| 친구 선택 (DM) | `chat-page` — 채팅 화면 (`screens/chat.md` 참고) |
| 그룹방 선택 | `chat-page` — 채팅 화면 |
| 오픈채팅방 선택 | `chat-page` — 채팅 화면 |
| 마이페이지 탭 | `mypage-page` — 마이페이지 화면 |

페이지 전환 애니메이션: `gtk_stack_set_transition_type(stack, GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT)`

---

## 7. 채팅방 생성 GtkWindow (모달)

**채팅 타입 선택 다이얼로그:**

```
GtkWindow (modal) "새 채팅 시작"
  GtkBox (vertical, spacing=12, margin=16)
    GtkLabel "어떤 채팅을 시작하시겠어요?"
    GtkButton "1:1 DM — 친구와 개인 대화"
    GtkButton "그룹 채팅방 — 초대 기반 비공개 방"
    GtkButton "오픈채팅방 — 누구나 참여 가능한 방"
    GtkBox (horizontal, action-area)
      GtkButton "취소" → gtk_window_close()
```

**그룹 채팅방 생성 다이얼로그:**

```
GtkWindow (modal) "그룹 채팅방 만들기"
  GtkGrid (column-spacing=12, row-spacing=8, margin=16)
    [0,0] GtkLabel "방 이름 *"   [0,1] GtkEntry (max_length=30)
    [1,0] GtkLabel "주제"        [1,1] GtkEntry (max_length=100)
    [2,0] GtkLabel "최대 인원"   [2,1] GtkSpinButton (range: 2~64, default=30)
    [3,0] GtkLabel "비밀번호"    [3,1] GtkEntry (visibility=FALSE, optional)
  GtkBox (horizontal, action-area)
    GtkButton "취소" → gtk_window_close()
    GtkButton "만들기" (css: suggested-action) → on_create_room_clicked()
```

성공 → `gtk_stack_set_visible_child_name(stack, "chat-page")`로 새 방 채팅 화면 진입.

---

## 8. 유저 검색 결과 패널

`GtkStack`에 `search-result-page` 추가. 검색 실행 시 해당 페이지로 전환.

```
GtkBox (vertical)
  GtkBox (horizontal)                        — 검색 헤더
    GtkLabel "검색 결과: 3"
    GtkButton "×" (닫기, 이전 페이지 복귀)
  GtkScrolledWindow
    GtkListBox (css: search-result-list)
      GtkListBoxRow  — SearchResultItem
        GtkBox (horizontal)
          GtkLabel (이니셜, css: avatar-label)
          GtkBox (vertical)
            GtkLabel (닉네임)
            GtkLabel (아이디, css: dim-label)
            GtkLabel (상태 메시지, css: dim-label)
          GtkButton "친구 추가" / "이미 친구" (비활성)
          GtkButton "DM"
```

- `[친구 추가]` 클릭 → FRIEND_ADD_REQ 전송 → 버튼을 `[요청 전송됨]`으로 교체 + 비활성화.

---

## 9. 알림 수신 처리

서버에서 수신하는 알림 패킷 → `GtkRevealer` 표시:

| 패킷 타입 | Revealer 동작 |
|-----------|-------------|
| `FRIEND_REQ_NOTIFY` | Revealer 표시, `[수락]` `[거절]` 버튼 |
| `MENTION_NOTIFY` | Revealer 표시, `[바로 이동]` 버튼 |
| 연결 끊김 감지 | 에러 Revealer, 해제 불가, 재연결 시 자동 숨김 |
