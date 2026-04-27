# 채팅 화면

KakaoTalk PC 스타일의 **좌/우 정렬 말풍선** + 발신자 그룹핑을 GTK4으로 구현.
`GtkApplicationWindow`의 GtkStack 내 `chat-page`로 표시되며, 방 선택 시 활성화된다.

---

## 1. 채팅 페이지 전체 구조 (GtkStack 자식)

```
GtkBox (vertical)                         — chat-page 루트
  GtkHeaderBar                            — 채팅방 헤더바 (채팅 진입 시 교체)
  GtkRevealer (공지/핀 바, initially hidden)
  GtkScrolledWindow                       — 메시지 영역
    GtkListBox (css: message-list)        — 메시지 목록
  GtkLabel (css: typing-indicator)        — 타이핑 표시 (initially hidden)
  GtkBox (horizontal, css: input-area)    — 입력 영역
    GtkEntry (css: message-entry)
    GtkButton "전송" (css: suggested-action)
```

---

## 2. GtkHeaderBar (채팅방 전용)

채팅방 진입 시 메인 윈도우의 GtkHeaderBar를 채팅방 전용으로 교체:

```
GtkHeaderBar
  [좌]  GtkButton "←" (뒤로 가기, icon: go-previous-symbolic)
  title: 방 이름 GtkLabel
  subtitle: "멤버 12명" GtkLabel  (DM: "● 온라인" / "● 오프라인")
  [우]  GtkButton (공지, icon: pin-symbolic)        — 그룹방만
        GtkButton (검색, icon: system-search-symbolic)
        GtkButton (멤버, icon: system-users-symbolic) — 그룹방만
        GtkMenuButton (더보기, icon: view-more-symbolic)
          → GtkPopoverMenu
```

**더보기 GtkPopoverMenu 항목:**

| 항목 | 표시 조건 |
|------|----------|
| 멤버 목록 | 그룹방 |
| 방 검색 | 항상 |
| 공지 설정 | 방장/관리자 전용 |
| 핀 메시지 설정 | 방장/관리자 전용 |
| 오픈채팅 닉네임 | 오픈채팅 전용 |
| 알림 무음 | 항상 |
| 채팅방 나가기 | 항상 |

**DM 모드 차이:**
- 방 이름 대신 상대방 닉네임 + 상태 표시 GtkLabel.
- 공지/핀 버튼, 멤버 버튼 숨김.
- 더보기 메뉴: DM 차단, 친구 삭제, 알림 무음만 표시.

---

## 3. 공지/핀 GtkRevealer

```
GtkRevealer
  GtkBox (horizontal, css: notice-bar)
    GtkLabel "◈ 오늘 자정까지 제출"   [hexpand=TRUE]
    GtkButton "전체 보기"             (공지 전문 표시)
```

- `gtk_revealer_set_reveal_child(revealer, FALSE)` 로 숨김.
- 공지(`notice`)와 핀 메시지(`pinned_msg`) 둘 다 있으면 공지 우선.
- 공지 없으면 `gtk_revealer_set_reveal_child(notice_revealer, FALSE)`.

---

## 4. 메시지 목록 (GtkListBox)

```
GtkScrolledWindow (vscrollbar-policy=GTK_POLICY_AUTOMATIC)
  GtkListBox (selection-mode=NONE, css: message-list)
    GtkListBoxRow  — DateSeparator ("2026년 4월 20일 월요일")
    GtkListBoxRow  — MessageBubble (상대방 메시지, 좌측 정렬)
    GtkListBoxRow  — MessageBubble (내 메시지, 우측 정렬)
    GtkListBoxRow  — SystemMessage (시스템 메시지, 중앙)
    ...
```

### 4-1. 상대방 메시지 (MessageBubble, 좌측 정렬)

```
GtkBox (horizontal, halign=GTK_ALIGN_START, spacing=8, margin-start=8)
  GtkLabel (이니셜, css: avatar-label)       — 그룹핑 첫 메시지만 표시
  GtkBox (vertical, spacing=2)
    GtkLabel (닉네임, css: sender-name)      — 그룹핑 첫 메시지만 표시
    GtkBox (horizontal, spacing=4)
      GtkLabel (메시지 내용, css: bubble-other, wrap=TRUE)
      GtkBox (vertical, css: message-meta)
        GtkLabel (미읽음 수, css: unread-count accent) — 0이면 숨김
        GtkLabel (시간, css: timestamp dim-label)
```

