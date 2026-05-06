# 패킷 형식

## 1. 기본 형식

```
<TYPE>|<PAYLOAD>\n
```

- `TYPE`: 패킷 종류를 나타내는 대문자 문자열 (예: `LOGIN_REQ`, `ROOM_MSG`)
- `PAYLOAD`: 타입별 데이터 필드
- 종단 문자: `\n` (LF, 0x0A)
- 최대 패킷 길이: **10240 bytes** (목록 응답 포함)

## 2. 구분자 규칙

| 구분자 | 역할 | 예시 |
|--------|------|------|
| `\|` | 패킷 TYPE과 PAYLOAD 분리 | `LOGIN_REQ\|alice:pass123` |
| `:` | PAYLOAD 내 필드 구분 | `alice:pass123` |
| `;` | 리스트 항목 구분 | `alice:홍길동:1;bob:김철수:0` |

## 3. Content-Last 규칙

자유 텍스트 필드(메시지 본문, 상태메시지, 주제 등)는 **항상 해당 패킷의 마지막 필드**로 배치한다.

**이유**: 자유 텍스트 내부에 `:` 문자가 포함될 수 있으므로, 파서는 고정 필드 N개를 파싱한 후 나머지 전체를 자유 텍스트로 처리한다.

**자유 텍스트 내 금지 문자**:
- `\n`: 패킷 종단자 → 포함 불가
- `;`: 리스트 구분자 → 포함 불가
- `|`: 패킷 타입 구분자 → 포함 불가

**예시** (올바른 파싱):
```
ROOM_MSG|42:안녕하세요, 오늘 날씨가 좋네요: 맞죠?
         ^  ^---------- content (마지막 필드, ':' 포함 허용) --------^
     room_id
```

## 4. Count 규칙 (리스트 패킷)

자유 텍스트 필드를 포함하는 리스트 패킷은 PAYLOAD 첫 번째 필드로 `<count>`(항목 수)를 포함한다.

**해당 패킷**:
- `FRIEND_LIST_RES`
- `USER_SEARCH_RES`
- `DM_HISTORY_RES`
- `ROOM_HISTORY_RES`
- `MSG_SEARCH_RES`
- `DM_LIST_RES`

파서는 count 값으로 루프 횟수를 결정하며, `;`는 보조 구분자로만 사용한다.

```c
// 리스트 파싱 예시
char *count_str = strtok(payload, ":");
int count = atoi(count_str);
for (int i = 0; i < count; i++) {
    char *id       = strtok(NULL, ":");
    char *nick     = strtok(NULL, ":");
    char *status   = strtok(NULL, ":");
    char *stat_msg = strtok(NULL, ";");  // 마지막 필드는 ';'로 분리
    // 처리 ...
}
```

## 5. 금지 문자 목록 (등록/입력 시)

ID, 닉네임, 비밀번호, 방 이름, 비밀번호에 아래 문자 사용 불가:

| 문자 | 이유 |
|------|------|
| `:` | 필드 구분자 충돌 |
| `;` | 리스트 구분자 충돌 |
| `\|` | 패킷 타입 구분자 충돌 |
| `\n` | 패킷 종단자 충돌 |

## 6. 파싱 예시 (C 코드)

### 6-1. 기본 패킷 파싱

```c
#define MAX_BUF 10240

// 수신 버퍼에서 TYPE 추출
void parse_packet(char *buf, SOCKET sock) {
    char tmp[MAX_BUF];
    strncpy(tmp, buf, MAX_BUF - 1);
    tmp[MAX_BUF - 1] = '\0';

    // '\n' 제거
    char *nl = strchr(tmp, '\n');
    if (nl) *nl = '\0';

    // TYPE 추출 (첫 번째 '|' 기준)
    char *type    = strtok(tmp, "|");
    char *payload = strtok(NULL, "");  // 나머지 전체를 payload로

    if (!type) return;

    if (strcmp(type, "LOGIN_REQ") == 0) {
        char *id = strtok(payload, ":");
        char *pw = strtok(NULL, ":");    // 비밀번호 (마지막 필드)
        handle_login(id, pw, sock);
    }
    else if (strcmp(type, "ROOM_MSG") == 0) {
        char *room_id_str = strtok(payload, ":");
        char *content     = strtok(NULL, "");  // content-last: 나머지 전체
        int   room_id     = atoi(room_id_str);
        handle_room_msg(room_id, content, sock);
    }
    // ... 기타 패킷 처리
}
```

### 6-2. 리스트 패킷 빌드

```c
// 친구 목록 응답 패킷 빌드 예시
// FRIEND_LIST_RES|<count>:<id>:<nick>:<status>:<status_msg>;<id>:...
void build_friend_list_res(char *out, int out_size,
                           FriendEntry *list, int count) {
    int offset = 0;
    offset += snprintf(out + offset, out_size - offset,
                       "FRIEND_LIST_RES|%d", count);
    for (int i = 0; i < count; i++) {
        offset += snprintf(out + offset, out_size - offset,
                           ":%s:%s:%d:%s",
                           list[i].id,
                           list[i].nick,
                           list[i].status,
                           list[i].status_msg);  // 마지막 필드
        if (i < count - 1)
            out[offset++] = ';';
    }
    out[offset++] = '\n';
    out[offset]   = '\0';
}
```

### 6-3. 단순 응답 패킷 전송

```c
// send_packet 헬퍼
void send_packet(SOCKET sock, const char *fmt, ...) {
    char buf[MAX_BUF];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf) - 2, fmt, args);
    va_end(args);
    // '\n' 보장
    size_t len = strlen(buf);
    if (len == 0 || buf[len - 1] != '\n') {
        buf[len++] = '\n';
        buf[len]   = '\0';
    }
    send(sock, buf, (int)len, 0);
}

// 사용 예
send_packet(sock, "LOGIN_RES|0");          // 로그인 성공
send_packet(sock, "REGISTER_RES|1");       // 회원가입 성공
send_packet(sock, "ROOM_CREATE_RES|1:%d", room_id);
```

## 7. 포트 및 연결

```c
#define DEFAULT_PORT  55555
#define SERVER_IP     "127.0.0.1"

// 서버 측
serverAddr.sin_port = htons(DEFAULT_PORT);

// 클라이언트 측
serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
serverAddr.sin_port        = htons(DEFAULT_PORT);
```
