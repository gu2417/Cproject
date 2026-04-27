# 클라이언트 모듈 (GTK4)

GTK4 기반 클라이언트의 파일 구조와 각 모듈의 책임을 정리한다.

---

## 디렉터리 구조

```
chat_program/src/client/
  main.c              -- GtkApplication 초기화, g_application_run()
  app_window.c/h      -- GtkApplicationWindow 생성, GtkStack 설정
  net.c/h             -- TCP 연결, send/recv 스레드, g_idle_add UI 연결
  packet.c/h          -- 패킷 직렬화/파싱 헬퍼
  screen_login.c/h    -- 로그인/회원가입 화면 위젯
  screen_main.c/h     -- 메인화면 (친구/채팅방/오픈채팅/마이페이지 탭)
  screen_chat.c/h     -- 채팅화면 (메시지 뷰, 입력, 참여자 패널)
  screen_mypage.c/h   -- 마이페이지 화면 위젯
  screen_settings.c/h -- 설정화면 위젯
  friend_view.c/h     -- 친구 목록 GtkListBox 위젯
  room_view.c/h       -- 채팅방 목록 GtkListBox 위젯
  notify.c/h          -- 앱 내 알림: GtkRevealer 배너, 배지
  resources/          -- CSS, 아이콘 (GResource 번들)
    style.css
    icons/
```

---

## 모듈 상세

### `main.c`

- **책임**: `GtkApplication` 생성, `activate` 시그널에 `app_window_new()` 연결, `g_application_run()` 호출.
- **주요 함수**:
  - `int main(int argc, char **argv)`
  - `static void on_activate(GtkApplication *app, gpointer data)`

---

### `app_window.c/h`

- **책임**: `GtkApplicationWindow` 생성, 최상위 `GtkStack` 설정 (LOGIN / MAIN / CHAT / MYPAGE / SETTINGS), 화면 전환 API 제공.
- **주요 함수**:
  - `GtkWidget *app_window_new(GtkApplication *app)`
  - `void app_window_show_screen(const char *screen_name)`
  - `GtkStack *app_window_get_stack(void)`
- **의존**: `screen_login`, `screen_main`, `screen_chat`, `screen_mypage`, `screen_settings`

---

### `net.c/h`

- **책임**: TCP 소켓 연결/해제, 송신 스레드(`tx_thread`), 수신 스레드(`rx_thread`), `tx_mutex` 보호 하에 스레드 세이프 전송.
- **주요 함수**:
  - `int  net_connect(const char *host, uint16_t port)`
  - `void net_disconnect(void)`
  - `int  net_send(const char *packet)`  ← `tx_mutex` 잠금
  - `void net_set_recv_handler(void (*handler)(const char *packet))`
- **의존**: `packet`, `notify` (연결 끊김 알림)

> recv 스레드에서 UI 접근 시 반드시 `g_idle_add()` 사용.

---

### `packet.c/h`

- **책임**: `<TYPE>|<PAYLOAD>\n` 형식 패킷의 직렬화/파싱. 최대 2048바이트.
- **주요 함수**:
  - `char *packet_build(const char *type, const char *payload)`
  - `int   packet_parse(const char *raw, char *type_out, char *payload_out)`
  - `char *packet_field(const char *payload, int index)`  ← `|` 구분자 기준

---

### `screen_login.c/h`

- **책임**: 로그인/회원가입 `GtkWidget*` 구성. `GtkEntry` (아이디, 비밀번호), `GtkButton` (로그인, 회원가입).
- **주요 함수**:
  - `GtkWidget *screen_login_new(void)`
  - `void screen_login_reset(void)`
  - `static void on_login_clicked(GtkButton *btn, gpointer data)`
- **의존**: `net`, `packet`

---

### `screen_main.c/h`

