# 구현 가이드라인

## 1. 금지 함수 및 대체 방법

### gets() 사용 금지

```c
/* 금지 — 버퍼 오버플로 취약점 */
gets(buf);

/* 올바른 대체 */
fgets(buf, sizeof(buf), stdin);
buf[strcspn(buf, "\n")] = '\0';  /* 개행 제거 */

/* 또는 길이 제한 scanf */
scanf("%20s", id);   /* 최대 20자 */
```

### 안전한 문자열 복사

```c
/* 금지 */
strcpy(dest, src);   /* 길이 검사 없음 */

/* 올바른 대체 */
strncpy(dest, src, sizeof(dest) - 1);
dest[sizeof(dest) - 1] = '\0';
```

---

## 2. 파일 NULL 체크 필수

`fopen()` 반환값이 NULL인 경우 (파일 없음)를 반드시 처리해야 한다.

```c
/* 올바른 처리 */
int load_users(const char *path, UserRecord *out, int max_count) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        /* 파일 없음 → 빈 배열로 시작 (정상 동작) */
        return 0;
    }
    int count = 0;
    char line[512];
    while (fgets(line, sizeof(line), fp) && count < max_count) {
        line[strcspn(line, "\n")] = '\0';
        if (strlen(line) == 0) continue;
        parse_user_line(line, &out[count]);
        count++;
    }
    fclose(fp);
    return count;
}

/* 잘못된 처리 — 절대 금지 */
FILE *fp = fopen("users.txt", "r");
while (!feof(fp)) { ... }  /* fp가 NULL이면 세그폴트 */
```

---

## 3. 비밀번호 마스킹

콘솔 클라이언트에서 `getch()`로 비밀번호를 한 자씩 읽어 `*`를 에코한다.

```c
#include <conio.h>

void read_password(char *pw, int max_len) {
    int  i = 0;
    char ch;

    while (1) {
        ch = getch();
        if (ch == 13 || ch == '\n') {  /* Enter: \r (13) */
            pw[i] = '\0';
            printf("\n");
            break;
        } else if (ch == 8) {          /* Backspace (8) */
            if (i > 0) {
                i--;
                printf("\b \b");       /* 커서 뒤로, 공백, 다시 뒤로 */
            }
        } else if (i < max_len - 1) {
            pw[i++] = ch;
            printf("*");
        }
    }
}

/* 사용 예 */
printf("> Enter your Password: ");
read_password(pw, sizeof(pw));
```

---

## 4. 스레드 안전 — Mutex 패턴

공유 자원(`g_sessions[]`, `g_rooms[]`, txt 파일 쓰기)에는 반드시 Mutex를 사용한다.

```c
/* 세션 배열 접근 */
WaitForSingleObject(g_sessions_mutex, INFINITE);
/* --- 임계 구역 시작 --- */
g_sessions[idx].active = 1;
strncpy(g_sessions[idx].user_id, id, 20);
/* --- 임계 구역 끝 --- */
ReleaseMutex(g_sessions_mutex);

/* 파일 쓰기 */
WaitForSingleObject(g_file_mutex, INFINITE);
FILE *fp = fopen(FILE_USERS, "a");
if (fp) {
    fprintf(fp, "%s//%s//...\n", u->id, u->pw_hash);
    fclose(fp);
}
ReleaseMutex(g_file_mutex);
```

**원칙**:
- 임계 구역 내에서 blocking I/O 최소화
- 모든 return 경로에서 `ReleaseMutex()` 보장
- 이중 Lock 금지 (같은 스레드에서 같은 Mutex 두 번 획득 시 교착)

---

## 5. 패킷 파싱 — Content-Last 규칙

자유 텍스트(메시지 본문, 상태메시지, 주제 등)는 항상 마지막 필드다.

```c
/* ROOM_MSG|<room_id>:<content> 파싱 */
char *room_id_str = strtok(payload, ":");
char *content     = strtok(NULL, "");  /* "" = 구분자 없이 나머지 전체 */
int   room_id     = atoi(room_id_str);

/* DM_RECV|<from_id>:<from_nick>:<timestamp>:<msg_id>:<content> 파싱 */
char *from_id   = strtok(payload, ":");
char *from_nick = strtok(NULL, ":");
char *timestamp = strtok(NULL, ":");
int   msg_id    = atoi(strtok(NULL, ":"));
char *content   = strtok(NULL, "");  /* content-last */
```

---

## 6. P0 브로드캐스트

P0에서는 전체 `g_sessions[]` 배열에 일괄 전송한다. 클라이언트가 `room_id`로 필터링한다.

