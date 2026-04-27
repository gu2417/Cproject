# 유저 프로필

다른 사용자의 프로필 카드를 보여주는 팝오버 및 다이얼로그.
채팅 화면, 친구 목록, 멤버 목록 등 **모든 화면에서 사용자 이름/아바타 클릭 시** 표시.

---

## 1. 진입 경로

| 진입 위치 | 동작 |
|-----------|------|
| 채팅 말풍선 — 아바타/닉네임 클릭 | 프로필 팝오버 표시 |
| 친구 탭 — 항목 클릭 | 프로필 팝오버 표시 |
| 멤버 목록 — 항목 클릭 | 프로필 팝오버 표시 |
| 검색 결과 — 항목 클릭 | 프로필 팝오버 표시 |

자기 자신 클릭 → 마이페이지로 이동 (프로필 팝오버 아님).

---

## 2. 프로필 팝오버 (GtkPopover)

```
GtkPopover (css: user-profile-popover, width=280px)
└── GtkBox (vertical, spacing=12, margin=16)
    ├── GtkBox (horizontal, spacing=12)           — Hero 행
    │   ├── GtkLabel (이니셜, css: avatar-label.large)
    │   └── GtkBox (vertical, spacing=4)
    │       ├── GtkLabel (닉네임, css: h3)
    │       ├── GtkLabel (아이디, css: dim-label)
    │       └── GtkBox (horizontal, spacing=4)
    │           ├── GtkLabel (● css: status-dot)
    │           └── GtkLabel (온라인/오프라인, css: dim-label)
    ├── GtkLabel (상태 메시지, css: body)          — 상태 메시지 (있으면)
    ├── GtkSeparator (horizontal)
    └── GtkBox (horizontal, spacing=8, homogeneous=TRUE) — 액션 버튼들
        ├── GtkButton "DM 보내기"  (suggested-action)
        ├── GtkButton "친구 추가"  OR "친구 삭제"
        └── GtkButton "차단"
```

### 버튼 상태 조건

| 관계 | "DM" 버튼 | "친구" 버튼 | "차단" 버튼 |
|------|-----------|-------------|-------------|
| 친구 | 활성 | "친구 삭제" | "차단" |
| 친구 요청 보냄 | 활성 | "요청 취소" (비활성) | "차단" |
| 친구 요청 받음 | 활성 | "수락" | "거절/차단" |
| 낯선 사람 | 활성 | "친구 추가" | "차단" |
| 차단한 상대 | 숨김 | 숨김 | "차단 해제" |
| 자기 자신 | 숨김 | 숨김 | 숨김 |

### 온라인 상태 표시

```c
/* 상태 점 + 텍스트 */
static const char *status_text[] = {
    "오프라인", "온라인", "자리 비움", "방해금지"
};
static const char *status_css[] = {
    "status-offline", "status-online", "status-busy", "status-dnd"
};
```

차단된 상대 또는 DND 상태인 상대: `online_status = 0` (서버가 마스킹).

---

## 3. 데이터 요청

팝오버 열기 → 즉시 `USER_VIEW|<id>` 전송.  
응답 전까지: GtkSpinner (Hero 영역 중앙).  
응답 수신 후: 스피너 제거 + 위젯 채움.

```
USER_VIEW|alice
USER_VIEW_RES|0|alice|앨리스|오늘도 화이팅|1    # id|nick|status_msg|online_status
USER_VIEW_RES|4                                  # 유저 없음 → 팝오버에 "찾을 수 없습니다" 표시
```

---

## 4. 액션 처리

### DM 보내기

```
[DM 보내기] 클릭
  → 팝오버 닫기
  → 메인 화면 DM 채팅으로 전환 (이미 DM 있으면) 또는 새 DM 채팅 시작
  → screen_chat_enter_dm(peer_id, peer_nick)
```

### 친구 추가

```
[친구 추가] 클릭
  → FRIEND_ADD|<id> 전송
  → 버튼을 "요청 중..." + 비활성으로 교체
  → FRIEND_ADD_RES 수신:
      0 → 버튼 "요청 보냄" 텍스트 + 비활성
      3 → Toast "이미 친구이거나 요청 중입니다"
      7 → Toast "차단된 상태입니다"
```

### 친구 삭제

```
[친구 삭제] 클릭
  → GtkAlertDialog "정말 친구를 삭제하시겠어요?" [취소] [삭제]
  → [삭제] → FRIEND_REMOVE|<id> 전송
  → 성공 → 버튼을 "친구 추가"로 교체 + Toast "친구가 삭제되었습니다"
```

### 차단

```
[차단] 클릭
  → GtkAlertDialog "홍길동 님을 차단하시겠어요?\n차단하면 메시지와 친구 요청이 차단됩니다." [취소] [차단]
  → [차단] → FRIEND_BLOCK|<id> 전송
  → 성공 → 팝오버 닫기 + Toast "차단되었습니다"
```

---

## 5. 팝오버 트리거 (GtkGestureClick)

```c
/* 아바타 또는 닉네임 라벨에 클릭 이벤트 연결 */
GtkGestureClick *gesture = GTK_GESTURE_CLICK(gtk_gesture_click_new());
gtk_gesture_click_set_button(gesture, GDK_BUTTON_PRIMARY);
g_signal_connect(gesture, "pressed",
                 G_CALLBACK(on_user_label_clicked), user_id_ptr);
gtk_widget_add_controller(avatar_or_nick_widget,
                          GTK_EVENT_CONTROLLER(gesture));
```

팝오버 위치: 클릭한 위젯 아래에 `gtk_widget_set_parent(popover, widget)` + `gtk_popover_popup(popover)`.

---

## 6. CSS

```css
.user-profile-popover {
  min-width: 280px;
  padding: 0;
}
.user-profile-popover .action-row {
  gap: 8px;
}
.user-profile-popover button {
  font-size: 13px;
  padding: 6px 10px;
}
```

---

## 7. 접근성

- `Tab`으로 버튼 포커스 이동.
- `Escape` → 팝오버 닫기.
- 버튼에 `gtk_accessible_update_property()` 로 `GTK_ACCESSIBLE_PROPERTY_DESCRIPTION` 설정.
  - 예: "DM 보내기 — 앨리스에게 개인 메시지"
