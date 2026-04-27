# 알림 패널

## 1. 진입 경로 및 위젯 구조

헤더바 우측에 위치한 🔔 `GtkButton`을 클릭하면 `GtkPopover`가 표시된다.

**진입 경로**
- `GtkHeaderBar` → 🔔 `GtkButton` (id: `notif_btn`) 클릭
- `GtkOverlay`로 뱃지(`GtkLabel.unread-badge`) 중첩 표시

**팝오버 구조**

```
GtkPopover (min-width: 320px)
└── GtkBox [VERTICAL]
    ├── GtkBox [HORIZONTAL]  ← 팝오버 헤더
    │   ├── GtkLabel "알림"
    │   └── GtkButton.flat "모두 읽음"
    └── GtkScrolledWindow
        └── GtkStack
            ├── "list"  → GtkListBox (.notification-list)
            └── "empty" → GtkBox(CENTER) [빈 상태]
```

**팝오버 설정**

```c
GtkWidget *popover = gtk_popover_new();
gtk_widget_set_size_request(popover, 320, -1);
gtk_popover_set_position(GTK_POPOVER(popover), GTK_POS_BOTTOM);
gtk_widget_set_parent(popover, notif_btn);

/* 외부 클릭 또는 Escape로 닫힘 (GTK4 기본 동작) */
gtk_popover_set_autohide(GTK_POPOVER(popover), TRUE);
```

---

## 2. 알림 유형별 행 구성

각 알림 행은 `GtkListBoxRow`로 구성되며, 내부 레이아웃은 다음과 같다.

```
GtkListBoxRow (.notification-row)
└── GtkBox [HORIZONTAL, spacing=8, margin=10]
    ├── GtkLabel (.notif-icon)       ← 아이콘 (이모지 또는 문자)
    └── GtkBox [VERTICAL, spacing=2]
        ├── GtkLabel (.notif-title)  ← 알림 제목
        ├── GtkLabel (.notif-body)   ← 본문 미리보기 (40자 ellipsize)
        └── GtkLabel (.notif-time)   ← 상대 시간 (예: "3분 전")
```

**본문 미리보기 ellipsize 설정**

```c
gtk_label_set_max_width_chars(GTK_LABEL(body_label), 40);
gtk_label_set_ellipsize(GTK_LABEL(body_label), PANGO_ELLIPSIZE_END);
gtk_label_set_xalign(GTK_LABEL(body_label), 0.0f);
```

### 2-1. 친구 요청 (FRIEND_REQUEST_NOTIFY)

- **아이콘**: `👤`
- **제목**: `"{nickname}님이 친구 요청을 보냈습니다"`
- **본문**: 없음 (빈 문자열)
- **액션 버튼**: `[수락]` `[거절]` — 행 하단에 `GtkBox [HORIZONTAL]`로 배치

```c
/* 친구 요청 행 액션 버튼 */
GtkWidget *action_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
GtkWidget *accept_btn = gtk_button_new_with_label("수락");
GtkWidget *reject_btn = gtk_button_new_with_label("거절");
gtk_widget_add_css_class(accept_btn, "suggested-action");
gtk_widget_add_css_class(reject_btn, "destructive-action");
gtk_box_append(GTK_BOX(action_box), accept_btn);
gtk_box_append(GTK_BOX(action_box), reject_btn);
```

### 2-2. 멘션 (SYSTEM_NOTIFY level=2)

- **아이콘**: `@`
- **제목**: `"{nickname}님이 {room_name}에서 멘션했습니다"`
- **본문**: 멘션된 메시지 내용 (40자 ellipsize)
- **액션 버튼**: `[이동]` — 해당 채팅방으로 이동

### 2-3. DM 수신 (DM_NEW_NOTIFY)

- **아이콘**: `✉`
- **제목**: `"{nickname}님의 메시지"`
- **본문**: DM 내용 미리보기 (40자 ellipsize)
- **액션 버튼**: 없음 (행 클릭 시 DM 채팅 화면으로 이동)

