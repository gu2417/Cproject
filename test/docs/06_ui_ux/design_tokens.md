# 디자인 토큰

GTK4 기반 C Chat의 시각적 일관성을 위한 CSS 변수 및 설정값.
GTK4 CSS Provider를 통해 런타임에 로드한다.

---

## 1. 색상 시스템 (GTK4 CSS 변수)

### 1.1 다크 테마 기본 팔레트

```css
/* 다크 테마 — C Chat 기본 */
@define-color bg_base       #1e1e2e;   /* 기본 배경 (윈도우/앱) */
@define-color bg_surface    #2a2a3e;   /* 카드·사이드바·패널 배경 */
@define-color bg_elevated   #313244;   /* 선택/호버 배경 */
@define-color fg_primary    #cdd6f4;   /* 본문 텍스트 */
@define-color fg_secondary  #a6adc8;   /* 보조 텍스트 */
@define-color fg_tertiary   #6c7086;   /* 메타/타임스탬프 */
@define-color fg_on_accent  #1e1e2e;   /* 강조 배경 위 텍스트 */
@define-color accent_primary #89b4fa;  /* 액센트 (링크·포커스·버튼) */
@define-color divider_color #45475a;   /* 구분선 */
```

### 1.2 상태 색상

```css
@define-color status_online  #a6e3a1;  /* 온라인 */
@define-color status_busy    #fab387;  /* 바쁨 */
@define-color status_dnd     #f38ba8;  /* 방해금지 */
@define-color status_offline #6c7086;  /* 오프라인 */
```

### 1.3 피드백 색상

```css
@define-color feedback_success #a6e3a1;  /* 성공 */
@define-color feedback_warn    #f9e2af;  /* 경고 */
@define-color feedback_error   #f38ba8;  /* 오류 */
```

### 1.4 라이트 테마 팔레트

```css
/* 라이트 테마 — GtkSettings 토글 시 적용 */
@define-color bg_base       #eff1f5;
@define-color bg_surface    #e6e9ef;
@define-color bg_elevated   #dce0e8;
@define-color fg_primary    #4c4f69;
@define-color fg_secondary  #5c5f77;
@define-color fg_tertiary   #9ca0b0;
@define-color fg_on_accent  #eff1f5;
@define-color accent_primary #1e66f5;
@define-color divider_color #ccd0da;
/* 상태·피드백 색은 다크와 동일하게 재사용 (대비 충분) */
```

### 1.5 다크/라이트 테마 전환

```c
/* GtkSettings로 다크 테마 활성화 */
GtkSettings *settings = gtk_settings_get_default();
g_object_set(settings,
    "gtk-application-prefer-dark-theme", TRUE,
    NULL);

/* 또는 런타임 토글 */
gboolean dark_mode = TRUE;
g_object_set(settings,
    "gtk-application-prefer-dark-theme", dark_mode,
    NULL);
/* 이후 CSS Provider로 해당 팔레트 파일을 reload */
```

CSS Provider 로드 예시:
```c
GtkCssProvider *provider = gtk_css_provider_new();
gtk_css_provider_load_from_path(provider,
    dark_mode ? "theme-dark.css" : "theme-light.css");
gtk_style_context_add_provider_for_display(
    gdk_display_get_default(),
    GTK_STYLE_PROVIDER(provider),
    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
```

---

## 2. 간격 (Spacing)

GTK4에서 픽셀 단위로 margin/padding 적용.

| 토큰 이름 | 픽셀 값 | 용도 |
|-----------|--------|------|
| `space_0` | 0px | 붙임 |
| `space_1` | 4px | 인접 요소 |
| `space_2` | 8px | 컴포넌트 내부 패딩 |
| `space_3` | 12px | 섹션 간 여백 |
| `space_4` | 16px | 컨테이너 내부 마진 |
| `space_section` | 24px | 주요 섹션 간 구분 |

