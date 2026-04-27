# 방 정보 및 설정

채팅방의 정보를 조회하고 방장이 설정을 변경할 수 있는 모달 다이얼로그.
`GtkWindow` (modal) 로 구현하며, 방장 여부에 따라 표시되는 탭이 달라진다.

---

## 1. 진입 경로

| 진입 위치 | 동작 |
|-----------|------|
| 채팅 헤더바 "⋯ 더보기" → "채팅방 정보" | 방 정보 모달 열기 |

모달 진입 즉시 `ROOM_INFO|<room_id>` 및 `ROOM_MEMBERS|<room_id>` 패킷을 서버에 전송한다.

---

## 2. 레이아웃 (GtkWindow)

### 윈도우 속성

| 속성 | 값 |
|------|----|
| 위젯 타입 | `GtkWindow` |
| `modal` | `TRUE` |
| `transient-for` | 메인 `GtkApplicationWindow` |
| `default-width` | 420 |
| `default-height` | 480 |
| `resizable` | `FALSE` |
| `title` | "채팅방 정보" |
| CSS class | `room-settings-dialog` |

### 위젯 트리

```
GtkWindow (modal, css: room-settings-dialog)
└── GtkBox (vertical, spacing=0)
    ├── GtkHeaderBar
    │   ├── title: GtkLabel "채팅방 정보"
    │   └── [우] GtkButton "닫기" (icon: window-close-symbolic)
    └── GtkNotebook (css: room-settings-notebook)
        ├── [탭 0] GtkLabel "정보"          — 탭 레이블
        │   └── 정보 탭 내용 (§3)
        ├── [탭 1] GtkLabel "관리"          — 탭 레이블 (방장만 표시)
        │   └── 관리 탭 내용 (§4)
        └── (비방장: 탭 1 숨김)
```

비방장 접속 시: `gtk_widget_hide(gtk_notebook_get_nth_page(notebook, 1))` 및 탭 레이블도 숨김.

---

## 3. 정보 탭

방의 기본 정보를 읽기 전용으로 표시한다. 모든 사용자에게 표시.

### 위젯 구성

```
GtkScrolledWindow (vscrollbar-policy=GTK_POLICY_AUTOMATIC)
└── GtkBox (vertical, spacing=16, margin=20, css: info-tab)
    ├── GtkBox (horizontal, spacing=12, css: room-hero)          — 방 요약
    │   ├── GtkLabel (방 이름 이니셜, css: room-avatar-label)
    │   └── GtkBox (vertical, spacing=4)
    │       ├── GtkLabel (방 이름, css: h3)
    │       └── GtkLabel ("공개" / "비공개", css: dim-label)
    ├── GtkSeparator (horizontal)
    ├── GtkGrid (column-spacing=12, row-spacing=8, css: info-grid)
    │   ├── [행 0] GtkLabel "주제"       (css: field-label)
    │   │         GtkLabel <topic>       (css: field-value, hexpand=TRUE)
    │   ├── [행 1] GtkLabel "인원수"     (css: field-label)
    │   │         GtkLabel "<n> / <max>명" (css: field-value)
    │   └── [행 2] GtkLabel "방장"       (css: field-label)
    │             GtkLabel <owner_nick>  (css: field-value)
    ├── GtkSeparator (horizontal)
    └── GtkBox (vertical, spacing=6, css: notice-section)        — 공지사항
        ├── GtkLabel "공지사항" (css: section-title)
        └── GtkLabel <notice_text>
            (css: notice-body, wrap=TRUE, selectable=TRUE)
            — 공지 없으면 GtkLabel "등록된 공지사항이 없습니다." (css: dim-label)
```

### 필드 상세

| 필드 | 출처 | 비고 |
|------|------|------|
| 방 이름 | `ROOM_INFO_RES` | — |
| 주제 | `ROOM_INFO_RES` | 없으면 `"(없음)"` |
| 공개 여부 | `ROOM_INFO_RES` | `is_public=1` → "공개", `0` → "비공개" |
| 인원수 | `ROOM_MEMBERS_RES` 응답 멤버 수 + `ROOM_INFO_RES` max_members | `<현재>/<최대>명` 형식 |
| 방장 | `ROOM_INFO_RES` owner_id → `ROOM_MEMBERS_RES` 에서 닉네임 조회 | — |
| 공지사항 | `ROOM_NOTICE` 패킷 또는 `ROOM_INFO_RES` notice 필드 | — |

---

## 4. 관리 탭 (방장 전용)

방장만 접근 가능. 방 설정 편집 및 방 삭제 기능을 제공한다.

### 위젯 구성

