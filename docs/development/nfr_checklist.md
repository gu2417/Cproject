# 비기능 요구사항 체크리스트

requirements.md 섹션9 기반, 콘솔 + 텍스트 파일 환경 반영.

---

## NFR 체크리스트

| ID | 항목 | 목표 | 구현 방법 | 확인 |
|----|------|------|-----------|------|
| NFR-01 | 동시 접속 | 최대 256명 | `MAX_CLIENTS=256`, per-client thread (`_beginthreadex`) | [ ] |
| NFR-02 | 응답 지연 | 50ms 이하 | 로컬 네트워크 기준, 인메모리 처리 우선 | [ ] |
| NFR-03 | 안정성 | 클라이언트 비정상 종료 시 서버 크래시 없음 | `recv()` 0/음수 반환 시 `leftClient()` 호출, 소켓 닫기 | [ ] |
| NFR-04 | 보안 | SHA-256 해시 저장, gets() 사용 금지 | WinCrypt API (`CALG_SHA_256`), `fgets()`/`scanf("%Ns")` 사용 | [ ] |
| NFR-05 | 경량성 | 서버 메모리 100MB 이하 | 인메모리 구조체 크기 계산 (아래 참조) | [ ] |
| NFR-06 | 이식성 | Windows 단일 빌드 | WinSock2, `_beginthreadex`, `CreateMutex` — Linux/Mac 불필요. **requirements.md의 "GTK4 4.0+ 필요" 의존성은 콘솔 UI 결정에 따라 제거** (project_summary.md 참조) | [ ] |
| NFR-07 | 확장성 | 서버/클라이언트 분리 | 모듈화 파일 구조 (auth/room/friend/dm/message 분리) | [ ] |
| NFR-08 | 영속성 | 재시작 후 데이터 유지 | txt 파일 저장, 서버 시작 시 전체 로드 | [ ] |
| NFR-09 | 스레드 안전 | 공유 자원 경쟁 없음 | `CreateMutex`/`WaitForSingleObject`/`ReleaseMutex` | [ ] |

---

## NFR-01: 동시 접속 256명 상세

```c
#define MAX_CLIENTS 256

ClientSession g_sessions[MAX_CLIENTS];
/* 크기: sizeof(ClientSession) × 256 */
```

per-client 스레드 모델:
- accept 시 빈 슬롯 탐색 → `g_sessions[i]` 등록 → `_beginthreadex`
- 슬롯 소진 시 새 연결 거부 (accept 후 즉시 close)

---

## NFR-02: 응답 지연 50ms

로컬 네트워크(`127.0.0.1`) 기준. 주요 지연 요소:

| 요소 | 영향 | 대책 |
|------|------|------|
| 파일 I/O (txt 쓰기) | 중간 | Mutex 경합 최소화, append 우선 사용 |
| Mutex 경합 | 중간 | 임계 구역 최소화, send() 후 즉시 ReleaseMutex |
| 브로드캐스트 루프 | 낮음 | 256명 순차 send() — 로컬에서 무시할 수준 |

---

## NFR-03: 안정성 — 비정상 종료 처리

```c
/* HandleClient 스레드 */
while (1) {
    strLen = recv(clientSock, msg, sizeof(msg), 0);
    if (strLen > 0) {
        router(msg, sess);
    } else if (strLen == 0) {
        /* 클라이언트 정상 종료 */
        leftClient(sess);
        break;
    } else {
        /* 네트워크 오류 — 비정상 종료 */
        leftClient(sess);
        break;
    }
}
```

`leftClient()` 보장 사항:
- 참여 중인 방에서 퇴장 시스템 메시지 브로드캐스트
- `online_status → 0`, `last_seen` 갱신
- 친구들에게 `FRIEND_STATUS_CHANGE` 전송
- 세션 슬롯 초기화 (`active=0`)
- `closesocket()` 호출

---

## NFR-04: 보안

### gets() 사용 금지

```c
/* 금지 */
gets(buf);

/* 올바른 사용 */
fgets(buf, sizeof(buf), stdin);
buf[strcspn(buf, "\n")] = '\0';  /* 개행 제거 */

/* 또는 */
scanf("%20s", id);   /* 최대 길이 명시 */
```