```c
/* 서버 P0 */
void broadcast_to_all(const char *packet) {
    int len = (int)strlen(packet);
    WaitForSingleObject(g_sessions_mutex, INFINITE);
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (g_sessions[i].active)
            send(g_sessions[i].fd, packet, len, 0);
    ReleaseMutex(g_sessions_mutex);
}

/* 클라이언트 P0 — room_id 필터링 */
if (strcmp(type, "ROOM_MSG_RECV") == 0) {
    int room_id = atoi(strtok(payload, ":"));
    /* ... */
    if (room_id == g_state.current_room_id) {
        /* 현재 방 메시지만 출력 */
        display_chat_message(...);
    }
    /* 다른 방 메시지는 무시 또는 알림 */
}
```

---

## 7. 비정상 종료 처리

```c
/* HandleClient 스레드에서 반드시 처리 */
strLen = recv(clientSock, msg, sizeof(msg), 0);
if (strLen > 0) {
    router(msg, sess);
} else {
    /* strLen == 0: 정상 종료 */
    /* strLen  < 0: 네트워크 오류 (비정상 종료) */
    leftClient(sess);
    break;
}
```

---

## 8. 파일 포맷 구분자

reference 구현(`server_main.c`) 준수: 구분자 `//` 사용.

```c
/* users.txt 파싱 */
char *id         = strtok(line, "//");
char *pw_hash    = strtok(NULL, "//");
char *nickname   = strtok(NULL, "//");
char *status_msg = strtok(NULL, "//");
/* ... */

/* users.txt 저장 */
fprintf(fp, "%s//%s//%s//%s//%d//%d//%s//%s\n",
        u->id, u->pw_hash, u->nickname, u->status_msg,
        u->online_status, u->is_admin, u->last_seen, u->created_at);
```

**주의**: 필드 값에 `//` 문자열 포함 불가.

---

## 9. SHA-256 구현

WinCrypt API 사용. `advapi32.lib` 링크 필요.

```c
#include <winsock2.h>
#include <Windows.h>
#include <wincrypt.h>

void sha256_hex(const char *input, char out_hex[65]) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    BYTE  hash[32];
    DWORD hash_len = 32;

    if (!CryptAcquireContext(&hProv, NULL, NULL,
                             PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) return;
    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0); return;
    }
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

## 10. 타임스탬프 유틸리티

```c
/* utils.c */
#include <time.h>

/* 현재 시각을 "YYYY-MM-DD HH:MM:SS" 형식으로 반환 */
void get_current_timestamp(char out[20]) {
    time_t    now = time(NULL);
    struct tm *t  = localtime(&now);
    strftime(out, 20, "%Y-%m-%d %H:%M:%S", t);
}

/* "YYYY-MM-DD HH:MM:SS" 문자열을 time_t로 변환 */
time_t parse_timestamp(const char *ts) {
    struct tm t = {0};
    sscanf(ts, "%d-%d-%d %d:%d:%d",
           &t.tm_year, &t.tm_mon, &t.tm_mday,
           &t.tm_hour, &t.tm_min, &t.tm_sec);
    t.tm_year -= 1900;
    t.tm_mon  -= 1;
    return mktime(&t);
}
```

---

## 11. 코딩 컨벤션

| 항목 | 규칙 | 예시 |
|------|------|------|
| 함수명 | snake_case | `handle_login`, `send_packet`, `broadcast_to_room` |
| 구조체 타입명 | PascalCase | `ClientSession`, `RoomInfo`, `MessageRecord` |
| 상수 | UPPER_CASE `#define` | `MAX_CLIENTS`, `DEFAULT_PORT`, `FILE_USERS` |
| 전역 변수 | `g_` 접두사 | `g_sessions`, `g_rooms`, `g_sessions_mutex` |
| 지역 변수 | snake_case | `room_id`, `from_nick`, `pw_hash` |
| 헤더 가드 | `#ifndef FILENAME_H` | `#ifndef AUTH_H` / `#define AUTH_H` / `#endif` |

---

## 12. 디버그 코드 금지

커밋 전 아래 항목 반드시 제거:

```c
/* 제거 필수 */
printf("DEBUG: %s %d\n", ...);   /* 디버그 출력 */
// TODO: 나중에 고칠 것                /* TODO 주석 */
// HACK: 임시 처리                    /* HACK 주석 */
```

서버 운영용 로그는 `printf` 대신 별도 로그 함수 사용 권장:

```c
void server_log(const char *fmt, ...) {
    char ts[20];
    get_current_timestamp(ts);
    va_list args;
    va_start(args, fmt);
    printf("[%s] ", ts);
    vprintf(fmt, args);
    va_end(args);
}

/* 사용 예 */
server_log("Connected: %s\n", inet_ntoa(clientAddr.sin_addr));
server_log("Login: %s\n", user_id);
```
