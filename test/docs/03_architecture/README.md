# 03 · Architecture

시스템 구조, 스레딩, 데이터 흐름, 빌드/이식성 전략.

| 문서 | 내용 |
|------|------|
| [system_context.md](./system_context.md) | 전체 구성 (Client↔Server↔MySQL) |
| [server_components.md](./server_components.md) | 서버 내부 계층 |
| [client_components.md](./client_components.md) | 클라이언트 내부 계층 |
| [threading_model.md](./threading_model.md) | 스레드 모델, mutex, DB 연결 |
| [session_lifecycle.md](./session_lifecycle.md) | 세션 수명주기 시퀀스 |
| [data_flow.md](./data_flow.md) | 메시지 송수신·브로드캐스트 |
| [build_and_portability.md](./build_and_portability.md) | Makefile, 플랫폼 분기 |
