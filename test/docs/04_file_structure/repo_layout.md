# 리포지토리 레이아웃

```
C_ChatProgram/
├── chat_program/
│   └── src/
│       ├── server/               # 서버 전용 소스
│       ├── client/               # 클라이언트 전용 소스
│       │   └── css/              # GTK4 CSS 테마 파일
│       └── common/               # 양쪽에서 공유되는 소스·헤더
├── sql/
│   └── schema.sql                # DB 생성·DDL·시드
├── docs/                         # 본 설계 문서(10 섹션)
├── Makefile                      # Linux/MinGW 통합 빌드
├── requirements.md               # 요구사항 명세 v2.0.0
├── CLAUDE.md                     # AI 어시스턴트 가이드
└── README.md                     # 프로젝트 소개(추후)
```

## 폴더별 책임

| 폴더 | 책임 | 의존 |
|------|------|------|
| `chat_program/src/common/` | 패킷 타입 상수, 공용 구조체, 유틸리티(문자열/시간/해시 래퍼), 플랫폼 소켓 호환 | 표준 C, 플랫폼 네트워크 헤더 |
| `chat_program/src/server/` | TCP listen, 멀티스레드 핸들러, 기능 처리, MySQL 접근 | `common/`, pthread, libmysqlclient |
| `chat_program/src/client/` | GTK4 GUI, GtkStack 화면 전환, g_idle_add UI 업데이트, send/recv 스레드 | `common/`, pthread, GTK4 |
| `sql/` | 초기화 SQL | (없음) |
| `docs/` | 설계 문서 | (없음) |

## 코드가 아닌 파일

- `Makefile` 에는 컴파일 규칙만. 플랫폼 감지/라이브러리 경로는 `uname -s` 분기(03장 build_and_portability 참조).
- 이진 산출물(`chat_server`, `chat_client`)은 `chat_program/src/server/`·`chat_program/src/client/` 아래에 생성. `.gitignore` 로 제외.