CSS 적용 예시:
```css
.message-entry {
  margin: 8px;        /* space_2 */
  padding: 8px 12px;  /* space_2 + space_3 */
}
.chat-list row {
  padding: 8px;       /* space_2 */
  margin-bottom: 4px; /* space_1 */
}
```

**원칙**: `GtkSeparator`보다 **여백**을 먼저 쓴다. 구분선은 구조상 꼭 필요할 때만.

---

## 3. 타이포그래피

GTK4에서는 Pango를 통해 폰트 크기·가중치 제어.

| 레벨 | CSS 스타일 | 픽셀 크기 | 용도 |
|------|-----------|----------|------|
| `title` | `font-weight: bold; font-size: 18px` | 18px | 윈도우/화면 제목 |
| `h2` | `font-weight: bold; font-size: 16px` | 16px | 섹션 헤더 |
| `h3` | `font-weight: bold; font-size: 14px` | 14px | 항목 이름 |
| `body` | `font-size: 14px` | 14px | 본문 텍스트 |
| `caption` | `font-size: 12px; color: @fg_secondary` | 12px | 보조 설명 |
| `meta` | `font-size: 11px; color: @fg_tertiary` | 11px | 시간·카운트 |
| `me-action` | `font-style: italic; color: @fg_secondary` | 14px | `/me` 액션 |

CSS 클래스 정의:
```css
.title        { font-size: 18px; font-weight: bold; color: @fg_primary; }
.h2           { font-size: 16px; font-weight: bold; color: @fg_primary; }
.h3           { font-size: 14px; font-weight: bold; color: @fg_primary; }
.body         { font-size: 14px; color: @fg_primary; }
.caption      { font-size: 12px; color: @fg_secondary; }
.meta-label   { font-size: 11px; color: @fg_tertiary; }
.dim-label    { color: @fg_secondary; }
.me-action    { font-style: italic; color: @fg_secondary; }
```

기본 시스템 폰트 사용 (`gtk-font-name` 설정 따름). 별도 폰트 임베딩 없음.

---

## 4. 위젯 크기

| 요소 | 크기 | 설명 |
|------|------|------|
| 사이드바 폭 | 220px | `GtkPaned` 초기 분할 위치 |
| 최소 윈도우 | 800 × 600px | `gtk_window_set_min_size()` |
| 기본 윈도우 | 1000 × 700px | `gtk_window_set_default_size()` |
| 로그인 윈도우 | 400 × 520px | `gtk_window_set_default_size()` |
| 아바타 (기본) | 36 × 36px | `min-width: 36px; min-height: 36px` |
| 아바타 (소형) | 24 × 24px | `.avatar-label.small` |
| 아바타 (대형) | 56 × 56px | `.avatar-label.large` |
| 멤버 팝오버 폭 | 260px | `gtk_widget_set_size_request(popover, 260, -1)` |
| 입력창 최소 높이 | 40px | CSS `min-height: 40px` |
| HeaderBar 높이 | 48px | Adwaita 기본값 |
| 미읽음 뱃지 | 20 × 20px (최소) | `min-width: 20px; border-radius: 10px` |

---

## 5. 아이콘

GTK4 표준 아이콘 이름 사용 (시스템 테마에서 자동 해석).

| 의미 | 아이콘 이름 | 비고 |
|------|------------|------|
| 뒤로 가기 | `go-previous-symbolic` | HeaderBar 좌측 |
| 설정 | `preferences-system-symbolic` | |
| 검색 | `system-search-symbolic` | |
| 멤버 목록 | `system-users-symbolic` | |
| 더보기 메뉴 | `view-more-symbolic` | |
| 친구 추가 | `list-add-symbolic` | |
| 새 채팅 | `chat-symbolic` | |
| 핀/공지 | `pin-symbolic` | |
| 비밀번호 방 | `changes-prevent-symbolic` | |
| 메시지 전송 | `mail-send-symbolic` | |
| 귓속말 | `mail-symbolic` | |
| 닫기 | `window-close-symbolic` | |
| 오류 | `dialog-error-symbolic` | |