### 2-4. 방 초대 (ROOM_INVITED_NOTIFY)

- **아이콘**: `#`
- **제목**: `"{nickname}님이 {room_name}에 초대했습니다"`
- **본문**: 없음
- **액션 버튼**: `[수락]` `[거절]`

### 2-5. 시스템 공지 (SYSTEM_NOTIFY level=0,1)

- **아이콘**: `⚙`
- **제목**: `"시스템 공지"` (level=0) / `"경고"` (level=1)
- **본문**: 공지 내용 (40자 ellipsize)
- **액션 버튼**: 없음
- **배경색**: level별 CSS 클래스로 구분

| level | CSS 클래스 | 배경 의미 |
|-------|-----------|---------|
| 0 | `.notif-system-info` | 일반 공지 (기본 배경) |
| 1 | `.notif-system-warn` | 경고 (노란 계열) |
| 2 | `.notif-system-mention` | 멘션 (파란 계열) |

---

## 3. 읽음 처리

**미읽음 행**: `accent` 배경색 적용

```css
/* CSS */
.notification-row.unread {
    background-color: alpha(@accent_bg_color, 0.15);
}
```

```c
/* C 코드: 미읽음 표시 */
if (!notif->is_read) {
    gtk_widget_add_css_class(row, "unread");
}
```

**"모두 읽음" 버튼 동작**

```c
static void on_mark_all_read(GtkButton *btn, gpointer user_data) {
    AppState *app = user_data;

    /* 모든 알림을 읽음 처리 */
    for (guint i = 0; i < app->notifications->len; i++) {
        Notification *n = g_ptr_array_index(app->notifications, i);
        n->is_read = TRUE;
    }

    /* 미읽음 CSS 클래스 제거 */
    GtkWidget *child = gtk_widget_get_first_child(app->notif_listbox);
    while (child != NULL) {
        gtk_widget_remove_css_class(child, "unread");
        child = gtk_widget_get_next_sibling(child);
    }

    /* 뱃지 숨기기 */
    update_unread_badge(app, 0);
}
```

**개별 알림 읽음 처리**: 행 클릭 시 해당 알림의 `is_read = TRUE` 설정 후 CSS 클래스 제거.

---

## 4. 실시간 업데이트

recv 스레드는 GTK 위젯을 직접 조작할 수 없으므로 `g_idle_add`를 통해 메인 스레드에서 처리한다.

**알림 데이터 구조**

```c
/* chat_program/src/common/types.h 또는 chat_program/src/client/notify.h */
typedef struct {
    NotifType type;      /* FRIEND_REQUEST_NOTIFY, DM_NEW_NOTIFY 등 */
    int       level;     /* SYSTEM_NOTIFY의 경우 0,1,2 */
    char      from_id[21];
    char      title[128];
    char      body[256];
    char      room_id[21];  /* 방 초대, 멘션의 경우 */
    time_t    timestamp;
    gboolean  is_read;
} Notification;
```

**알림 저장소**: `GPtrArray` 사용, 최대 50개 유지

```c
/* chat_program/src/client/notify.c */
#define MAX_NOTIFICATIONS 50

void notifications_add(AppState *app, Notification *notif) {
    /* 최대 개수 초과 시 가장 오래된 항목 제거 */
    if (app->notifications->len >= MAX_NOTIFICATIONS) {
        Notification *oldest = g_ptr_array_index(app->notifications, 0);
        g_ptr_array_remove_index(app->notifications, 0);
        g_free(oldest);
    }
    g_ptr_array_add(app->notifications, notif);
}
```

**recv 스레드 → 메인 스레드 전달**

