# C Console Chat — 설계 문서

**버전**: 2.0.0-design.1
**기준**: [`../requirements.md`](../requirements.md) v2.0.0
**언어**: 한국어 (기술 용어만 영어)
**다이어그램**: [Mermaid](https://mermaid.js.org/)

## 문서 구성

| # | 섹션 | 목적 |
|---|------|------|
| 01 | [overview](./01_overview/) | 프로젝트 목적, 타깃 플랫폼, NFR, 용어 |
| 02 | [features](./02_features/) | 기능 요구사항(FR) 상세 |
| 03 | [architecture](./03_architecture/) | 시스템 구성, 스레딩, 데이터 흐름 |
| 04 | [file_structure](./04_file_structure/) | 소스 트리, 모듈 책임, 네이밍 |
| 05 | [security](./05_security/) | 인증, 검증, 권한, 위협 모델 |
| 06 | [ui_ux](./06_ui_ux/) | 화면/명령어/테마/렌더링 전략 |
| 07 | [database](./07_database/) | 스키마, 쿼리, 연결 관리 |
| 08 | [api](./08_api/) | 패킷 프로토콜 명세 |
| 09 | [exception](./09_exception/) | 예외 분류·처리·엣지 케이스 |
| 10 | [todo_list](./10_todo_list/) | 단계별 로드맵·수용 기준·리스크 |

## 읽는 순서

**처음 읽는 사람**: `01 → 02 → 03 → 06 → 07 → 08 → 05 → 09 → 04 → 10`
**구현자**: `04 → 08 → 07 → 03 → 09` 순서로 참조
**리뷰어**: `01 → 10 → 02 → 05 → 09`

## 문서 규약

- 파일/함수/매크로/패킷 타입은 `backtick` 으로 표기.
- 모든 FR/NFR 는 `requirements.md` 의 ID 를 **그대로** 사용(FR-A01, NFR-05 등).
- 다이어그램은 mermaid 코드블록으로만 작성(이미지 파일 금지).
- 시간/날짜 기반 일정 표기는 금지(우선순위 P0~P4 로 대체).
