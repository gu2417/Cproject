# 설정

Google Workspace 설정 페이지의 **행 단위 카드** 패턴.
좌측에 라벨 + 설명, 우측에 GTK4 컨트롤. 섹션은 `GtkSeparator`로 구분.

## 레이아웃 (GTK4 위젯 트리)

```
GtkScrolledWindow
└── GtkBox (vertical, spacing=12, margin=24)
    ├── GtkLabel "외관" (섹션 헤더, font-weight=bold)
    ├── GtkBox (horizontal): GtkLabel "테마"
    │   └── GtkDropDown ["다크", "라이트"]  ← GtkStringList
    ├── GtkBox (horizontal): GtkLabel "메시지 색상"
    │   └── GtkColorDialogButton
    ├── GtkBox (horizontal): GtkLabel "닉네임 색상"
    │   └── GtkColorDialogButton
    ├── GtkBox (horizontal): GtkLabel "타임스탬프 형식"
    │   └── GtkDropDown ["HH:MM", "오전/오후 HH:MM", "YYYY-MM-DD HH:MM"]
    ├── GtkSeparator (horizontal)
    ├── GtkLabel "알림" (섹션 헤더, font-weight=bold)
    └── GtkBox (horizontal): GtkLabel "방해금지 모드"
        └── GtkSwitch
```

## 컨트롤 컴포넌트

### GtkDropDown (테마 / 타임스탬프 형식)

```c
GtkStringList *list = gtk_string_list_new((const char *[]){"다크", "라이트", NULL});
GtkWidget *dropdown = gtk_drop_down_new(G_LIST_MODEL(list), NULL);
```

선택 변경 시 `notify::selected` 시그널 → 즉시 `SETTINGS_UPDATE` 전송.

### GtkColorDialogButton (메시지/닉네임 색상)

```c
GtkColorDialog *dialog = gtk_color_dialog_new();
GtkWidget *btn = gtk_color_dialog_button_new(dialog);
```

색상 변경 시 `notify::rgba` 시그널 → 즉시 `SETTINGS_UPDATE` 전송.

### GtkSwitch (방해금지 모드)

```c
GtkWidget *sw = gtk_switch_new();
g_signal_connect(sw, "notify::active", G_CALLBACK(on_dnd_changed), NULL);
```

토글 시 즉시 `SETTINGS_UPDATE` 전송.

## 실시간 적용

변경이 발생하면 즉시 `SETTINGS_UPDATE|<msg_color>|<nick_color>|<theme>|<ts_format>|<dnd>` 전송.
별도 저장 버튼 없음. 응답 OK 시 `g_state.settings` 갱신 + 현재 화면 즉시 재렌더.

## GTK4 가속키

| 키 | 동작 |
|----|------|
| `Ctrl+,` | 설정 화면 열기 |
| `Escape` | 변경이 있으면 확인 `GtkAlertDialog`, 없으면 바로 MAIN 복귀 |

## 데이터 소스

- 진입 시 `SETTINGS_GET` → 각 컨트롤 초기값 설정.
- `GtkDropDown`: `gtk_drop_down_set_selected()` 로 초기 선택 반영.
- `GtkColorDialogButton`: `gtk_color_dialog_button_set_rgba()` 로 초기 색상 반영.
- `GtkSwitch`: `gtk_switch_set_active()` 로 초기 상태 반영.