### SHA-256 해시

```c
/* WinCrypt API 사용 */
#include <wincrypt.h>
#pragma comment(lib, "Advapi32.lib")

void sha256_hex(const char *input, char out_hex[65]) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    BYTE  hash[32];
    DWORD hash_len = 32;
    CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT);
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

### 비밀번호 마스킹 (클라이언트)

```c
#include <conio.h>

void read_password(char *pw, int max_len) {
    int i = 0;
    char ch;
    while (1) {
        ch = getch();
        if (ch == 13) { pw[i] = '\0'; printf("\n"); break; }
        else if (ch == 8 && i > 0) { i--; printf("\b \b"); }
        else if (i < max_len - 1) { pw[i++] = ch; printf("*"); }
    }
}
```

---

## NFR-05: 경량성 — 메모리 사용량 계산

```
ClientSession 구조체 크기 (근사):
  SOCKET(8) + user_id(21) + nickname(21) + online_status(4)
  + dnd(4) + current_room_id(4) + muted_rooms(128) + muted_count(4)
  + is_admin(4) + active(4) + hThread(8) = ~210 bytes

MAX_CLIENTS = 256:
  210 × 256 = ~53,760 bytes ≈ 53 KB

RoomInfo 구조체 크기 (근사):
  id(4) + name(31) + topic(101) + pw_hash(65) + max_users(4)
  + owner_id(21) + notice(256) + is_open(4) + pinned_msg_id(4)
  + member_count(4) + member_ids(64×21=1344) + admin_flags(64×4=256)
  + active(4) = ~2,098 bytes

MAX_ROOMS = 100:
  2,098 × 100 = ~209,800 bytes ≈ 205 KB

인메모리 메시지 캐시 (MAX_MSG_HISTORY=1000):
  MessageRecord ≈ 600 bytes × 1000 = ~600 KB

파일 I/O 레코드 배열 (users, friends 등):
  ~2 MB 추정

총합: << 100 MB  ✓
```

---

## NFR-08: 영속성 — txt 파일 저장 전략

| 연산 | 방식 | 성능 |
|------|------|------|
| 레코드 추가 | `fopen("a")` append | 빠름 |
| 레코드 수정 | 전체 재작성 `fopen("w")` | 느림 (파일 크기 작으므로 허용) |
| 레코드 삭제 | 전체 재작성 | 느림 (허용) |
| 서버 시작 시 로드 | `fopen("r")` 전체 읽기 | 1회성 |

서버 재시작 후 `g_next_room_id`, `g_next_msg_id` 복원:

```c
/* 서버 시작 시 파일 로드 완료 후 — restore_next_ids() */
g_next_room_id = 1;
for (int i = 0; i < g_room_count; i++)
    if (g_rooms[i].id >= g_next_room_id)
        g_next_room_id = g_rooms[i].id + 1;

g_next_msg_id = 1;
for (int i = 0; i < g_msg_count; i++)
    if (g_messages[i].id >= g_next_msg_id)
        g_next_msg_id = g_messages[i].id + 1;

g_next_friend_id = 1;
for (int i = 0; i < g_friend_count; i++)
    if (g_friends[i].id >= g_next_friend_id)
        g_next_friend_id = g_friends[i].id + 1;

g_next_invite_id = 1;
for (int i = 0; i < g_room_invite_count; i++)
    if (g_room_invites[i].id >= g_next_invite_id)
        g_next_invite_id = g_room_invites[i].id + 1;
```

---

## NFR-09: 스레드 안전

```
공유 자원          Mutex               접근 패턴
-----------        ---------------     -------------------------
g_sessions[]       g_sessions_mutex    모든 HandleClient 스레드
g_rooms[]          g_sessions_mutex    방 생성/삭제/멤버 변경
g_client_count     g_sessions_mutex    accept 스레드 + leftClient
txt 파일 쓰기       g_file_mutex        모든 file_io write 연산
```

Mutex 사용 원칙:
1. 임계 구역 최소화 — Mutex 보유 중 blocking I/O 최소화
2. 이중 Lock 금지 — 재귀 획득 시 교착 발생
3. 모든 return 경로에서 `ReleaseMutex()` 보장
