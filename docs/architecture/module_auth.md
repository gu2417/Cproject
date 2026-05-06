# 모듈: auth.c/h — 인증

## 1. 책임

- 회원가입: ID 중복 체크, SHA-256 해시 후 `users.txt` 저장
- 로그인: `users.txt` 조회, SHA-256 해시 비교, 세션 등록
- 중복 로그인 차단 (P1)
- 비밀번호 마스킹은 클라이언트 측 처리 (서버는 해시값만 수신)

---

## 2. 함수 목록

```c
/* auth.h */

/* 로그인 처리 — users.txt에서 id 검색 후 pw_hash 비교 */
void handle_login(const char *id, const char *pw, ClientSession *sess);

/* 회원가입 처리 — 중복 ID 체크 후 users.txt에 추가 */
void handle_register(const char *id, const char *pw,
                     const char *nick, const char *status_msg,
                     ClientSession *sess);

/* 로그아웃 처리 — last_seen 갱신, 세션 초기화 (P1) */
void handle_logout(ClientSession *sess);

/* 비밀번호 변경 (P1) */
void handle_pass_change(const char *old_pw, const char *new_pw,
                        ClientSession *sess);

/* SHA-256 해시 계산 → hex 문자열 반환 */
void sha256_hex(const char *input, char out_hex[65]);

/* ID/닉네임에 금지 문자 포함 여부 검사 */
int contains_forbidden_chars(const char *str);
```

---

## 3. handle_login 상세

```c
void handle_login(const char *id, const char *pw, ClientSession *sess) {
    char pw_hash[65];
    sha256_hex(pw, pw_hash);

    /* users.txt 인메모리 배열에서 검색 */
    WaitForSingleObject(g_sessions_mutex, INFINITE);

    /* P1: 이미 로그인 중인지 확인 */
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_sessions[i].active &&
            strcmp(g_sessions[i].user_id, id) == 0) {
            ReleaseMutex(g_sessions_mutex);
            send_packet(sess->fd, "LOGIN_RES|3");  /* ALREADY_ONLINE */
            return;
        }
    }
    ReleaseMutex(g_sessions_mutex);

    /* users.txt 배열에서 ID + 해시 검증 */
    UserRecord *u = find_user_by_id(id);
    if (!u) {
        send_packet(sess->fd, "LOGIN_RES|1");  /* WRONG_ID */
        return;
    }
    if (strcmp(u->pw_hash, pw_hash) != 0) {
        send_packet(sess->fd, "LOGIN_RES|2");  /* WRONG_PW */
        return;
    }

    /* 세션 등록 */
    WaitForSingleObject(g_sessions_mutex, INFINITE);
    strncpy(sess->user_id,  u->id,       20);
    strncpy(sess->nickname, u->nickname, 20);
    sess->online_status = 1;  /* online */
    sess->dnd           = 0;
    ReleaseMutex(g_sessions_mutex);

    /* online_status 파일 갱신 */
    update_user_online_status(id, 1);

    /* 친구들에게 온라인 알림 */
    notify_friend_status_change(id, 1);

    send_packet(sess->fd, "LOGIN_RES|0");  /* OK */
}
```

---

## 4. handle_register 상세

```c
void handle_register(const char *id, const char *pw,
                     const char *nick, const char *status_msg,
                     ClientSession *sess) {
    /* 1. 금지 문자 검사 */
    if (contains_forbidden_chars(id) ||
        contains_forbidden_chars(pw) ||
        (nick && contains_forbidden_chars(nick))) {
        send_packet(sess->fd, "REGISTER_RES|2");
        return;
    }

    /* 2. 길이 검사 */
    if (strlen(id) > 20 || strlen(pw) > 19) {
        send_packet(sess->fd, "REGISTER_RES|2");
        return;
    }

    /* 3. 중복 ID 검사 */
    if (find_user_by_id(id) != NULL) {
        send_packet(sess->fd, "REGISTER_RES|2");  /* DUPLICATE_ID */
        return;
    }

    /* 4. 닉네임 미입력 시 ID로 대체 */
    const char *actual_nick = (nick && strlen(nick) > 0) ? nick : id;

    /* 5. 비밀번호 해시 */
    char pw_hash[65];
    sha256_hex(pw, pw_hash);

    /* 6. users.txt에 append */
    char created_at[20];
    get_current_timestamp(created_at);

    UserRecord u = {0};
    strncpy(u.id,         id,          20);
    strncpy(u.pw_hash,    pw_hash,     64);
    strncpy(u.nickname,   actual_nick, 20);
    strncpy(u.status_msg, status_msg ? status_msg : "", 100);
    strncpy(u.created_at, created_at, 19);
    u.online_status = 0;
    u.is_admin      = 0;

    WaitForSingleObject(g_file_mutex, INFINITE);
    append_user(FILE_USERS, &u);
    ReleaseMutex(g_file_mutex);

    /* 7. 인메모리 배열에도 추가 */
    add_user_to_cache(&u);

    send_packet(sess->fd, "REGISTER_RES|1");  /* OK */
}
```

---

## 5. SHA-256 해시 구현

WinCrypt API를 사용하여 SHA-256을 계산한다.

```c
#include <wincrypt.h>
#pragma comment(lib, "Advapi32.lib")

void sha256_hex(const char *input, char out_hex[65]) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    BYTE  hash[32];
    DWORD hash_len = 32;

    CryptAcquireContext(&hProv, NULL, NULL,
                        PROV_RSA_AES, CRYPT_VERIFYCONTEXT);
    CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash);
    CryptHashData(hHash, (BYTE*)input, (DWORD)strlen(input), 0);
    CryptGetHashParam(hHash, HP_HASHVAL, hash, &hash_len, 0);

    for (int i = 0; i < 32; i++)
        sprintf(out_hex + i * 2, "%02x", hash[i]);
    out_hex[64] = '\0';

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
}
```

---

## 6. 금지 문자 검사

```c
int contains_forbidden_chars(const char *str) {
    if (!str) return 0;
    while (*str) {
        if (*str == ':' || *str == ';' || *str == '|' || *str == '\n')
            return 1;
        str++;
    }
    return 0;
}
```

---

## 7. 응답 패킷

| 패킷 | 코드 | 의미 |
|------|------|------|
| `LOGIN_RES\|0` | 0 | 로그인 성공 |
| `LOGIN_RES\|1` | 1 | ID 없음 (WRONG_ID) |
| `LOGIN_RES\|2` | 2 | 비밀번호 불일치 (WRONG_PW) |
| `LOGIN_RES\|3` | 3 | 이미 로그인 중 (ALREADY_ONLINE, P1) |
| `REGISTER_RES\|1` | 1 | 회원가입 성공 |
| `REGISTER_RES\|2` | 2 | ID 중복 (DUPLICATE_ID) |
