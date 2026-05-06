# FR-C: 유저 커스터마이징

콘솔 환경에서의 커스터마이징은 Windows 콘솔 ANSI escape code 또는 `SetConsoleTextAttribute` API를 사용한다. 설정값은 `user_settings.txt`에 영속 저장되며, 로그인 시 서버에서 `SETTINGS_RES`로 전달받아 클라이언트 상태에 적용한다.

---

## FR-C01 — 내 메시지 색상

내가 보낸 메시지의 텍스트 색상을 변경한다.

### 지원 색상

| 값 | ANSI 코드 | 표시 색상 |
|----|-----------|-----------|
| `cyan` | `\033[36m` | 청록색 (기본) |
| `white` | `\033[37m` | 흰색 |
| `green` | `\033[32m` | 녹색 |
| `yellow` | `\033[33m` | 노란색 |
| `magenta` | `\033[35m` | 자홍색 |
| `red` | `\033[31m` | 빨간색 |

### 적용

```c
void print_my_message(const char *timestamp, const char *content) {
    const char *color = get_ansi_color(g_state.msg_color);
    printf("[%s] %s나%s: %s\n",
           timestamp, color, "\033[0m", content);
}
```

---

## FR-C02 — 닉네임 색상

채팅창에 표시되는 내 닉네임 색상을 변경한다.

```c
void print_chat_message(const char *from_nick, const char *timestamp,
                        const char *content) {
    const char *nick_color = "\033[0m";

    /* 내 메시지이면 설정 색상 적용 */
    if (strcmp(from_nick, g_state.nickname) == 0)
        nick_color = get_ansi_color(g_state.nick_color);

    printf("[%s] %s%s\033[0m: %s\n",
           timestamp, nick_color, from_nick, content);
}
```

---

## FR-C03 — 테마

`dark`(기본) / `light` 테마 선택. 배경색 변경은 Windows Console API를 사용한다.

```c
void apply_theme(const char *theme) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (strcmp(theme, "light") == 0) {
        /* 흰 배경, 검정 글자 */
        SetConsoleTextAttribute(hConsole,
            BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
    } else {
        /* dark: 검정 배경, 흰 글자 (기본) */
        SetConsoleTextAttribute(hConsole,
            FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }
}
```

---

## FR-C04 — 타임스탬프 형식

| ts_format | 형식 | 예시 |
|-----------|------|------|
| 0 | `HH:MM` | `14:30` (기본) |
| 1 | `HH:MM:SS` | `14:30:25` |
| 2 | `MM-DD HH:MM` | `05-07 14:30` |

설정 화면에서 숫자 선택으로 변경:
```
  4. 타임스탬프 형식 : [HH:MM    ]
     0=HH:MM  1=HH:MM:SS  2=MM-DD HH:MM
> 선택 (0-2):
```

---

## FR-C05 — 상태메시지

친구 목록에 표시되는 한 줄 상태메시지를 수정한다.

```
C→S  PROFILE_UPDATE|<nickname>:<status_msg>
S→C  PROFILE_UPDATE_RES|0
```

마이페이지 또는 설정에서 변경 가능. 최대 100자, `\n` `;` `|` 금지.

---

## FR-C06 — 온라인 상태

수동으로 온라인 상태를 설정한다.

```
C→S  STATUS_CHANGE|<status>
```

| status 값 | 의미 | 친구에게 표시 |
|-----------|------|--------------|
| `online` | 온라인 | `[ON ]` |
| `busy` | 바쁨 | `[바쁨]` |
| `invisible` | 보이지 않음 | `[OFF]` (실제로는 온라인) |

설정 화면에서 변경:
```
  6. 온라인 상태 : [online]
     1=online  2=busy  3=invisible
> 선택 (1-3):
```

---

## FR-C07 — 오픈채팅 닉네임

오픈채팅방별 별도 닉네임을 설정한다.

```
C→S  ROOM_SET_OPEN_NICK|<room_id>:<nick>
S→C  ROOM_SET_OPEN_NICK_RES|0   (성공)
                             1   (실패)
```

제약: 최대 20자, `:` `;` `|` `\n` 금지.

`room_members.txt`의 `open_nick` 필드에 저장. 방 내 메시지 표시 시 `open_nick` 우선 적용.

---

## 설정 패킷

### 로그인 후 설정 조회

```
C→S  SETTINGS_REQ|
S→C  SETTINGS_RES|<msg_color>:<nick_color>:<theme>:<ts_format>:<dnd>
```

```c
/* 로그인 성공 후 즉시 요청 */
send_packet(g_state.sock, "SETTINGS_REQ|");

/* packet_parse()에서 처리 */
else if (strcmp(type, "SETTINGS_RES") == 0) {
    strncpy(g_state.msg_color,  strtok(payload, ":"), 15);
    strncpy(g_state.nick_color, strtok(NULL, ":"),    15);
    strncpy(g_state.theme,      strtok(NULL, ":"),    10);
    g_state.ts_format = atoi(strtok(NULL, ":"));
    g_state.dnd       = atoi(strtok(NULL, ":"));
    apply_theme(g_state.theme);
}
```

### 설정 변경 저장

```
C→S  SETTINGS_UPDATE|<msg_color>:<nick_color>:<theme>:<ts_format>:<dnd>
S→C  SETTINGS_UPDATE_RES|0
```

서버는 `user_settings.txt`에서 해당 유저 레코드를 upsert한다.

---

## 설정 기본값

| 항목 | 기본값 |
|------|--------|
| msg_color | `cyan` |
| nick_color | `yellow` |
| theme | `dark` |
| ts_format | `0` (HH:MM) |
| dnd | `0` (off) |
