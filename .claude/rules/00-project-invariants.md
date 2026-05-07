# Project Invariants — DO NOT REVERT

## Stack 결정 (CLAUDE.md §"Key Design Decisions" 잠금)
1. UI는 콘솔 (`menu_*.c/h`). GTK4·CSS·screen_*.c/h 도입 금지.
2. 영속은 `data/*.txt` 9개. MySQL·DDL·`db.c/h` 도입 금지.
3. 소켓 타입은 `SOCKET` (WinSock2). `int` 비교 금지. 비교는 `INVALID_SOCKET`만.
4. `server/admin.c/h` 추가 금지 (FR-ADM01~05 OoS).
5. `common/net_win.c` 추가 금지 (`client/net.c`로 통합됨).

## 위반 시
PR/커밋 차단. `txt-schema-guard` 또는 `packet-auditor` 에이전트가 거부.