모든 아이콘은 CSS로 크기 지정 (`font-size: 16px` 또는 `gtk_image_set_icon_size()` 사용, GTK4에서 icon size enum 제거됨).

---

## 6. 아바타 색상 — 닉네임 해시

이니셜 아바타 배경색은 닉네임/아이디 해시로 결정. 라이트/다크 공통 팔레트:

```c
/* GTK4용 아바타 색상 팔레트 (ARGB) */
static const GdkRGBA AVATAR_COLORS[] = {
    { 0.537, 0.706, 0.980, 1.0 },  /* #89b4fa — 파랑 */
    { 0.651, 0.847, 0.631, 1.0 },  /* #a6e3a1 — 초록 */
    { 0.980, 0.702, 0.529, 1.0 },  /* #fab387 — 주황 */
    { 0.956, 0.545, 0.659, 1.0 },  /* #f48daa — 분홍 */
    { 0.816, 0.773, 0.976, 1.0 },  /* #d0bef4 — 보라 */
    { 0.596, 0.906, 0.988, 1.0 },  /* #98d8fc — 하늘 */
    { 0.980, 0.902, 0.435, 1.0 },  /* #f9e56f — 노랑 */
    { 0.671, 0.902, 0.839, 1.0 },  /* #abe6d6 — 민트 */
};
#define AVATAR_COLOR_COUNT 8

uint32_t fnv1a(const char *str) {
    uint32_t h = 2166136261u;
    while (*str) { h ^= (uint8_t)*str++; h *= 16777619u; }
    return h;
}

int avatar_color_index(const char *user_id) {
    return fnv1a(user_id) % AVATAR_COLOR_COUNT;
}
```

전경색은 항상 `@bg_base` (진한 배경)으로 충분한 대비 확보.

---

## 7. 포커스 · 선택 표현

| 상태 | GTK4 표현 |
|------|-----------|
| 기본 | 장식 없음 |
| hover (마우스 오버) | Adwaita 자동 처리 (`GtkListBoxRow:hover`) |
| 포커스 (키보드) | Adwaita focus ring 자동 표시 |
| 선택됨 | `GtkListBoxRow:selected` — 배경색 `@accent_primary` (dim) |
| 비활성 | `gtk_widget_set_sensitive(w, FALSE)` → dim 처리 자동 |

CSS 추가 스타일:
```css
row:hover {
  background-color: @bg_elevated;
}
row:selected {
  background-color: alpha(@accent_primary, 0.2);
}
row:selected label {
  color: @fg_primary;
}
```

---

## 8. 텍스트 말줄임 (Truncation)

| 요소 | 최대 문자 수 | Pango 설정 |
|------|------------|-----------|
| 닉네임 | 20자 | `PANGO_ELLIPSIZE_END` |
| 방 이름 (HeaderBar) | 동적 (윈도우 폭 기반) | `PANGO_ELLIPSIZE_MIDDLE` |
| 메시지 미리보기 | 40자 | `PANGO_ELLIPSIZE_END` |
| 방 주제 (RoomCard) | 50자 | `PANGO_ELLIPSIZE_END` |
| 검색 결과 미리보기 | 60자 | `PANGO_ELLIPSIZE_END` |
| 답장 인용 미리보기 | 40자 | `PANGO_ELLIPSIZE_END` |
| 공지 요약 (InfoBar) | 60자 | `PANGO_ELLIPSIZE_END` |

```c
/* 적용 예시 */
gtk_label_set_max_width_chars(GTK_LABEL(label), 40);
gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
```

---

## 9. 애니메이션 타이밍