### 4-2. 내 메시지 (MessageBubble, 우측 정렬)

```
GtkBox (horizontal, halign=GTK_ALIGN_END, spacing=8, margin-end=8)
  GtkBox (vertical, css: message-meta, halign=END)
    GtkLabel (미읽음 수, css: unread-count accent) — 0이면 숨김
    GtkLabel (시간, css: timestamp dim-label)
  GtkLabel (메시지 내용, css: bubble-self, wrap=TRUE)
```

### 4-3. 답장 메시지 (reply_to 존재)

```
GtkBox (vertical, spacing=2)
  GtkFrame (css: reply-quote)           — 인용 원문
    GtkLabel (원문 1줄 미리보기, css: dim-label, max 40자)
  GtkLabel (답장 내용, css: bubble-other 또는 bubble-self)
```

### 4-4. 시스템 메시지

```
GtkListBoxRow (css: system-message-row)
  GtkLabel (내용, css: system-message, halign=CENTER)
  — 예: "이영희 님이 입장했습니다"
  — 예: "홍길동 님이 닉네임을 '개발자A'로 변경했습니다"
```

- CSS: `color: @fg_tertiary; font-size: 12px;`

### 4-5. 날짜 구분

```
GtkListBoxRow (css: date-separator-row)
  GtkBox (horizontal)
    GtkSeparator [hexpand=TRUE]
    GtkLabel "2026년 4월 20일 월요일" (css: date-label)
    GtkSeparator [hexpand=TRUE]
```

---

## 5. 메시지 장식 변형

| 상황 | GTK4 표현 |
|------|-----------|
| `msg_type=2` 귓속말 | `bubble-whisper` CSS class + 왼쪽 아이콘 `mail-symbolic`. 발신·수신자만 표시. |
| `msg_type=3` me-action | `GtkLabel` `halign=CENTER`, CSS class `me-action` (이탤릭, dim 색상) |
| `reply_to` 존재 | `GtkFrame` (css: reply-quote) + 원문 1줄 미리보기 |
| `edited_at` 존재 | 시간 GtkLabel 뒤에 `"· 수정됨"` 추가 (css: dim-label) |
| `is_deleted=1` | 메시지 내용 GtkLabel에 `"(삭제된 메시지)"` + CSS class `deleted-message` |
| `@mention` 포함 | `gtk_label_set_markup()`으로 멘션 토큰에 `<span color="...">@닉네임</span>` 적용 |

---

## 6. 읽음 카운터

- **계산식**: `unread = member_count − read_count − 1` (발신자 본인 제외)
  - DM: `unread ∈ {0, 1}` — 1이면 미확인, 0이면 읽음.
  - 그룹방: `1 ≤ unread ≤ N`, 실시간 감소.
- `unread > 0`: `GtkLabel` 표시 (CSS class: `unread-count`, accent 색상).
- `unread == 0`: `gtk_widget_hide(unread_label)`.
- **실시간 갱신**: `DM_READ_NOTIFY` 수신 → 해당 `msg_id` 행의 unread GtkLabel 갱신.
  - 100ms 디바운스: `g_timeout_add(100, update_read_count_cb, ...)`.

---

## 7. 타이핑 표시 (GtkLabel)

```
GtkLabel (css: typing-indicator, initially hidden)
— 예: "홍길동 님이 입력 중..."
— 복수: "홍길동 외 2명이 입력 중..."
```

- `TYPING_NOTIFY` 패킷 수신 → `gtk_label_set_text()` + `gtk_widget_show()`.
- `TYPING_STOP` 수신 또는 5초 경과 → `gtk_widget_hide()`.

---

## 8. 입력 영역 (GtkBox)

```
GtkBox (horizontal, spacing=8, margin=8, css: input-area)
  GtkEntry (hexpand=TRUE, placeholder_text="메시지 입력...", css: message-entry)
  GtkButton "전송" (css: suggested-action, icon: mail-send-symbolic)
```

- `GtkEntry`에서 Enter 키 → `gtk_button_clicked(send_btn)` 트리거.
- 빈 입력 시 전송 버튼 비활성화 (`gtk_widget_set_sensitive(send_btn, FALSE)`).
- 연결 끊김 상태: `gtk_widget_set_sensitive(entry, FALSE)` + 입력창 dim 처리.

