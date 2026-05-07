# Thread and Mutex Rules

## 3종 Mutex 책임 분리
| Mutex | 보호 대상 |
|-------|----------|
| `g_sessions_mutex` | `g_sessions[]`, `g_rooms[]`, `g_session_count`, `g_room_count` |
| `g_file_mutex` | 모든 `fopen`/`fclose`/`fprintf`/`fgets` 호출 |
| `g_console_mutex` | 클라이언트 `RecvMsg` 스레드 vs 메인 스레드 콘솔 출력 |

## 규칙
- 모든 mutex 보호 영역 진입 전: `WaitForSingleObject(g_xxx_mutex, INFINITE);`
- 모든 mutex 보호 영역 탈출 후: `ReleaseMutex(g_xxx_mutex);`
- 모든 mutex 사용 영역에 `// MUTEX: g_xxx` 주석 필수

## recv 처리
```c
int len = recv(sess->sock, buf, sizeof(buf)-1, 0);
if (len <= 0) {
    leftClient(sess);  // break만 하면 안 됨!
    return 0;
}
```

## 스레드 모델
- 서버: 1 accept 루프 + N HandleClient 스레드 (`_beginthreadex`)
- 클라이언트: 1 main 메뉴 루프 + 1 RecvMsg 스레드