```c
/* recv 스레드에서 호출 */
typedef struct {
    AppState    *app;
    Notification *notif;
} NotifIdleData;

static gboolean notif_idle_cb(gpointer user_data) {
    NotifIdleData *data = user_data;
    AppState      *app  = data->app;
    Notification  *notif = data->notif;

    /* 알림 저장 */
    notifications_add(app, notif);

    /* 리스트 최상단에 행 추가 */
    GtkWidget *row = build_notification_row(notif);
    gtk_list_box_prepend(GTK_LIST_BOX(app->notif_listbox), row);

    /* 빈 상태 스택 전환 */
    gtk_stack_set_visible_child_name(GTK_STACK(app->notif_stack), "list");

    /* 뱃지 업데이트 */
    app->unread_count++;
    update_unread_badge(app, app->unread_count);

    g_free(data);
    return G_SOURCE_REMOVE;  /* 한 번만 실행 */
}

/* recv 스레드 내부 */
void on_notification_received(AppState *app, Notification *notif) {
    NotifIdleData *data = g_new(NotifIdleData, 1);
    data->app   = app;
    data->notif = notif;
    g_idle_add(notif_idle_cb, data);
}
```

---

## 5. 알림 클릭 동작

`GtkListBox`의 `row-activated` 시그널에 연결.

```c
g_signal_connect(app->notif_listbox, "row-activated",
                 G_CALLBACK(on_notif_row_activated), app);

static void on_notif_row_activated(GtkListBox *listbox,
                                   GtkListBoxRow *row,
                                   gpointer user_data) {
    AppState     *app   = user_data;
    Notification *notif = g_object_get_data(G_OBJECT(row), "notification");

    if (!notif) return;

    /* 읽음 처리 */
    notif->is_read = TRUE;
    gtk_widget_remove_css_class(GTK_WIDGET(row), "unread");
    app->unread_count = MAX(0, app->unread_count - 1);
    update_unread_badge(app, app->unread_count);

    /* 팝오버 닫기 */
    gtk_popover_popdown(GTK_POPOVER(app->notif_popover));

    /* 유형별 화면 이동 */
    switch (notif->type) {
        case DM_NEW_NOTIFY:
            /* DM 채팅 화면으로 이동 */
            app_navigate_to_dm(app, notif->from_id);
            break;

        case FRIEND_REQUEST_NOTIFY:
            /* 친구 탭으로 이동 (수락/거절은 행의 버튼으로 처리) */
            app_navigate_to_friends_tab(app);
            break;

        case ROOM_INVITED_NOTIFY:
            /* 방 입장 확인 다이얼로그 표시 */
            show_room_join_dialog(app, notif->room_id, notif->title);
            break;

        case SYSTEM_NOTIFY:
            if (notif->level == 2) {
                /* 멘션: 해당 채팅방으로 이동 */
                app_navigate_to_room(app, notif->room_id);
            }
            /* level 0,1: 별도 화면 이동 없음 */
            break;

        default:
            break;
    }
}
```

**방 초대 확인 다이얼로그**

```c
static void show_room_join_dialog(AppState *app,
                                  const char *room_id,
                                  const char *room_name) {
    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(app->main_window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_NONE,
        "%s 방에 입장하시겠습니까?", room_name
    );
    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                           "거절", GTK_RESPONSE_REJECT,
                           "수락", GTK_RESPONSE_ACCEPT,
                           NULL);
    g_signal_connect(dialog, "response",
                     G_CALLBACK(on_room_invite_response), app);
    gtk_widget_show(dialog);
}
```

---

## 6. 빈 상태

알림이 없거나 모두 삭제된 경우 `GtkStack`의 `"empty"` 자식을 표시한다.

```c
/* 빈 상태 위젯 구성 */
GtkWidget *empty_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
gtk_widget_set_valign(empty_box, GTK_ALIGN_CENTER);
gtk_widget_set_halign(empty_box, GTK_ALIGN_CENTER);
gtk_widget_set_vexpand(empty_box, TRUE);

GtkWidget *empty_icon  = gtk_label_new("🔕");
GtkWidget *empty_label = gtk_label_new("알림이 없습니다");

gtk_widget_add_css_class(empty_icon,  "dim-label");
gtk_widget_add_css_class(empty_label, "dim-label");

/* empty_icon 폰트 크기 확대 (CSS로 처리 권장) */
gtk_widget_add_css_class(empty_icon, "notif-empty-icon");

gtk_box_append(GTK_BOX(empty_box), empty_icon);
gtk_box_append(GTK_BOX(empty_box), empty_label);

/* 스택에 추가 */
gtk_stack_add_named(GTK_STACK(notif_stack), empty_box, "empty");

/* 초기 상태 설정 */
gtk_stack_set_visible_child_name(GTK_STACK(notif_stack), "empty");
```