### 답장 모드

답장 대상 메시지가 있을 때 입력 영역 위에 인용 바 추가:

```
GtkBox (horizontal, css: reply-preview-bar)
  GtkLabel (원문 미리보기, css: dim-label)  [hexpand=TRUE]
  GtkButton "×" (답장 취소, icon: window-close-symbolic)
GtkEntry ...  (기존 입력창)
```

---

## 9. 메시지 우클릭 컨텍스트 메뉴 (GtkPopoverMenu)

메시지 `GtkListBoxRow`에 우클릭 이벤트 연결:

```
GtkPopoverMenu
  "답장"       — reply_to 설정 → 입력 영역에 인용 바 표시
  "수정"       — 내 메시지만 활성화, 전송 후 5분 이내
  "삭제"       — 내 메시지만 활성화, GtkAlertDialog 확인 후
  "귓속말"     — 그룹방만, 대상 발신자에게 귓속말 모드 설정
  "복사"       — 클립보드에 메시지 내용 복사
```

---

## 10. 검색 오버레이 (GtkSearchBar)

HeaderBar의 검색 버튼 클릭 시 `GtkSearchBar` 슬라이드 인:

```
GtkSearchBar (search-mode-enabled=TRUE)
  GtkBox (horizontal)
    GtkSearchEntry (hexpand=TRUE, placeholder="방 내 메시지 검색...")
    GtkLabel "결과 3개" (css: meta-label)
    GtkButton "↑" (이전 결과)
    GtkButton "↓" (다음 결과)
    GtkButton "×" (검색 닫기)

GtkScrolledWindow (검색 결과 목록, initially hidden)
  GtkListBox (css: search-result-list)
    GtkListBoxRow  — SearchResultItem
      GtkLabel "[14:00] 홍길동"
      GtkLabel (내용 미리보기, 검색어 강조: gtk_label_set_markup)
```

- 검색어 포함 단어: `<b><span color="@accent_primary">검색어</span></b>`로 markup.
- 결과 항목 클릭 → 해당 메시지 위치로 스크롤 + 해당 행 2초간 CSS class `highlight`.

---

## 11. 멤버 목록 (GtkPopover)

HeaderBar 멤버 버튼 클릭 → `GtkPopover` 표시 (우측 GtkPaned 분할도 고려):

```
GtkPopover (width=260, css: member-popover)
  GtkBox (vertical)
    GtkBox (horizontal)
      GtkLabel "멤버 12명" (title)
      GtkButton "×" (popover 닫기)
    GtkSeparator
    GtkScrolledWindow
      GtkListBox (css: member-list)
        [섹션 헤더] GtkLabel "방장"
        GtkListBoxRow — MemberItem (홍길동, ★, online)
        [섹션 헤더] GtkLabel "관리자"
        GtkListBoxRow — MemberItem (김철수, ☆, busy)
        [섹션 헤더] GtkLabel "멤버 (10)"
        GtkListBoxRow — MemberItem ...
```

**MemberItem 우클릭 → GtkPopoverMenu:**
- `DM 보내기` (항상)
- `강퇴` (방장/관리자 전용)
- `관리자 권한 부여/해제` (방장 전용)

---

## 12. 스크롤 · 히스토리

- 메시지 표시 한도: `CHAT_VIEW_CAP=200`.
- 맨 위까지 스크롤 시 자동으로 이전 메시지 추가 요청 (스크롤 position < 10%).
  - 로딩 중: GtkListBox 맨 위에 `GtkSpinner` 표시.
- 스크롤이 맨 아래가 아닐 때: 우하단에 `GtkButton "↓ N개 새 메시지"` 오버레이 표시.
  - 클릭 시 `gtk_adjustment_set_value(adj, gtk_adjustment_get_upper(adj))`.

---

## 13. 오픈채팅 전용 요소

- HeaderBar 타이틀에 `[오픈]` GtkLabel (CSS class: `open-chat-chip`) 추가.
- 멤버 목록에서 닉네임은 `open_nick` 사용.
- 내 메시지 옆에 `(나: 개발자A)` GtkLabel (CSS class: `open-nick-meta`).
- 닉네임 변경 시 시스템 메시지 자동 표시.
