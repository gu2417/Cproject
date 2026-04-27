# 로그인 · 회원가입

Google Workspace 로그인 페이지의 **중앙 카드** 패턴을 GTK4 로 구현.
단 하나의 초점(로그인 폼) + 앱 브랜드 타이틀 + 보조 버튼.

---

## 1. 로그인 윈도우 (GtkWindow)

### 윈도우 속성

| 속성 | 값 |
|------|-----|
| 위젯 타입 | `GtkWindow` |
| 타이틀 | `"C Chat v2.0"` |
| 기본 크기 | 400 × 520 px |
| `resizable` | FALSE |
| CSS class | `login-window` |

### 위젯 트리 (GtkBox 수직 레이아웃)

루트 컨테이너는 `GtkBox` (orientation=VERTICAL, spacing=12, margin=32).

```
GtkWindow "C Chat v2.0"
  GtkBox (vertical, spacing=12, margin=32)
    GtkLabel  — 앱 타이틀
    GtkLabel  — 서브타이틀
    GtkSeparator (horizontal)
    GtkEntry  — 아이디 입력
    GtkEntry  — 비밀번호 입력 (visibility=FALSE)
    GtkButton "로그인"       (css: suggested-action)
    GtkButton "회원가입"     (secondary)
    GtkLabel  — 오류 메시지  (initially hidden)
```

### 각 위젯 상세

**앱 타이틀 GtkLabel**
- `gtk_label_set_markup(label, "<b><big>C Chat</big></b>")`
- `halign = GTK_ALIGN_CENTER`
- margin-top = 16px
- CSS class: `app-title`

**서브타이틀 GtkLabel**
- 텍스트: `"가볍고 빠른 채팅 메신저"`
- CSS class: `dim-label` (Adwaita 제공)
- `halign = GTK_ALIGN_CENTER`

**아이디 GtkEntry**
- `placeholder_text = "아이디 입력"`
- `max_length = 20`
- `input_purpose = GTK_INPUT_PURPOSE_FREE_FORM`
- `activates_default = TRUE`

**비밀번호 GtkEntry**
- `placeholder_text = "비밀번호 입력"`
- `visibility = FALSE` (마스킹)
- `input_purpose = GTK_INPUT_PURPOSE_PASSWORD`
- `activates_default = TRUE`

**로그인 GtkButton**
- 라벨: `"로그인"`
- CSS class: `suggested-action` (파란색 강조)
- `can_default = TRUE` — Enter 키 트리거
- 클릭 시 → `on_login_clicked()` 콜백

**회원가입 GtkButton**
- 라벨: `"회원가입"`
- 스타일: 기본 (보조 액션)
- 클릭 시 → `on_register_clicked()` — 회원가입 다이얼로그 표시

**오류 메시지 GtkLabel**
- 초기 상태: `gtk_widget_hide(error_label)`
- CSS class: `error` (Adwaita: 빨간 텍스트)
- `halign = GTK_ALIGN_CENTER`
- 로그인 실패 시: `gtk_label_set_text()` + `gtk_widget_show()`
- 로그인 성공 시: `gtk_widget_hide()`

---

## 2. 회원가입 다이얼로그 (GtkWindow)

### 다이얼로그 속성

| 속성 | 값 |
|------|-----|
| 위젯 타입 | `GtkWindow` |
| 타이틀 | `"회원가입"` |
| `modal` | TRUE |
| `transient_for` | 로그인 GtkWindow |
| 기본 크기 | 420 × 600 px |

### 위젯 트리

```
GtkWindow "회원가입"
  GtkBox (vertical)
    GtkBox (vertical, spacing=8, margin=24)   — 콘텐츠 영역
      GtkLabel "아이디"  +  GtkEntry (id_entry)
      GtkLabel — 아이디 검증 힌트 (caption)
      GtkLabel "비밀번호"  +  GtkEntry (pw_entry, visibility=FALSE)
      GtkLabel "비밀번호 확인"  +  GtkEntry (pw2_entry, visibility=FALSE)
      GtkLabel "닉네임"  +  GtkEntry (nick_entry)
      GtkLabel — 닉네임 검증 힌트 (caption)
      GtkLabel "상태 메시지"  +  GtkEntry (status_entry)
      GtkLabel — 선택 항목 안내
    GtkBox (horizontal, action-area)          — 액션 버튼 영역
      GtkButton "취소"   (on_cancel_clicked)
      GtkButton "가입하기" (on_register_clicked, css: suggested-action)
```

