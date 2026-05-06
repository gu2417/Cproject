# FR-A: 계정 관리

## FR-A01 — 회원가입

### 제약조건

| 필드 | 최대 길이 | 금지 문자 | 비고 |
|------|-----------|-----------|------|
| ID | 20자 | `:` `;` `\|` `\n` | 고유값 |
| Password | 19자 | `:` `;` `\|` `\n` | SHA-256 해시 저장 |
| 닉네임 | 20자 | `:` `;` `\|` `\n` | 미입력 시 ID로 대체, 고유값 |
| 상태메시지 | 100자 | `\n` `;` `\|` | 선택 입력 |

### 처리 흐름

```
클라이언트                          서버
    |                                |
    | REGISTER_REQ|id:pw:nick:status |
    |-------------------------------->
    |                                |
    |                    금지문자 검사
    |                    길이 검사
    |                    중복 ID 검사 (users.txt)
    |                    SHA-256(pw) → pw_hash
    |                    users.txt에 append
    |                    인메모리 배열 추가
    |                                |
    | REGISTER_RES|1  (성공)         |
    |<--------------------------------
    또는
    | REGISTER_RES|2  (중복 ID)      |
    |<--------------------------------
```

### users.txt 저장 포맷

```
alice//5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8//홍길동//안녕하세요//0//0////2026-05-07 10:00:00
```

---

## FR-A02 — 로그인

### 처리 흐름

```
클라이언트                          서버
    |                                |
    | LOGIN_REQ|id:pw                |
    |-------------------------------->
    |                                |
    |                    SHA-256(pw) → pw_hash
    |                    users.txt에서 id 검색
    |                    pw_hash 비교
    |                    (P1) 중복 로그인 검사
    |                    세션 등록
    |                    online_status → 1
    |                    친구들에게 FRIEND_STATUS_CHANGE
    |                                |
    | LOGIN_RES|0  (성공)            |
    |<--------------------------------
```

### 중복 로그인 차단 (P1)

- `g_sessions[]`를 순회하여 동일 `user_id`가 `active=1`인 세션 존재 시 `LOGIN_RES|3` 반환
- P0에서는 중복 로그인 허용 (reference 구현 기준)

### 비밀번호 마스킹 구현

```c
void read_password(char *pw, int max_len) {
    int i = 0;
    char ch;
    while (1) {
        ch = getch();
        if (ch == 13 || ch == '\n') {   /* Enter 키 */
            pw[i] = '\0';
            printf("\n");
            break;
        } else if (ch == 8 && i > 0) { /* Backspace */
            i--;
            printf("\b \b");
        } else if (i < max_len - 1) {
            pw[i++] = ch;
            printf("*");
        }
    }
}
```

---

## FR-A03 — 로그아웃

### P0 구현 (소켓 종료)

클라이언트에서 메뉴 선택으로 종료 시:
```c
void exitService(SOCKET sock) {
    closesocket(sock);
    WSACleanup();
    printf("> End of Service.\n");
    exit(0);
}
```

서버는 `recv()` 반환값 0을 감지하여 `leftClient()` 호출 → `online_status=0`, `last_seen` 갱신.

### P1 구현 (LOGOUT_REQ 패킷)

```
C→S: LOGOUT_REQ|
S→C: LOGOUT_RES|OK
```

서버 처리:
1. `online_status → 0`, `last_seen` 갱신 (users.txt)
2. 친구들에게 `FRIEND_STATUS_CHANGE|<id>:<nick>:0` 전송
3. 참여 중인 방에서 퇴장 처리
4. 세션 슬롯 초기화

---

## FR-A04 — 프로필 수정

```
C→S: PROFILE_UPDATE|<nickname>:<status_msg>
S→C: PROFILE_UPDATE_RES|0   (성공)
     PROFILE_UPDATE_RES|1   (닉네임 중복)
```

서버 처리:
1. 닉네임 중복 검사 (users.txt 전체 탐색)
2. 금지 문자 검사
3. users.txt에서 해당 유저 레코드 갱신
4. 인메모리 세션의 `nickname` 필드 갱신
5. 현재 접속 중인 방의 멤버들에게 닉네임 변경 반영 (선택)

---

## FR-A05 — 비밀번호 변경

```
C→S: PASS_CHANGE|<old_pass>:<new_pass>
S→C: PASS_CHANGE_RES|0   (성공)
     PASS_CHANGE_RES|1   (현재 비밀번호 불일치)
```

서버 처리:
1. `SHA-256(old_pass)` → 현재 `pw_hash`와 비교
2. 일치 시 `SHA-256(new_pass)` → `pw_hash` 갱신
3. users.txt 전체 재작성

---

## FR-A06 — 마지막 접속 시간

오프라인 유저 조회 시 `last_seen` 필드를 기반으로 "N분 전" 형식으로 표시.

```c
/* 마지막 접속 시간 → 상대 시간 문자열 변환 */
void format_last_seen(const char *last_seen_str, char *out, int out_len) {
    if (!last_seen_str || strlen(last_seen_str) == 0) {
        strncpy(out, "알 수 없음", out_len - 1);
        return;
    }
    time_t last = parse_timestamp(last_seen_str);
    time_t now  = time(NULL);
    double diff = difftime(now, last);

    if      (diff < 60)         snprintf(out, out_len, "방금 전");
    else if (diff < 3600)       snprintf(out, out_len, "%d분 전", (int)(diff / 60));
    else if (diff < 86400)      snprintf(out, out_len, "%d시간 전", (int)(diff / 3600));
    else                        snprintf(out, out_len, "%d일 전", (int)(diff / 86400));
}
```

마이페이지 및 친구 목록에서 오프라인 유저에게 표시:
```
[OFF] 이영희  | 마지막 접속: 2시간 전
```
