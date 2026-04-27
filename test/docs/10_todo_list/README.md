# 구현 TODO 리스트

## 개요

이 폴더는 `docs/01~09`의 설계 문서를 기반으로 작성된 단계별 구현 TODO 목록을 담고 있다.  
각 항목에는 구현 파일 경로, 참조 문서, 완료 기준이 명시되어 있다.

## 소스코드 경로 규칙

```
프로젝트 루트/
├── Makefile
├── sql/schema.sql
├── chat_program/src/
│   ├── server/          ← 서버 소스
│   ├── client/          ← 클라이언트 소스
│   │   └── css/         ← GTK4 CSS
│   └── common/          ← 공용 소스
└── docs/
```

## 파일 구성

| 파일 | 내용 | 우선순위 |
|------|------|----------|
| [phase0_foundation.md](./phase0_foundation.md) | 빌드·DB 스키마·공통 레이어·서버/클라이언트 뼈대 | **필수 선행** |
| [phase1_p0_core.md](./phase1_p0_core.md) | 로그인·회원가입·그룹방·오픈채팅 핵심 + 기본 UI | P0 |
| [phase2_p1_social.md](./phase2_p1_social.md) | 친구·DM·마이페이지·프로필 수정 | P1 |
| [phase3_p2_enhance.md](./phase3_p2_enhance.md) | 메시지 고급·커스터마이징·알림·방 관리 | P2 |
| [phase4_p3_advanced.md](./phase4_p3_advanced.md) | 답장·이모티콘·타이핑·keepalive 등 | P3 |
| [cross_cutting.md](./cross_cutting.md) | 보안·예외 처리·로깅 (전 단계 적용) | 상시 |

## 우선순위 기준

- **P0**: 동작하는 MVP (로그인 + 그룹 채팅)
- **P1**: 핵심 소셜 기능 (친구·DM)
- **P2**: 완성도 향상 (알림·설정·메시지 편집)
- **P3**: 선택적 고급 기능

## 의존성 순서

```
Phase 0 → Phase 1 (P0) → Phase 2 (P1) → Phase 3 (P2) → Phase 4 (P3)
                                                           ↑
                                     Cross-Cutting (전 단계 병행)
```

## 이미 생성된 파일

| 파일 | 상태 |
|------|------|
| `chat_program/src/common/protocol.h` | ✅ 생성됨 (내용 검증 필요) |
| `chat_program/src/common/types.h` | ✅ 생성됨 (내용 검증 필요) |
| `chat_program/src/common/utils.c/h` | ✅ 생성됨 (내용 검증 필요) |
| `chat_program/src/client/css/theme-dark.css` | ✅ 생성됨 |
| `chat_program/src/client/css/theme-light.css` | ✅ 생성됨 |
| `chat_program/src/client/css/components.css` | ✅ 생성됨 |
