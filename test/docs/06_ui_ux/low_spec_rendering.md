# GTK4 클라이언트 성능 가이드

GTK4 기반 클라이언트가 일반 데스크탑 환경에서 60fps 수준의 응답성을 유지하기 위한 지침.

---

## 1. 스레딩 UI 업데이트

**규칙**: recv 스레드에서 GTK 위젯에 직접 접근 금지. 반드시 `g_idle_add()`로 GTK 메인 루프에 위임.

`GSourceFunc` 콜백은 GTK 메인 스레드에서 실행되므로 위젯 접근이 안전하다.

```c
typedef struct {
    char sender[21];
    char content[501];
} MsgData;

static gboolean append_message_cb(gpointer user_data) {
    MsgData *d = user_data;
    screen_chat_append_message(d->sender, d->content);
    g_free(d);
    return G_SOURCE_REMOVE;  /* 1회 실행 후 소스 제거 */
}

/* recv 스레드에서 호출: */
MsgData *d = g_new(MsgData, 1);
strncpy(d->sender,  sender,  20);
strncpy(d->content, content, 500);
g_idle_add(append_message_cb, d);
```

> `g_idle_add`는 스레드 세이프. `d`는 힙 할당 후 콜백에서 `g_free`.

---

## 2. GtkTextBuffer 메시지 추가

채팅 메시지 표시에는 `GtkTextView` + `GtkTextBuffer` 사용.

```c
/* 단일 메시지 추가 */
GtkTextIter end;
gtk_text_buffer_get_end_iter(buffer, &end);
gtk_text_buffer_insert_with_tags(buffer, &end,
    formatted_line, -1, tag_normal, NULL);

/* 자동 스크롤 */
gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(text_view),
    &end, 0.0, FALSE, 0.0, 1.0);
```

**대량 히스토리 로드** (100개 이상):  
`g_idle_add` 배치로 분할 추가. 한 번에 100개씩 처리 후 UI가 갱신될 틈을 줌.

```c
typedef struct {
    GPtrArray *messages;
    guint      index;
} HistoryLoadCtx;

static gboolean load_history_batch_cb(gpointer user_data) {
    HistoryLoadCtx *ctx = user_data;
    guint end = MIN(ctx->index + 100, ctx->messages->len);
    for (guint i = ctx->index; i < end; i++) {
        /* GtkTextBuffer에 메시지 삽입 */
    }
    ctx->index = end;
    if (ctx->index >= ctx->messages->len) {
        g_ptr_array_free(ctx->messages, TRUE);
        g_free(ctx);
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;  /* 다음 idle 사이클에 계속 */
}
```

---

## 3. GtkListBox 메모리 관리

| 목록 종류 | 최대 행 수 | 권장 위젯 |
|-----------|-----------|-----------|
| 친구 목록 | ≤ 200 | `GtkListBox` |
| 채팅방 목록 | ≤ 200 | `GtkListBox` |
| 200 초과 | — | `GtkListView` + `GtkSignalListItemFactory` |

`GtkListView` 사용 시 각 데이터 항목은 `GObject` 서브클래스로 래핑:

```c
/* FriendObject: GObject 서브클래스 */
G_DECLARE_FINAL_TYPE(FriendObject, friend_object, APP, FRIEND_OBJECT, GObject)

struct _FriendObject {
    GObject parent;
    char    user_id[21];
    char    nickname[21];
    int     status;  /* 0=offline, 1=online, 2=busy */
};

/* GtkSignalListItemFactory 바인딩 */
static void bind_cb(GtkListItemFactory *f, GtkListItem *item, gpointer data) {
    FriendObject *obj = gtk_list_item_get_item(item);
    GtkLabel     *lbl = GTK_LABEL(gtk_list_item_get_child(item));
    gtk_label_set_text(lbl, obj->nickname);
}
```

---

## 4. CSS 적용

앱 시작 시 한 번만 CSS를 로드.

```c
GtkCssProvider *provider = gtk_css_provider_new();
gtk_css_provider_load_from_resource(provider, "/com/example/chat/style.css");
gtk_style_context_add_provider_for_display(
    gdk_display_get_default(),
    GTK_STYLE_PROVIDER(provider),
    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
);
g_object_unref(provider);
```

CSS 파일은 `resources/style.css`로 관리하며 `GResource`로 번들. 런타임에 CSS를 재로드하는 것은 성능 비용이 크므로 금지.

---

## 5. 창 크기 변경 처리

최소 창 크기 설정:

```c
gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
/* 최솟값 강제: 600×400px */
gtk_widget_set_size_request(GTK_WIDGET(window), 600, 400);
```

창 크기 변경 시 레이아웃 재계산이 필요한 경우 `notify::default-width` / `notify::default-height` 시그널 사용:

```c
g_signal_connect(window, "notify::default-width",
    G_CALLBACK(on_window_size_changed), NULL);
g_signal_connect(window, "notify::default-height",
    G_CALLBACK(on_window_size_changed), NULL);
```

> GTK4에서는 `GtkPaned`, `GtkScrolledWindow` 등이 크기 변경을 자동으로 처리하므로 수동 재계산은 최소화.

---

## 6. 목표 성능 지표

| 지표 | 목표값 | 측정 방법 |
|------|--------|-----------|
| CPU 사용률 (idle) | < 5% | `top` / 작업 관리자 |
| 메모리 상주 크기 | < 50MB | `/proc/<pid>/status` VmRSS |
| 메시지 추가 지연 | < 16ms (60fps 기준) | GTK Inspector → Statistics |
| 앱 시작 시간 | < 2초 | 실행 후 로그인 화면 표시까지 |

> **측정 도구**: `GTK_DEBUG=interactive ./chat_client` 실행 시 GTK Inspector 활성화.  
> Inspector → Statistics 탭에서 프레임 타임 확인 가능.