| 토큰 | 값 | 용도 |
|------|----|------|
| `anim_message_appear` | 150ms ease-out | 새 메시지 페이드 인 |
| `anim_panel_slide` | 200ms ease-in-out | GtkStack 페이지 전환 슬라이드 |
| `anim_popover_appear` | 100ms | GtkPopover 나타남 |
| `anim_revealer_slide` | 200ms | GtkRevealer 슬라이드 인/아웃 |
| `debounce_search` | 300ms | 검색어 변경 후 서버 요청 지연 |
| `debounce_read` | 100ms | 읽음 카운터 업데이트 지연 |
| `banner_auto_dismiss` | 5,000ms | 일반 알림 Revealer 자동 숨김 |
| `banner_friend_req` | 10,000ms | 친구 요청 Revealer 자동 숨김 |
| `toast_duration` | 2,000ms | Toast 자동 숨김 |
| `register_redirect` | 1,200ms | 가입 완료 후 로그인 화면 전환 |
| `typing_expire` | 5,000ms | TYPING_STOP 미수신 시 타이핑 표시 소멸 |
| `ping_interval` | 30,000ms | PING 패킷 전송 간격 |
| `ping_timeout` | 60,000ms | PONG 미수신 → 연결 끊김 판정 |
| `reconnect_interval` | 5,000ms | 재연결 시도 간격 |

타이밍 구현:
```c
/* GLib 타이머로 구현 */
guint timer_id = g_timeout_add(5000, on_banner_dismiss, infobar);
/* 취소 시: g_source_remove(timer_id); */
```

GTK4 CSS 애니메이션:
```css
/* GtkStack 전환 — C 코드로 설정 */
/* gtk_stack_set_transition_type(stack, GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT); */
/* gtk_stack_set_transition_duration(stack, 200); */

/* 메시지 나타남 — CSS 트랜지션 */
.message-list row {
  transition: opacity 150ms ease-out;
}
.message-list row.new-message {
  opacity: 0;
}
```

---

## 10. Z-레이어 모델 (GTK4 stacking)

GTK4에서는 `GtkOverlay`와 `GtkPopover`의 자연스러운 스태킹 활용.

| 레이어 | 위젯 | 방법 |
|--------|------|------|
| 최상위 | 연결 끊김 GtkRevealer | `GtkOverlay` 최상위 자식, `valign=START` |
| 다이얼로그 | GtkAlertDialog, GtkPopover, GtkWindow (모달) | GTK 자체 모달 처리 |
| 오버레이 | 검색 GtkSearchBar, 멤버 GtkPopover | `GtkOverlay` 자식 또는 `GtkPopover` |
| 알림 | GtkRevealer 스택 | 메인 GtkBox 상단 |
| 기본 | 메인 콘텐츠 | GtkStack |

**규칙**: GtkDialog가 활성이면 부모 윈도우 입력이 자동 차단. Escape는 항상 현재 최상위 위젯(팝오버/다이얼로그) 닫기.

---

## 11. 전체 CSS 구조

프로젝트 CSS 파일 구성:

```
chat_program/src/client/
  css/
    theme-dark.css      — 다크 테마 색상 변수 (@define-color)
    theme-light.css     — 라이트 테마 색상 변수
    components.css      — 공통 컴포넌트 스타일
    chat.css            — 채팅 화면 전용 스타일
    login.css           — 로그인 화면 전용 스타일
```

런타임 로드 순서:
1. `theme-dark.css` (또는 `theme-light.css`) — 변수 정의
2. `components.css` — 변수 참조
3. 화면별 CSS — 화면 진입 시 추가 로드

---

## 12. 금기

- 인라인 스타일 (`gtk_widget_override_*` 남용) — CSS Provider로 일원화
- 하드코딩된 색상 값 — 반드시 `@define-color` 변수 참조
- 픽셀 퍼펙트 수동 위치 지정 (`gtk_fixed`) — GTK 레이아웃 위젯 사용
- 메인 스레드 외 GTK 위젯 직접 조작 — `g_idle_add()` 또는 `g_main_context_invoke()` 사용
- 1초보다 짧은 전체 화면 재렌더 루프 — 변경된 위젯만 업데이트
