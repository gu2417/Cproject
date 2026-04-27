# 08 · API (Packet Protocol)

서버-클라이언트 간 **텍스트 라인 기반 프레이밍 프로토콜**. JSON/Protobuf 사용하지 않음(저사양 파싱 부담 최소화).

| 문서 | 내용 |
|------|------|
| [packet_format.md](./packet_format.md) | 프레임/필드 규칙, 이스케이프, 한계값 |
| [error_codes.md](./error_codes.md) | 공통 `*_RES` 코드 표 |
| [packets/](./packets/) | 카테고리별 상세(auth, friend, dm, room, message, settings, mypage, typing, notify, admin, keepalive) |
| [sequence_diagrams.md](./sequence_diagrams.md) | 주요 플로우 sequence diagram |

## 빠른 요약

- 포맷: `TYPE|arg1|arg2|...|content\n`
- 최대 크기: **2048 byte**
- 필드 구분자: `|`, 다중값 구분자: `:` , 리스트 엔트리 구분자: `;`
- 모든 요청에는 대응 `*_RES` 가 있음(단, `*_NOTIFY` 는 서버 단방향 push).
- content(자유 텍스트) 는 항상 **마지막 필드**.
