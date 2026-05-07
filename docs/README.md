# C Console Chat Application — 문서 인덱스

**버전**: 2.0.0 | **언어**: C (C11) | **플랫폼**: Windows

카카오톡/Google Chat 스타일 실시간 채팅 애플리케이션.  
TCP Server-Client · 멀티스레드 · **콘솔 UI** · **텍스트 파일 영속 저장**

> **기술 결정**: reference 파일 분석 결과 콘솔 기반 + users.txt 구조 확인  
> → GTK4 GUI 제거, MySQL 제거 → 콘솔 메뉴 + `.txt` 파일 사용

---

## 기술 스택

| 항목 | 내용 |
|------|------|
| 언어 | C11 |
| UI | 콘솔 터미널 (printf / scanf / getch) |
| 네트워크 | Windows WinSock2 |
| 스레드 | `_beginthreadex`, `CreateMutex` |
| 영속 저장 | 텍스트 파일 (`data/*.txt`) |
| 빌드 | MinGW gcc / MSVC |
| 포트 | 55555 |
| 최대 접속 | 256명 (`MAX_CLIENTS`) |

---

## 문서 구조

### 필수 문서

| 폴더 | 파일 | 내용 |
|------|------|------|
| `overview/` | `project_summary.md` | 프로젝트 개요, 범위, 기술 결정 근거 |
| | `system_architecture.md` | 전체 시스템 아키텍처 다이어그램 |
| | `requirements_traceability.md` | requirements.md ↔ docs 추적성 매트릭스 |
| `architecture/` | `server_architecture.md` | 서버 모듈 구조, 스레드 모델, router 디스패치 |
| | `client_architecture.md` | 클라이언트 모듈 구조 |
| | `module_common.md` | 공통 모듈 (`protocol.h`, `types.h`, `utils.c/h`) |
| | `module_auth.md` | 인증 모듈 (로그인/회원가입) |
| | `module_friend.md` | 친구 관리 모듈 |
| | `module_room.md` | 채팅방 모듈 (히스토리 open_nick 포함) |
| | `module_dm.md` | DM 모듈 |
| | `module_message.md` | 메시지 처리 모듈 |
| | `module_broadcast.md` | 브로드캐스트 모듈 |
| `file_structure/` | `project_layout.md` | 전체 프로젝트 파일 트리 및 역할 |
| `uiux/` | `console_ui_design.md` | 메인 화면 41종 ASCII 레이아웃 |
| | `menu_flow.md` | 메뉴 전환 흐름 14종, 슬래시 명령어 목록 |
| | `screen_inventory.md` | 전체 110+ 화면/알림/오류 인벤토리 |
| | `dialogs_and_actions.md` | 확인 다이얼로그·액션 메뉴 27종 |
| | `help_and_guides.md` | /help, 이모티콘, 명령 가이드 9종 |
| | `error_and_recovery.md` | 오류·복구 화면 13종 |
| | `notification_patterns.md` | 비동기 알림 표시 패턴 17종 |
| `features/` | `FR_A_account.md` | 계정 관리 (FR-A01~A06) |
| | `FR_F_friend.md` | 친구 관리 (FR-F01~F07) |
| | `FR_D_dm.md` | 1:1 DM (FR-D01~D05) |
| | `FR_G_group_chat.md` | 그룹 채팅 (FR-G01~G10) |
| | `FR_O_open_chat.md` | 오픈채팅 (FR-O01~O05) |
| | `FR_M_message.md` | 메시지 기능 (FR-M01~M11) |
| | `FR_N_notification.md` | 알림 (FR-N01~N06) |
| | `FR_C_customization.md` | 커스터마이징 (FR-C01~C07) |
| `security/` | `security_overview.md` | 위협 모델 및 보안 대책 목록 |
| | `password_hashing.md` | SHA-256 WinCrypt 구현 코드 |
| | `input_validation.md` | 금지문자 검증, 버퍼 안전, 스레드 안전 |
| `database/` | `file_schema.md` | 9개 txt 파일 스키마 (room_reads.txt 포함) 및 포맷 규칙 |
| | `in_memory_structures.md` | 서버 인메모리 구조체·전역변수·Mutex (C 코드) |

### 추가 문서

| 폴더 | 파일 | 내용 |
|------|------|------|
| `protocol/` | `packet_format.md` | 패킷 형식, 구분자 규칙, C 파싱 예시 |
| | `packet_reference.md` | 전체 패킷 정의 레퍼런스 표 |
| `build/` | `build_guide.md` | MinGW/MSVC 빌드 명령, Makefile 예시 |
| `development/` | `development_phases.md` | Phase 0~3 구현 범위 및 체크리스트 |
| | `implementation_guide.md` | 구현 주의사항 12가지, 코딩 컨벤션 |
| | `nfr_checklist.md` | 비기능 요구사항 (NFR-01~09) 체크리스트 |

---

## 개발 우선순위 요약

| Phase | 범위 | 핵심 목표 |
|-------|------|-----------|
| **P0** | FR-A01~A03, FR-G01/G03/G05, FR-O01/O02/O04 | MVP: 로그인 + 방 생성/입장/채팅 |
| **P1** | FR-F01~F07, FR-D01~D05, FR-A04~A06, FR-P01~P06 | 소셜: 친구·DM·마이페이지 |
| **P2** | FR-M01~M03, FR-G06~G08, FR-C01~C07, FR-N01~N02 | 완성도: 수정·알림·설정 |
| **P3** | FR-M04/M06/M08/M10/M11, FR-N03~N06, FR-O03 | 풍부함: 답장·검색·타이핑 |

---

## 데이터 파일 (`data/`)

```
users.txt          유저 정보 (id, 비번해시, 닉네임, 상태메시지 등)
rooms.txt          채팅방 정보 (이름, 주제, 방장, 공지 등)
messages.txt       메시지 히스토리 (DM 포함, room_id=0 이면 DM)
friends.txt        친구 관계 (pending/accepted/blocked)
room_members.txt   채팅방 멤버 (오픈닉, 관리자 여부, 알림 무음)
dm_reads.txt       DM 읽음 상태 (메시지 단위)
room_invites.txt   오프라인 초대 대기 목록
user_settings.txt  색상·테마·타임스탬프·DND·welcome_shown 설정
room_reads.txt     그룹/오픈채팅 읽음 상태 (사용자별 last_read_msg_id) — FR-D05·FR-G09
```

---

## 핵심 상수

```c
#define DEFAULT_PORT      55555
#define MAX_CLIENTS         256
#define MAX_ROOMS           100
#define MAX_ROOM_MEMBERS     64
#define MAX_PKT_SIZE      10240
```