```
GtkScrolledWindow (vscrollbar-policy=GTK_POLICY_AUTOMATIC)
└── GtkBox (vertical, spacing=20, margin=20, css: admin-tab)
    ├── GtkBox (vertical, spacing=6, css: form-group)            — 방 이름
    │   ├── GtkLabel "방 이름" (css: field-label)
    │   └── GtkEntry (css: room-name-entry,
    │                 placeholder_text="방 이름 입력",
    │                 max_length=50)
    ├── GtkBox (vertical, spacing=6, css: form-group)            — 주제
    │   ├── GtkLabel "주제" (css: field-label)
    │   └── GtkEntry (css: room-topic-entry,
    │                 placeholder_text="주제 입력 (선택)",
    │                 max_length=100)
    ├── GtkBox (horizontal, spacing=12, css: form-group)         — 최대 인원
    │   ├── GtkLabel "최대 인원" (css: field-label, hexpand=TRUE)
    │   └── GtkSpinButton (min=2, max=64, step=1,
    │                      css: max-members-spin)
    ├── GtkBox (horizontal, spacing=12, css: form-group)         — 공개 여부
    │   ├── GtkLabel "공개방" (css: field-label, hexpand=TRUE)
    │   └── GtkSwitch (css: public-switch)
    ├── GtkBox (vertical, spacing=6, css: form-group)            — 비밀번호
    │   ├── GtkLabel "비밀번호" (css: field-label)
    │   └── GtkPasswordEntry (css: room-password-entry,
    │                         placeholder_text="새 비밀번호 (비우면 변경 없음)",
    │                         show-peek-icon=TRUE)
    ├── GtkButton "변경 사항 저장"
    │   (css: suggested-action, halign=END)
    ├── GtkSeparator (horizontal)
    └── GtkBox (vertical, spacing=8, css: danger-zone)           — 위험 구역
        ├── GtkLabel "위험 구역" (css: section-title danger-title)
        └── GtkButton "방 삭제"
            (css: destructive-action, halign=START)
```

### 변경 사항 저장 동작

```
[변경 사항 저장] 클릭
  → 입력값 검증 (방 이름 비어 있으면 GtkEntry shake 애니메이션 + Toast)
  → ROOM_EDIT|<room_id>|<name>|<topic>|<max_members>|<is_public>|<password> 전송
  → 응답 대기 중: 버튼 비활성화 + GtkSpinner 표시
  → ROOM_EDIT_RES|0 → Toast "저장되었습니다" + 정보 탭 갱신
  → ROOM_EDIT_RES|<err> → Toast "저장 실패: <사유>"
```

### 방 삭제 확인 다이얼로그

방 삭제 버튼 클릭 시 `GtkAlertDialog`로 2단계 확인:

```c
GtkAlertDialog *alert = gtk_alert_dialog_new(
    "방을 삭제하시겠어요?");
gtk_alert_dialog_set_detail(alert,
    "방을 삭제하면 모든 대화 내용이 사라지며 복구할 수 없습니다.");
const char *buttons[] = { "취소", "삭제", NULL };
gtk_alert_dialog_set_buttons(alert, buttons);
gtk_alert_dialog_set_cancel_button(alert, 0);   /* "취소" */
gtk_alert_dialog_set_default_button(alert, 0);  /* 기본: 취소 */
gtk_alert_dialog_choose(alert, GTK_WINDOW(parent),
                        NULL, on_delete_room_response, user_data);
```

**콜백 처리:**

```c
static void
on_delete_room_response(GObject *source, GAsyncResult *result,
                        gpointer user_data)
{
    GtkAlertDialog *alert = GTK_ALERT_DIALOG(source);
    int button = gtk_alert_dialog_choose_finish(alert, result, NULL);

    if (button == 1) {   /* "삭제" 선택 */
        RoomSettingsData *d = user_data;
        /* ROOM_DELETE|<room_id> 전송 */
        send_packet(d->sock_fd, "ROOM_DELETE", d->room_id_str);
        /* 모달 닫기 + 채팅 화면 → 메인 화면으로 전환 */
        gtk_window_destroy(GTK_WINDOW(d->dialog));
        screen_main_show(d->app_data);
    }
    /* button == 0 ("취소"): 아무 동작 없음 */
}
```

---

## 5. 데이터 흐름

### 진입 시 패킷 흐름

```
클라이언트                                서버
─────────────────────────────────────────────────────
ROOM_INFO|<room_id>              →
                                 ←  ROOM_INFO_RES|<room_id>|<name>|<topic>|
                                                  <is_public>|<max_members>|
                                                  <owner_id>|<notice>
ROOM_MEMBERS|<room_id>           →
                                 ←  ROOM_MEMBERS_RES|<room_id>|
                                        <id1>:<nick1>;<id2>:<nick2>;...
```

응답 도착 전까지 탭 내용 영역에 `GtkSpinner` 표시.

### 저장 시 패킷 흐름

```
클라이언트                                서버
─────────────────────────────────────────────────────
ROOM_EDIT|<room_id>|<name>|      →
          <topic>|<max>|
          <is_public>|<pw>
                                 ←  ROOM_EDIT_RES|<code>
                                        0  = 성공
                                        1  = 권한 없음
                                        2  = 이름 중복
                                        9  = 서버 오류
```

### 방 삭제 패킷 흐름

```
클라이언트                                서버
─────────────────────────────────────────────────────
ROOM_DELETE|<room_id>            →
                                 ←  ROOM_DELETE_RES|0        (성공)
                                 →  (서버가 모든 멤버에게 브로드캐스트)
                                 ←  ROOM_NOTICE|<room_id>|DELETED
```

