# 09 · Exception Handling

| 문서 | 내용 |
|------|------|
| [error_taxonomy.md](./error_taxonomy.md) | 에러 분류 체계 |
| [server_error_handling.md](./server_error_handling.md) | 서버 에러 전파·복구 |
| [client_error_handling.md](./client_error_handling.md) | 클라이언트 에러 처리·재연결 |
| [db_failure_modes.md](./db_failure_modes.md) | DB 장애 모드 |
| [edge_cases.md](./edge_cases.md) | 엣지 케이스 카탈로그 |
| [logging_and_diagnostics.md](./logging_and_diagnostics.md) | 로그 포맷·레벨 |

## 원칙

1. **절대 크래시 금지** (NFR-02): 모든 예외는 세션 종료까지만 영향, 프로세스 보호.
2. **조용한 실패 금지**: 사용자 가시 실패는 code 로 전달, 내부 실패는 로그 남김.
3. **리소스 누수 방지**: mutex, fd, MYSQL*, MYSQL_STMT*, malloc — 모두 오류 경로에서 반드시 해제.
