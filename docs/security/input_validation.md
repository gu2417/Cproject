# 입력 검증 및 버퍼 안전

## 금지 함수

| 금지 | 대체 | 이유 |
|------|------|------|
| `gets()` | `fgets(buf, size, stdin)` | 버퍼 오버플로우 무방비 |
| `scanf("%s", ...)` | `scanf("%19s", ...)` | 크기 제한 없음 |
| `strcpy()` (검증 없이) | `strncpy()` + 널 종단 보장 | 오버플로우 |
| `strcat()` (검증 없이) | `strncat()` | 오버플로우 |
| `sprintf()` (검증 없이) | `snprintf()` | 오버플로우 |

---

## 프로토콜 구분자 금지문자

패킷 필드에 아래 문자가 포함되면 파싱이 깨짐 → 입력 시 즉시 거부.

| 문자 | 역할 | 금지 필드 |
|------|------|-----------|
| `\|` (0x7C) | 패킷 타입 구분자 | ID, 닉네임, 비밀번호, 방이름, 비밀번호 |
| `:` (0x3A) | 필드 내부 구분자 | ID, 닉네임, 비밀번호, 방이름 |
| `;` (0x3B) | 리스트 항목 구분자 | 동일 |
| `\n` (0x0A) | 패킷 종단자 | 모든 필드 |

```c
/* 금지문자 포함 여부 확인. 포함=1(거부), 없음=0(허용) */
int has_forbidden_chars(const char *s) {
    for (; *s; s++) {
        if (*s == '|' || *s == ':' || *s == ';' || *s == '\n')
            return 1;
    }
    return 0;
}
```

---

## 필드별 최대 길이 상수

```c
/* config.h */
#define MAX_ID_LEN       20
#define MAX_PW_LEN       19
#define MAX_NICK_LEN     20
#define MAX_STATUS_LEN  100
#define MAX_ROOM_NAME    30
#define MAX_TOPIC_LEN   100
#define MAX_ROOM_PW      10
#define MAX_CONTENT_LEN 500
#define MAX_OPEN_NICK    20
#define MAX_NOTICE_LEN  255
```

---

## 파일 I/O 안전

```c
/* fopen 반환값 필수 체크 — NULL이면 빈 배열로 시작 */
FILE *fp = fopen("data/users.txt", "r");
if (fp == NULL) {
    /* 파일 없음 → 빈 사용자 목록으로 시작 (정상 케이스) */
    return;
}
/* fp가 NULL인 상태에서 feof(NULL) 호출 시 세그폴트 발생 — 절대 금지 */
```

---

## 스레드 안전 패턴

```c
/* 전역 배열 접근 전 반드시 mutex 획득 */
WaitForSingleObject(g_mutex, INFINITE);
/* ... g_sessions[], g_rooms[] 접근 ... */
ReleaseMutex(g_mutex);

/* 파일 쓰기도 mutex 보호 (동시 쓰기 방지) */
WaitForSingleObject(g_file_mutex, INFINITE);
FILE *fp = fopen("data/users.txt", "a");
/* ... 쓰기 ... */
fclose(fp);
ReleaseMutex(g_file_mutex);
```