---

## 7. 헤더바 뱃지

`GtkOverlay`로 🔔 버튼 위에 미읽음 수 `GtkLabel`을 중첩 표시한다.

```c
/* 헤더바 뱃지 구성 */
GtkWidget *overlay    = gtk_overlay_new();
GtkWidget *notif_btn  = gtk_button_new_with_label("🔔");
GtkWidget *badge_label = gtk_label_new("0");

gtk_widget_add_css_class(notif_btn,   "flat");
gtk_widget_add_css_class(badge_label, "unread-badge");

/* 뱃지를 오른쪽 상단에 배치 */
gtk_widget_set_halign(badge_label, GTK_ALIGN_END);
gtk_widget_set_valign(badge_label, GTK_ALIGN_START);

gtk_overlay_set_child(GTK_OVERLAY(overlay), notif_btn);
gtk_overlay_add_overlay(GTK_OVERLAY(overlay), badge_label);

/* 헤더바에 추가 */
gtk_header_bar_pack_end(GTK_HEADER_BAR(headerbar), overlay);
```

**뱃지 업데이트 함수**

```c
void update_unread_badge(AppState *app, int count) {
    if (count <= 0) {
        gtk_widget_set_visible(app->badge_label, FALSE);
    } else {
        char buf[8];
        /* 99개 초과 시 "99+" 표시 */
        if (count > 99) {
            g_snprintf(buf, sizeof(buf), "99+");
        } else {
            g_snprintf(buf, sizeof(buf), "%d", count);
        }
        gtk_label_set_text(GTK_LABEL(app->badge_label), buf);
        gtk_widget_set_visible(app->badge_label, TRUE);
    }
}
```

---

## 8. CSS

```css
/* assets/style.css */

/* 알림 팝오버 헤더 */
.notification-header {
    padding: 8px 12px;
    border-bottom: 1px solid @borders;
}

/* 알림 리스트 */
.notification-list {
    background: transparent;
}

/* 알림 행 */
.notification-row {
    padding: 4px 0;
    border-bottom: 1px solid alpha(@borders, 0.5);
}

/* 미읽음 행 강조 */
.notification-row.unread {
    background-color: alpha(@accent_bg_color, 0.15);
}

/* 알림 아이콘 */
.notif-icon {
    font-size: 1.4rem;
    min-width: 32px;
}

/* 알림 제목 */
.notif-title {
    font-weight: bold;
    font-size: 0.9rem;
}

/* 알림 본문 미리보기 */
.notif-body {
    font-size: 0.85rem;
    color: alpha(@window_fg_color, 0.7);
}

/* 상대 시간 */
.notif-time {
    font-size: 0.75rem;
    color: alpha(@window_fg_color, 0.5);
}

/* 시스템 알림 level별 배경색 */
.notification-row.notif-system-warn {
    background-color: alpha(#f0c040, 0.12);
}

.notification-row.notif-system-mention {
    background-color: alpha(@accent_bg_color, 0.18);
}

/* 헤더바 미읽음 뱃지 */
.unread-badge {
    background-color: @error_color;
    color: white;
    font-size: 0.65rem;
    font-weight: bold;
    min-width: 16px;
    min-height: 16px;
    border-radius: 8px;
    padding: 0 3px;
    /* 버튼 우상단 위치 조정 */
    margin-top: 2px;
    margin-right: 2px;
}

/* 빈 상태 아이콘 크기 */
.notif-empty-icon {
    font-size: 2.5rem;
    margin-bottom: 4px;
}
```
