# 컴포넌트

GTK4 위젯 기반 재사용 단위. 모든 스크린은 이 컴포넌트의 배치로 구성된다.
디자인 토큰은 `design_tokens.md` 참고.

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

## 1. Avatar (이니셜 원형 아바타)

GTK4에서는 별도 그리기 없이 CSS로 원형 아바타를 구현한다.

```c
GtkWidget *avatar = gtk_label_new("김");
gtk_style_context_add_class(gtk_widget_get_style_context(avatar), "avatar-label");
/* CSS에서 border-radius: 50%, min-width/min-height: 36px 로 원형 처리 */
```

CSS:
```css
.avatar-label {
  min-width: 36px;
  min-height: 36px;
  border-radius: 50%;
  font-weight: bold;
  color: @avatar_fg;
  background-color: @avatar_bg;  /* 닉네임 해시 기반으로 C 코드에서 동적 설정 */
}
.avatar-label.offline {
  opacity: 0.5;
}
```

- 닉네임 첫 글자(한글/영문/숫자) 1자. 이모지 이니셜은 `?`로 치환.
- 색 결정: `design_tokens.md §6` FNV1a 해시 → CSS provider로 동적 적용 (`gtk_widget_add_css_class()`). `gtk_widget_override_background_color()`는 GTK4에서 제거됨.
- 크기 변형: `avatar-label.small` (24px), `avatar-label.large` (56px).

---

## 2. StatusDot (온라인 상태 점)

```c
GtkWidget *dot = gtk_label_new("●");
gtk_style_context_add_class(ctx, "status-dot");
gtk_style_context_add_class(ctx, "status-online");  /* online / busy / dnd / offline */
```

CSS:
```css
.status-dot.status-online  { color: @status_online; }
.status-dot.status-busy    { color: @status_busy; }
.status-dot.status-dnd     { color: @status_dnd; }
.status-dot.status-offline { color: @status_offline; }
```

- 아바타 오른쪽 또는 닉네임 앞에 배치.
- 상태 변경 시 CSS class를 교체하여 색상 업데이트.

---

## 3. UnreadBadge (미읽음 카운트 뱃지)

```c
GtkWidget *badge = gtk_label_new("3");
gtk_style_context_add_class(ctx, "unread-badge");
/* 값이 0이면 gtk_widget_hide(badge) */
```

CSS:
```css
.unread-badge {
  background-color: @accent_primary;
  color: @fg_on_accent;
  border-radius: 10px;
  min-width: 20px;
  padding: 2px 6px;
  font-size: 11px;
  font-weight: bold;
}
```

- 1~99: 숫자 표시.
- 100+: `"99+"` 클램프.
- 0: `gtk_widget_hide()`.

---

## 4. Chip/Tag (멘션·주제·방 참조 태그)