### 공지사항 수신

```
                                 ←  ROOM_NOTICE|<room_id>|<notice_text>
```

`ROOM_NOTICE` 수신 시 정보 탭의 공지 GtkLabel을 실시간으로 갱신:

```c
gtk_label_set_text(GTK_LABEL(notice_label), notice_text);
```

---

## 6. GTK4 C 코드 예시

### 모달 생성 및 표시

```c
GtkWidget *
room_settings_dialog_new(GtkWindow *parent, AppData *app,
                         const char *room_id)
{
    GtkWidget *dialog = gtk_window_new();
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
    gtk_window_set_title(GTK_WINDOW(dialog), "채팅방 정보");
    gtk_window_set_default_size(GTK_WINDOW(dialog), 420, 480);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

    GtkWidget *root_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(dialog), root_box);

    /* HeaderBar */
    GtkWidget *header = gtk_header_bar_new();
    gtk_window_set_titlebar(GTK_WINDOW(dialog), header);
    GtkWidget *close_btn = gtk_button_new_from_icon_name(
                               "window-close-symbolic");
    g_signal_connect_swapped(close_btn, "clicked",
                             G_CALLBACK(gtk_window_destroy), dialog);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), close_btn);

    /* Notebook */
    GtkWidget *notebook = gtk_notebook_new();
    gtk_box_append(GTK_BOX(root_box), notebook);

    GtkWidget *info_page  = build_info_tab(app, room_id);
    GtkWidget *admin_page = build_admin_tab(app, room_id);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), info_page,
                             gtk_label_new("정보"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), admin_page,
                             gtk_label_new("관리"));

    /* 방장이 아니면 관리 탭 숨김 */
    if (!is_room_owner(app, room_id)) {
        GtkWidget *tab_label =
            gtk_notebook_get_tab_label(GTK_NOTEBOOK(notebook), admin_page);
        gtk_widget_hide(admin_page);
        gtk_widget_hide(tab_label);
    }

    /* 데이터 요청 */
    send_packet(app->sock_fd, "ROOM_INFO", room_id);
    send_packet(app->sock_fd, "ROOM_MEMBERS", room_id);

    return dialog;
}
```

### GtkSpinButton 초기화 (최대 인원)

```c
GtkAdjustment *adj = gtk_adjustment_new(
    32,   /* value (현재 최대 인원) */
    2,    /* lower */
    64,   /* upper */
    1,    /* step_increment */
    10,   /* page_increment */
    0     /* page_size */
);
GtkWidget *spin = gtk_spin_button_new(adj, 1.0, 0);
gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin), TRUE);
```

### 정보 탭 GtkGrid 구성

```c
GtkWidget *grid = gtk_grid_new();
gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

static const char *labels[] = { "주제", "인원수", "방장" };
for (int i = 0; i < 3; i++) {
    GtkWidget *lbl = gtk_label_new(labels[i]);
    gtk_widget_add_css_class(lbl, "field-label");
    gtk_label_set_xalign(GTK_LABEL(lbl), 0.0f);
    gtk_grid_attach(GTK_GRID(grid), lbl, 0, i, 1, 1);

    GtkWidget *val = gtk_label_new("—");
    gtk_widget_add_css_class(val, "field-value");
    gtk_label_set_xalign(GTK_LABEL(val), 0.0f);
    gtk_widget_set_hexpand(val, TRUE);
    gtk_grid_attach(GTK_GRID(grid), val, 1, i, 1, 1);
}
```

---

## 7. 접근성 / 단축키

| 단축키 | 동작 |
|--------|------|
| `Escape` | 모달 닫기 |
| `Tab` / `Shift+Tab` | 포커스 순환 |
| `Enter` (GtkEntry 포커스) | "변경 사항 저장" 동작 트리거 |

```c
/* Escape 키로 모달 닫기 */
GtkEventController *key_ctrl = gtk_event_controller_key_new();
g_signal_connect_swapped(key_ctrl, "key-pressed",
                         G_CALLBACK(on_dialog_key_pressed), dialog);
gtk_widget_add_controller(dialog, key_ctrl);

static gboolean
on_dialog_key_pressed(GtkWidget *dialog, guint keyval,
                      guint keycode, GdkModifierType state,
                      GtkEventControllerKey *ctrl)
{
    if (keyval == GDK_KEY_Escape) {
        gtk_window_destroy(GTK_WINDOW(dialog));
        return TRUE;
    }
    return FALSE;
}
```

- `GtkEntry` / `GtkSpinButton` 에 `gtk_accessible_update_property()` 로
  `GTK_ACCESSIBLE_PROPERTY_LABEL` 설정.
  - 예: `"방 이름 입력란"`, `"최대 인원 수 설정"`
- 방 삭제 버튼: `GTK_ACCESSIBLE_PROPERTY_DESCRIPTION` →
  `"방을 영구적으로 삭제합니다. 이 작업은 되돌릴 수 없습니다."`
