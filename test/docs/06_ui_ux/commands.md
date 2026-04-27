# GTK4 GUI 인터랙션 레퍼런스

GTK4 GUI에서 TUI의 슬래시 커맨드 역할을 하는 것은 버튼, 컨텍스트 메뉴, 키보드 단축키다.  
이 문서는 앱의 모든 주요 액션이 어떤 UI 요소로 노출되는지 정리한다.

---

## 1. 전역 단축키

`GtkShortcutController`를 `GtkApplicationWindow`에 연결해 처리.

| 단축키 | 동작 | 구현 메모 |
|--------|------|-----------|
| `Ctrl+Q` | 앱 종료 | `g_application_quit()` |
| `Ctrl+,` | 설정 화면 열기 | `gtk_stack_set_visible_child_name(stack, "settings")` |
| `Ctrl+Shift+N` | 새 오픈채팅방 만들기 | 오픈채팅 탭으로 전환 후 생성 다이얼로그 표시 |
| `Alt+1` | 채팅 목록 탭 | `gtk_stack_set_visible_child_name(main_stack, "rooms")` |
| `Alt+2` | 친구 목록 탭 | `gtk_stack_set_visible_child_name(main_stack, "friends")` |
| `Alt+3` | 오픈채팅 탭 | `gtk_stack_set_visible_child_name(main_stack, "open")` |
| `Alt+4` | 마이페이지 탭 | `gtk_stack_set_visible_child_name(main_stack, "mypage")` |

---

## 2. 채팅창 단축키

채팅 화면(`screen_chat`) 진입 시 활성화. `GtkShortcutController`를 `screen_chat` 위젯에 추가.

| 단축키 | 동작 |
|--------|------|
| `Enter` | 메시지 전송 |
| `Shift+Enter` | 입력창 줄바꿈 |
| `Ctrl+Enter` | 메시지 전송 (대안) |
| `Ctrl+F` | 메시지 검색 바 표시 (`GtkSearchBar` 토글) |
| `Escape` | 답장/수정 취소, 검색 바 닫기 |

---

## 3. 컨텍스트 메뉴 (메시지 우클릭)

`GtkPopoverMenu` + `GMenuModel`로 구현. 각 메시지 위젯에 `GtkGestureClick` (button=3) 연결.

### GMenuModel 항목 명세

```c
GMenu *menu = g_menu_new();
g_menu_append(menu, "답장",        "chat.reply");
g_menu_append(menu, "메시지 복사", "chat.copy");
/* 내 메시지일 때만 추가 */
g_menu_append(menu, "수정",        "chat.edit");    /* 5분 이내 */
g_menu_append(menu, "삭제",        "chat.delete");  /* 언제든 가능 */
/* 다른 사람 메시지일 때만 추가 */
g_menu_append(menu, "귓속말",      "chat.whisper"); /* 귓속말 모드 토글 */
```

| 항목 | 조건 | 대응 패킷 |
|------|------|-----------|
| 답장 | 모든 메시지 | `MSG_REPLY` |
| 메시지 복사 | 모든 메시지 | 클립보드 (`gdk_clipboard_set_text`) |
| 수정 | 내 메시지, 전송 후 5분 이내 | `MSG_EDIT` |
| 삭제 | 내 메시지, 언제든 | `MSG_DELETE` |
| 귓속말 | 다른 사람 메시지 | `WHISPER` |

---

## 4. 헤더바 버튼

각 화면의 `GtkHeaderBar` 오른쪽(`gtk_header_bar_pack_end`)에 추가되는 버튼.

| 화면 | 버튼 | 동작 |
|------|------|------|
| 채팅방 (`screen_chat`) | 👥 참여자 목록 | `GtkPopover` 또는 사이드 패널 표시 |
| | 📌 공지사항 | `GtkAlertDialog`로 현재 공지 표시 |
| | 🔍 메시지 검색 | `GtkSearchBar` 토글 (`Ctrl+F`와 동일) |
| | ⋮ 더보기 | `GtkPopoverMenu`: 방 나가기, 초대 |
| 메인 (`screen_main`) | ✏ 새 채팅 | 채팅방 생성 다이얼로그 (`GtkAlertDialog`) |
| | 🔔 알림 | 알림 목록 `GtkPopover` |
| 마이페이지 (`screen_mypage`) | ✏ 프로필 편집 | 편집 모드 전환 |
| 설정 (`screen_settings`) | — | 저장 버튼은 콘텐츠 영역에 배치 |

---

## 5. GtkPopoverMenu 팝오버 명세

### 5-1. 채팅방 더보기 팝오버 (`chat_overflow_menu`)

```c
GMenu *menu = g_menu_new();
g_menu_append(menu, "채팅방 정보",    "chat.room-info");
g_menu_append(menu, "유저 초대",      "chat.invite");
g_menu_append(menu, "알림 끄기/켜기", "chat.toggle-mute");
g_menu_append(menu, "채팅방 나가기",  "chat.leave");
GtkWidget *popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
gtk_menu_button_set_popover(GTK_MENU_BUTTON(btn), popover);
```

### 5-2. 친구 행 팝오버 (`friend_row_menu`)

```c
GMenu *menu = g_menu_new();
g_menu_append(menu, "1:1 DM 보내기", "friend.dm");
g_menu_append(menu, "친구 삭제",     "friend.delete");
g_menu_append(menu, "차단",          "friend.block");
```

### 5-3. 오픈채팅방 행 팝오버 (`open_room_row_menu`)

```c
GMenu *menu = g_menu_new();
g_menu_append(menu, "입장",         "open.join");
g_menu_append(menu, "방 정보 보기", "open.info");
```

---

## 6. 접근성: 키보드 전용 접근 가능 여부

모든 주요 기능은 마우스 없이 키보드만으로 접근 가능해야 한다.

| 기능 | 키보드 접근 방법 | 가능 여부 |
|------|-----------------|-----------|
| 로그인 / 회원가입 | `Tab`으로 필드 이동, `Enter` 제출 | ✅ |
| 탭 전환 (채팅/친구/오픈/마이) | `Alt+1~4` | ✅ |
| 메시지 전송 | `Enter` | ✅ |
| 메시지 검색 | `Ctrl+F` | ✅ |
| 컨텍스트 메뉴 열기 | `Shift+F10` (GTK 기본 컨텍스트 메뉴 키) | ✅ |
| 설정 화면 | `Ctrl+,` | ✅ |
| 앱 종료 | `Ctrl+Q` | ✅ |
| 채팅방 나가기 | 헤더바 더보기 팝오버 → `Tab` + `Enter` | ✅ |
| 친구 추가 | 친구 탭 → 검색 필드 → `Tab` + `Enter` | ✅ |
| 새 오픈채팅방 | `Ctrl+Shift+N` | ✅ |

> `GtkWidget`은 기본적으로 포커스 가능. 커스텀 위젯은 `gtk_widget_set_focusable(widget, TRUE)` 명시적으로 호출할 것.