### 각 필드 유효성 규칙

| 필드 | 규칙 | 힌트 텍스트 |
|------|------|-------------|
| 아이디 | 3~20자, 영숫자/언더스코어 | `"3~20자 · 영문/숫자/_"` |
| 비밀번호 | 4~30자 | `"4~30자"` |
| 비밀번호 확인 | 비밀번호와 일치 | `"비밀번호가 일치하지 않습니다"` |
| 닉네임 | 1~20자 | `"1~20자"` |
| 상태 메시지 | 0~100자, 선택 | `"선택 항목 · 최대 100자"` |

- 각 필드 아래 `GtkLabel` (caption)으로 규칙 힌트 표시.
- 유효성 위반 시 해당 `GtkLabel`에 오류 메시지 교체 + CSS class `error` 추가.
- `GtkEntry`에 `has-error` CSS class 추가로 테두리 빨간색.

---

## 3. 상태 전환

### 로그인 성공 흐름

```
사용자 입력 → [로그인] 버튼 클릭
  → on_login_clicked() 호출
  → LOGIN 패킷 서버 전송
  → 서버 응답 수신 (비동기)
    → 성공 (code=0): gtk_widget_hide(login_window)
                     gtk_widget_show(main_window)
    → 실패: gtk_label_set_text(error_label, "오류 메시지")
            gtk_widget_show(error_label)
```

### 로그인 실패 오류 메시지 매핑

서버 응답 코드(`08_api/error_codes.md`) 기반:

| code | GtkLabel 표시 문구 |
|-----:|--------------------|
| 0 | 성공 → MainWindow 전환 |
| 1 | `"입력을 확인해 주세요"` |
| 2 | `"아이디 또는 비밀번호가 맞지 않습니다"` |
| 3 | `"이미 존재하는 아이디입니다"` |
| 6 | `"이미 접속 중인 계정입니다"` |
| 9 | `"일시적인 서버 오류가 발생했습니다"` |

### 회원가입 성공 흐름

```
[가입하기] 버튼 클릭 → REGISTER 패킷 서버 전송
  → 성공 응답:
      GtkWindow 닫기 (gtk_window_close(register_window))
      GtkRevealer 또는 GtkLabel로 "가입이 완료되었습니다" 표시
      1.2초 후 (g_timeout_add) 자동으로 로그인 화면으로 복귀
  → 실패 응답:
      다이얼로그 내 오류 GtkLabel에 메시지 표시
```

---

## 4. CSS 스타일 (GTK4 CSS)

```css
/* 로그인 윈도우 전체 배경 */
.login-window {
  background-color: @bg_base;
}

/* 앱 타이틀 */
.app-title {
  color: @accent_primary;
  margin-bottom: 4px;
}

/* 오류 메시지 레이블 */
label.error {
  color: @error_color;
  font-size: 12px;
}

/* 로그인 버튼 강조 */
button.suggested-action {
  background-color: @accent_primary;
  color: @fg_primary;
  border-radius: 6px;
  padding: 8px 24px;
  margin-top: 8px;
}
```

---

## 5. 접근성 · UX 보조

- `gtk_widget_set_can_default(login_btn, TRUE)` + `gtk_window_set_default(window, login_btn)` — Enter 키로 로그인.
- 탭 순서: 아이디 → 비밀번호 → 로그인 → 회원가입 (`gtk_widget_set_focus_on_click`).
- 비밀번호 필드는 `visibility=FALSE`로 항상 마스킹; 별도 토글 버튼은 미구현(추후 P3).
- 클라이언트 사전 검증 통과 후에만 `LOGIN`/`REGISTER` 패킷 전송.