멘션(@닉네임), 주제 태그, 방 참조(#방이름)에 사용.

```c
GtkWidget *chip = gtk_label_new("@홍길동");
gtk_widget_add_css_class(chip, "chip");
gtk_widget_add_css_class(chip, "chip-mention");  /* mention / topic / room-ref */
```

CSS:
```css
.chip { border-radius: 12px; padding: 2px 8px; font-size: 12px; }
.chip-mention { background-color: alpha(@accent_primary, 0.2); color: @accent_primary; }
.chip-topic   { background-color: alpha(@status_online, 0.2);  color: @status_online; }
```

---

## 5. FriendItem (친구 목록 항목)

`GtkListBox`의 `GtkListBoxRow` 내부 구조:

```c
GtkWidget *row_box  = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
GtkWidget *avatar   = make_avatar_label(nick);
GtkWidget *info_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
GtkWidget *name_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
GtkWidget *dot      = make_status_dot(status);
GtkWidget *nick_lbl = gtk_label_new(nickname);
GtkWidget *status_m = gtk_label_new(status_msg);    /* dim-label */
GtkWidget *meta_lbl = gtk_label_new(last_seen);     /* 우측 끝, meta-label */

gtk_widget_set_hexpand(info_box, TRUE);
gtk_label_set_xalign(GTK_LABEL(status_m), 0.0);
gtk_label_set_ellipsize(GTK_LABEL(status_m), PANGO_ELLIPSIZE_END);
```

- 우클릭 → `GtkPopoverMenu`: `[DM 보내기]` `[프로필 보기]` `[친구 삭제]`.
- 친구 요청 행: 우측에 `[수락]` `[거절]` GtkButton 추가.
- 오프라인 항목: status_msg 자리에 `"마지막 접속: 2시간 전"` 표시.

---

## 6. RoomItem (채팅방 목록 항목)

```c
GtkWidget *row_box    = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
GtkWidget *icon_lbl   = make_room_icon(room_type, room_name); /* # 또는 ✉ */
GtkWidget *info_box   = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
GtkWidget *top_row    = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
GtkWidget *name_lbl   = gtk_label_new(room_name);
GtkWidget *time_lbl   = gtk_label_new(last_msg_time);   /* meta-label, 우측 정렬 */
GtkWidget *preview    = gtk_label_new(last_msg_preview);/* dim-label */
GtkWidget *badge      = make_unread_badge(unread_count);/* unread-badge, 우측 */

gtk_widget_set_hexpand(name_lbl, TRUE);
gtk_label_set_ellipsize(GTK_LABEL(preview), PANGO_ELLIPSIZE_END);
```

- 방 타입에 따른 아이콘: 그룹 `#`, DM `✉`, 오픈 `#` + `[오픈]` 칩.
- 미읽음 = 0이면 `gtk_widget_hide(badge)`.

---

## 7. Tabs (사이드바 GtkNotebook 탭)

친구/채팅/오픈채팅/마이페이지 4탭. 탭 라벨에 UnreadBadge 포함. Alt+1~4 단축키.

```c
GtkWidget *notebook = gtk_notebook_new();
gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
/* 탭 라벨: GtkBox(horizontal) = GtkLabel(탭명) + GtkLabel(.unread-badge) */
GtkWidget *tab_box  = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
GtkWidget *tab_lbl  = gtk_label_new("채팅");
GtkWidget *badge    = gtk_label_new("3");
gtk_widget_add_css_class(badge, "unread-badge");
gtk_widget_set_visible(badge, FALSE);  /* 미읽음=0이면 숨김 */
```

---

## 8. HeaderBar (화면 헤더바)

화면별 버튼 구성:

| 화면 | 왼쪽 | 제목 | 오른쪽 |
|------|------|------|--------|
| 메인 | - | "C Chat" | 친구추가, 방만들기, 설정 |
| 채팅(그룹) | ← 뒤로 | 방이름 / "멤버N명" | 📌 공지, 🔍 검색, 👥 멤버, ⋯ 더보기 |
| 채팅(DM) | ← 뒤로 | 상대닉네임 / 상태 | 🔍 검색, ⋯ 더보기 |
| 마이페이지 | ← 뒤로 | "마이페이지" | ✏ 편집 |
| 설정 | ← 뒤로 | "설정" | - |

```c
GtkWidget *headerbar = gtk_header_bar_new();
/* 뒤로가기 버튼 */
GtkWidget *back_btn = gtk_button_new_from_icon_name("go-previous-symbolic");
gtk_header_bar_pack_start(GTK_HEADER_BAR(headerbar), back_btn);
/* 더보기 메뉴버튼 */
GtkWidget *menu_btn = gtk_menu_button_new();
gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(menu_btn), "view-more-symbolic");
gtk_header_bar_pack_end(GTK_HEADER_BAR(headerbar), menu_btn);
```

---

## 9. Composer (메시지 입력창)

GtkEntry + 전송 GtkButton. 답장 모드, 커맨드 자동완성 포함.

- 답장 모드: Composer 위에 GtkRevealer(슬라이드-인) 답장 미리보기 바
- "/" 입력 시: GtkPopover (CommandSuggestion) 자동 표시
- 빈 입력: 전송 버튼 비활성 (`gtk_widget_set_sensitive`)
- 연결 끊김: entry 비활성화

```c
GtkWidget *input_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
/* 답장 미리보기 */
GtkWidget *reply_revealer = gtk_revealer_new();
gtk_revealer_set_transition_type(GTK_REVEALER(reply_revealer),
    GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN);
GtkWidget *reply_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
gtk_widget_add_css_class(reply_bar, "reply-preview-bar");
/* 입력창 행 */
GtkWidget *entry_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
GtkWidget *msg_entry = gtk_entry_new();
gtk_widget_add_css_class(msg_entry, "message-entry");
gtk_widget_set_hexpand(msg_entry, TRUE);
GtkWidget *send_btn = gtk_button_new_from_icon_name("mail-send-symbolic");
gtk_widget_add_css_class(send_btn, "suggested-action");
/* "/" 입력 감지 → CommandSuggestion GtkPopover 표시 */
g_signal_connect(msg_entry, "changed", G_CALLBACK(on_entry_changed), send_btn);
```

### CommandSuggestion (커맨드 자동완성)

입력창에서 `/` 타이핑 시 `GtkPopover`로 자동완성 목록 표시.

```c
GtkWidget *suggest_pop  = gtk_popover_new(message_entry);
GtkWidget *suggest_list = gtk_list_box_new();

/* 현재 입력에 맞는 커맨드 필터링 후 GtkListBoxRow 추가 */
for each matching_cmd:
    GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
    GtkWidget *cmd_lbl = gtk_label_new(cmd_name);   /* bold */
    GtkWidget *arg_lbl = gtk_label_new(cmd_args);   /* dim */
    GtkWidget *desc    = gtk_label_new(cmd_desc);   /* dim, hexpand */
```

- 커맨드 목록: `/w` (귓속말), `/me` (액션), `/edit` (수정 모드), `/delete` (삭제)
- 최대 5개 표시, 나머지는 스크롤.
- 현재 화면에서 사용 불가 커맨드: CSS class `disabled` + `gtk_widget_set_sensitive(row, FALSE)`.
- Tab/↑↓로 선택, Enter 확정, Escape 닫기.

---

## 10. NotificationBanner (알림 배너)

`GtkRevealer + GtkBox`를 사용한 알림 컴포넌트 (GTK4에서 GtkInfoBar 제거됨).

```c
GtkWidget *revealer = gtk_revealer_new();
gtk_revealer_set_transition_type(GTK_REVEALER(revealer),
    GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN);

GtkWidget *bar_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
gtk_widget_add_css_class(bar_box, "notification-bar");
gtk_widget_add_css_class(bar_box, "notification-info");  /* info/error/other */

GtkWidget *msg_lbl     = gtk_label_new(message_text);
GtkWidget *accept_btn  = gtk_button_new_with_label("수락");
GtkWidget *reject_btn  = gtk_button_new_with_label("거절");

gtk_box_append(GTK_BOX(bar_box), msg_lbl);
gtk_box_append(GTK_BOX(bar_box), accept_btn);
gtk_box_append(GTK_BOX(bar_box), reject_btn);
gtk_revealer_set_child(GTK_REVEALER(revealer), bar_box);

/* 표시 */
gtk_revealer_set_reveal_child(GTK_REVEALER(revealer), TRUE);

/* 자동 해제 (5초) */
g_timeout_add(5000, (GSourceFunc)hide_notification_cb, revealer);
```

| 알림 유형 | CSS class | 자동 해제 |
|-----------|-----------|----------|
| 친구 요청 | `notification-info` | 10초 |
| 멘션 알림 | `notification-other` | 5초 |
| 일반 정보 | `notification-info` | 5초 |
| 연결 끊김 | `notification-error` | 수동만 가능 |

- 최대 3개 스택: `GtkBox (vertical)` 안에 여러 GtkRevealer 추가.

---

## 11. MessageBubble (좌/우 정렬 말풍선)

메시지 하나를 표현하는 복합 위젯. `GtkListBox` 내 `GtkListBoxRow`에 삽입.

### 상대방 메시지 (좌측 정렬)

```c
/* 최상위: GtkBox horizontal, halign=START */
GtkWidget *row_box   = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
GtkWidget *avatar    = make_avatar_label(sender_nick);
GtkWidget *content   = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
GtkWidget *nick_lbl  = gtk_label_new(sender_nick);   /* 그룹핑 첫 메시지만 */
GtkWidget *bubble    = gtk_label_new(message_text);
GtkWidget *meta_box  = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
GtkWidget *unread    = gtk_label_new("3");            /* unread count */
GtkWidget *timestamp = gtk_label_new("14:02");

gtk_style_context_add_class(gtk_widget_get_style_context(bubble), "bubble-other");
gtk_label_set_line_wrap(GTK_LABEL(bubble), TRUE);
gtk_label_set_xalign(GTK_LABEL(nick_lbl), 0.0);
```

### 내 메시지 (우측 정렬)

```c
GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
gtk_widget_set_halign(row_box, GTK_ALIGN_END);
/* meta_box (unread + timestamp)를 bubble 왼쪽에 배치 */
GtkWidget *bubble = gtk_label_new(message_text);
gtk_style_context_add_class(gtk_widget_get_style_context(bubble), "bubble-self");
```

CSS:
```css
.bubble-other {
  background-color: @bg_surface;
  color: @fg_primary;
  border-radius: 0 12px 12px 12px;
  padding: 8px 12px;
  max-width: 480px;
}
.bubble-self {
  background-color: @accent_primary;
  color: @fg_on_accent;
  border-radius: 12px 12px 0 12px;
  padding: 8px 12px;
  max-width: 480px;
}
```

### 그룹핑 규칙

- 같은 발신자의 연속 메시지가 **180초 이내**: 두 번째부터 avatar + nick GtkLabel을 `gtk_widget_hide()`로 숨김.
- 시간·미읽음 수 GtkLabel은 **그룹 마지막 메시지에만** 표시.

---

## 12. MessageDecorations (메시지 장식 변형)

### 답장 인용 (GtkFrame)

```c
GtkWidget *quote_frame = gtk_frame_new(NULL);
gtk_style_context_add_class(ctx, "reply-quote");
GtkWidget *quote_lbl   = gtk_label_new(original_text_preview);
gtk_label_set_max_width_chars(GTK_LABEL(quote_lbl), 40);
gtk_label_set_ellipsize(GTK_LABEL(quote_lbl), PANGO_ELLIPSIZE_END);
```

CSS:
```css
.reply-quote {
  border-left: 3px solid @accent_primary;
  padding: 4px 8px;
  background-color: @bg_surface;
  border-radius: 0 4px 4px 0;
}
```

### 삭제된 메시지

```c
gtk_label_set_markup(bubble_lbl,
    "<i><span color='@fg_tertiary'>(삭제된 메시지)</span></i>");
```

### 수정된 메시지

```c
gchar *ts_text = g_strdup_printf("%s · 수정됨", time_str);
gtk_label_set_text(GTK_LABEL(timestamp_lbl), ts_text);
gtk_style_context_add_class(ctx_ts, "dim-label");
```

### 멘션 강조

```c
/* gtk_label_set_markup으로 @닉네임 부분만 강조 */
gchar *markup = g_markup_printf_escaped(
    "%s<span color='#89b4fa' font_weight='bold'>@%s</span>%s",
    before, mention_nick, after);
gtk_label_set_markup(GTK_LABEL(bubble_lbl), markup);
```

---

## 13. TypingIndicator (타이핑 표시)

```c
GtkWidget *typing_lbl = gtk_label_new("");
gtk_style_context_add_class(ctx, "typing-indicator");
gtk_widget_hide(typing_lbl);  /* 초기 숨김 */

/* TYPING_NOTIFY 수신 시 */
gtk_label_set_text(GTK_LABEL(typing_lbl), "홍길동 님이 입력 중...");
gtk_widget_show(typing_lbl);

/* TYPING_STOP 또는 5초 타임아웃 */
gtk_widget_hide(typing_lbl);
```

- 복수 타이핑: `"홍길동 외 2명이 입력 중..."`.
- 입력창 바로 위에 배치.

CSS:
```css
.typing-indicator {
  color: @fg_secondary;
  font-size: 12px;
  font-style: italic;
  margin: 2px 12px;
}
```

---

## 14. DateSeparator (날짜 구분선)

날짜가 바뀌는 메시지 경계에 삽입.
GtkBox (horizontal): GtkSeparator[hexpand] + GtkLabel(날짜) + GtkSeparator[hexpand]

```c
GtkWidget *sep_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
gtk_widget_add_css_class(sep_box, "date-separator");
GtkWidget *line1 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
GtkWidget *line2 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
GtkWidget *date_lbl = gtk_label_new("2026년 4월 20일 월요일");
gtk_widget_add_css_class(date_lbl, "date-label");
gtk_widget_set_hexpand(line1, TRUE);
gtk_widget_set_hexpand(line2, TRUE);
```

CSS: `.date-label { font-size: 11px; color: @fg_tertiary; padding: 0 8px; }`

---

## 15. SystemMessage (시스템 이벤트 메시지)

입장/퇴장/이름변경/공지 등 시스템 이벤트.
GtkLabel (halign=CENTER, css: system-message)

```c
GtkWidget *sys_lbl = gtk_label_new("홍길동 님이 입장했습니다");
gtk_widget_set_halign(sys_lbl, GTK_ALIGN_CENTER);
gtk_widget_add_css_class(sys_lbl, "system-message");
gtk_label_set_selectable(GTK_LABEL(sys_lbl), FALSE);
```

CSS: `.system-message { color: @fg_tertiary; font-size: 12px; padding: 4px 0; }`

---

## 16. MembersPanel (멤버 목록 팝오버)

HeaderBar 멤버 버튼에 연결된 `GtkPopover`.

```c
GtkWidget *popover   = gtk_popover_new(member_btn);
GtkWidget *pop_box   = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
GtkWidget *header    = make_member_header(member_count);
GtkWidget *scroll    = gtk_scrolled_window_new(NULL, NULL);
GtkWidget *list      = gtk_list_box_new();

gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll), 300);
gtk_scrolled_window_set_max_content_height(GTK_SCROLLED_WINDOW(scroll), 500);
```

**MemberItem (GtkListBoxRow 내부):**

```c
GtkWidget *item_box  = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
GtkWidget *avatar    = make_avatar_label(nick);
GtkWidget *info_box  = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
GtkWidget *nick_lbl  = gtk_label_new(nickname);
GtkWidget *dot       = make_status_dot(status);
GtkWidget *role_icon = gtk_label_new("★");  /* 방장, ☆ 공동방장, "" 일반 */
```

섹션 헤더: `gtk_list_box_set_header_func()`으로 "방장" / "관리자" / "멤버 (N)" 구분.

**MemberItem 우클릭 → GtkPopoverMenu:**
- `DM 보내기` (항상)
- `강퇴` (방장/관리자 전용, GtkAlertDialog 확인)
- `관리자 권한 부여/해제` (방장 전용)

---

## 17. SearchOverlay (메시지 검색 오버레이)

```c
GtkWidget *search_bar   = gtk_search_bar_new();
GtkWidget *search_entry = gtk_search_entry_new();
gtk_search_bar_connect_entry(GTK_SEARCH_BAR(search_bar), GTK_ENTRY(search_entry));
gtk_search_bar_set_show_close_button(GTK_SEARCH_BAR(search_bar), TRUE);

/* 검색어 변경 시그널 (300ms 디바운스) */
g_signal_connect(search_entry, "search-changed", G_CALLBACK(on_search_changed), NULL);
```

- HeaderBar 검색 버튼 또는 Ctrl+F → `gtk_search_bar_set_search_mode(bar, TRUE)`.
- Escape → `gtk_search_bar_set_search_mode(bar, FALSE)`.

---

## 18. EmptyState (빈 상태 안내)

```c
GtkWidget *empty_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
gtk_widget_set_halign(empty_box, GTK_ALIGN_CENTER);
gtk_widget_set_valign(empty_box, GTK_ALIGN_CENTER);

GtkWidget *main_lbl = gtk_label_new("아직 친구가 없습니다.");
GtkWidget *sub_lbl  = gtk_label_new("친구 추가 버튼으로 첫 친구를 추가해보세요.");
gtk_style_context_add_class(ctx_sub, "dim-label");
```

- 2행 구성: 상태 설명 + 행동 유도(dim).
- 위젯 중앙 정렬.

---

## 19. RoomCard (오픈채팅 방 카드)

```c
GtkWidget *card_box   = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
GtkWidget *top_row    = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
GtkWidget *name_lbl   = gtk_label_new(room_name);
GtkWidget *count_lbl  = gtk_label_new("8/30");        /* meta-label */
GtkWidget *lock_icon  = gtk_image_new_from_icon_name( /* 비번 방만 */
                            "changes-prevent-symbolic"); /* GTK4: size enum 제거됨 */
GtkWidget *topic_lbl  = gtk_label_new(room_topic);    /* dim-label */

gtk_label_set_max_width_chars(GTK_LABEL(topic_lbl), 50);
gtk_label_set_ellipsize(GTK_LABEL(topic_lbl), PANGO_ELLIPSIZE_END);
gtk_style_context_add_class(ctx_name, "h3");
```

- 만원인 방: `count_lbl`에 `error` CSS class + `[FULL]` GtkLabel 추가.
- 참여 중인 방: 우측에 `[참여중]` GtkLabel (CSS class: `status-chip-online`).

---

## 20. Modal (확인 다이얼로그)

강퇴·삭제 등 위험 액션에 사용하는 표준 확인 다이얼로그 (GTK4 GtkAlertDialog 사용).

```c
GtkAlertDialog *dialog = gtk_alert_dialog_new("홍길동 님을 강퇴하시겠어요?");
const char *buttons[] = {"취소", "강퇴", NULL};
gtk_alert_dialog_set_buttons(dialog, buttons);
gtk_alert_dialog_set_cancel_button(dialog, 0);   /* 취소 = 인덱스 0 */
gtk_alert_dialog_set_default_button(dialog, 0);  /* 기본 포커스: 덜 위험한 쪽 */

gtk_alert_dialog_choose(dialog, GTK_WINDOW(parent), NULL,
                        on_confirm_response, user_data);
```

- 기본 포커스: 덜 위험한 쪽 (`취소`).
- Escape = 취소, Enter = 포커스 버튼.

---

## 21. Toast (하단 스낵바)

짧은 피드백 메시지 (GTK4에서 GtkInfoBar 제거됨 → GtkRevealer 사용).

```c
/* GtkOverlay의 오버레이로 하단에 표시 */
GtkWidget *revealer = gtk_revealer_new();
gtk_revealer_set_transition_type(GTK_REVEALER(revealer),
    GTK_REVEALER_TRANSITION_TYPE_CROSSFADE);

GtkWidget *toast_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
gtk_widget_add_css_class(toast_box, "toast");
gtk_box_append(GTK_BOX(toast_box), gtk_label_new("복사되었습니다."));
gtk_revealer_set_child(GTK_REVEALER(revealer), toast_box);

gtk_widget_set_halign(revealer, GTK_ALIGN_CENTER);
gtk_widget_set_valign(revealer, GTK_ALIGN_END);

/* 표시 */
gtk_revealer_set_reveal_child(GTK_REVEALER(revealer), TRUE);

/* 2초 후 자동 해제 */
g_timeout_add(2000, (GSourceFunc)hide_toast_cb, revealer);
```

- 성공: CSS class `toast-info`.
- 오류: CSS class `toast-error`.

---

## 22. PinNoticeBar (공지/핀 표시 바)

```c
GtkWidget *revealer  = gtk_revealer_new();
GtkWidget *notice_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
gtk_widget_add_css_class(notice_box, "notice-bar");

GtkWidget *icon_lbl = gtk_label_new("◈");
GtkWidget *text_lbl = gtk_label_new(notice_text);
GtkWidget *more_btn = gtk_button_new_with_label("전체 보기");

gtk_label_set_ellipsize(GTK_LABEL(text_lbl), PANGO_ELLIPSIZE_END);
gtk_label_set_max_width_chars(GTK_LABEL(text_lbl), 60);
gtk_widget_set_hexpand(text_lbl, TRUE);

gtk_box_append(GTK_BOX(notice_box), icon_lbl);
gtk_box_append(GTK_BOX(notice_box), text_lbl);
gtk_box_append(GTK_BOX(notice_box), more_btn);
gtk_revealer_set_child(GTK_REVEALER(revealer), notice_box);
```

- 공지 없으면 `gtk_revealer_set_reveal_child(revealer, FALSE)`.
- `[전체 보기]` 클릭 → GtkWindow로 공지 전문 표시.