- **책임**: 메인화면 `GtkWidget*`. 내부 `GtkStack`으로 FRIENDS / ROOMS / OPEN / MYPAGE 탭 전환. 헤더바 탭 버튼 관리.
- **주요 함수**:
  - `GtkWidget *screen_main_new(void)`
  - `void screen_main_refresh_friends(void)`
  - `void screen_main_refresh_rooms(void)`
  - `void screen_main_switch_tab(const char *tab_name)`
- **의존**: `friend_view`, `room_view`, `screen_mypage`, `net`

---

### `screen_chat.c/h`

- **책임**: 채팅화면 `GtkWidget*`. `GtkTextView` 메시지 뷰, `GtkEntry` 입력창, `GtkListBox` 참여자 패널. 답장/수정 상태 표시 (`GtkRevealer`).
- **주요 함수**:
  - `GtkWidget *screen_chat_new(void)`
  - `void screen_chat_enter_room(int room_id, const char *room_name)`
  - `void screen_chat_append_message(const char *sender, const char *content)`
  - `void screen_chat_set_reply_context(int msg_id, const char *preview)`
  - `void screen_chat_clear(void)`
- **의존**: `net`, `packet`, `notify`

---

### `screen_mypage.c/h`

- **책임**: 마이페이지 `GtkWidget*`. `GtkGrid` 레이아웃, `GtkLabel` (닉네임, 상태 메시지), `GtkButton` (프로필 편집, 로그아웃).
- **주요 함수**:
  - `GtkWidget *screen_mypage_new(void)`
  - `void screen_mypage_refresh(void)`
  - `static void on_logout_clicked(GtkButton *btn, gpointer data)`

---

### `screen_settings.c/h`

- **책임**: 설정화면 `GtkWidget*`. `GtkDropDown` + `GtkStringList` (언어/테마), `GtkSwitch` (DND, 알림), `GtkColorDialogButton` + `GtkColorDialog` (테마 색상).
- **주요 함수**:
  - `GtkWidget *screen_settings_new(void)`
  - `void screen_settings_load(void)`
  - `void screen_settings_save(void)`

---

### `friend_view.c/h`

- **책임**: 친구 목록 `GtkListBox` 위젯 생성/업데이트/삭제. 행 당 상태 아이콘 + 닉네임 표시. 우클릭 시 `GtkPopoverMenu`.
- **주요 함수**:
  - `GtkWidget *friend_view_new(void)`
  - `void friend_view_set_list(const FriendEntry *entries, int count)`
  - `void friend_view_update_status(const char *user_id, int status)`
- **의존**: `net`, `packet`

---

### `room_view.c/h`

- **책임**: 채팅방 목록 `GtkListBox` 위젯. 읽지 않은 메시지 배지 (`GtkLabel`) 포함.
- **주요 함수**:
  - `GtkWidget *room_view_new(void)`
  - `void room_view_set_list(const ChatRoom *rooms, int count)`
  - `void room_view_update_badge(int room_id, int unread_count)`
- **의존**: `net`, `packet`

---

### `notify.c/h`

- **책임**: 앱 내 알림 표시. `GtkRevealer` 배너 (슬라이드-인), 탭 배지 숫자 (`GtkLabel`). 자동 해제 타이머 (`g_timeout_add`).
- **주요 함수**:
  - `GtkWidget *notify_banner_new(void)`        ← `GtkRevealer` 래퍼
  - `void notify_show(const char *message, int timeout_ms)`
  - `void notify_set_badge(const char *tab_name, int count)`

---

## 의존성 그래프

```
main
 └─ app_window
     ├─ screen_login   ──→ net, packet
     ├─ screen_main    ──→ friend_view, room_view, net
     ├─ screen_chat    ──→ net, packet, notify
     ├─ screen_mypage
     └─ screen_settings

net ──→ packet, notify
friend_view ──→ net, packet
room_view   ──→ net, packet
notify      (독립)
```

---

## 빌드

```bash
pkg-config --cflags --libs gtk4
gcc $(pkg-config --cflags gtk4) chat_program/src/client/*.c -o chat_program/src/client/chat_client $(pkg-config --libs gtk4)
```

또는 프로젝트 루트에서:

```bash
make client
```
