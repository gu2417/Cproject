# C Chat Application

> **C11 기반 GTK4 GUI 실시간 채팅 애플리케이션** — KakaoTalk · Google Chat 스타일의 데스크탑 채팅 클라이언트

[![Language](https://img.shields.io/badge/Language-C11-blue.svg)](https://en.cppreference.com/w/c/11)
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Windows-lightgrey.svg)](#지원-플랫폼)
[![GUI](https://img.shields.io/badge/GUI-GTK4-blueviolet.svg)](#시스템-요구사항)
[![Architecture](https://img.shields.io/badge/Architecture-TCP%20Server--Client-green.svg)](#시스템-아키텍처)
[![Database](https://img.shields.io/badge/Database-MySQL%205.7%2B-orange.svg)](#데이터베이스)
[![Version](https://img.shields.io/badge/Requirements-v2.0.0-informational.svg)](./requirements.md)

---

## 목차

1. [프로젝트 개요](#1-프로젝트-개요)
2. [주요 기능](#2-주요-기능)
3. [시스템 요구사항](#3-시스템-요구사항)
4. [빠른 시작](#4-빠른-시작)
5. [시스템 아키텍처](#5-시스템-아키텍처)
6. [프로젝트 구조](#6-프로젝트-구조)
7. [패킷 프로토콜](#7-패킷-프로토콜)
8. [데이터베이스](#8-데이터베이스)
9. [GUI 화면 구성](#9-gui-화면-구성)
10. [메뉴 및 버튼 구성](#10-메뉴-및-버튼-구성)
11. [보안](#11-보안)
12. [비기능 요구사항](#12-비기능-요구사항)
13. [개발 로드맵](#13-개발-로드맵)
14. [설계 문서](#14-설계-문서)

---

## 1. 프로젝트 개요

### 1-1. 소개

**C Chat Application**은 C11과 GTK4로 작성된 데스크탑 실시간 채팅 애플리케이션입니다. 카카오톡의 1:1 채팅·그룹 채팅·오픈채팅 개념과 Google Chat의 Space 개념을 **GTK4 GUI** 로 구현합니다.

모든 영속 데이터(유저·메시지·채팅방·친구 관계·설정)는 **MySQL**에 저장되며, 서버 재시작 후에도 완전히 복원됩니다. GTK4 기반 네이티브 창을 통해 직관적인 사용자 인터페이스를 제공합니다.

### 1-2. 대상 사용자

- 소규모(≤100 동시 접속) 학습용·사내 커뮤니티 운영자
- GUI가 제한되거나 CPU/메모리가 부족한 저사양 노트북 사용자
- C 네트워크 프로그래밍·멀티스레딩·GTK4 GUI 개발을 학습하는 개발자

### 1-3. 채팅방 유형

| 유형 | 설명 | 참여 방식 |
|------|------|-----------|
| **그룹 채팅방** | 초대 기반 닫힌 방 (카카오톡 단톡방) | 방장 또는 멤버가 친구 초대 |
| **오픈채팅방** | 누구나 목록에서 발견·참여 가능, 선택적 비밀번호 설정 | 목록 조회 후 자유 참여 |
| **1:1 DM** | 친구 또는 ID 직접 입력으로 자동 개설 | 자동 개설 |

### 1-4. 범위

**In-Scope (v2.0.0)**
- TCP Server-Client 분리 구조, 멀티스레드 서버
- MySQL 영속 저장 (유저·친구·방·메시지·리액션·설정)
- GTK4 GUI: 로그인·메인·채팅·마이페이지·설정 창
- 버튼·메뉴 기반 GUI 인터랙션
- 크로스 플랫폼 빌드: **Linux · Windows(MinGW-w64 + GTK4)**
- 보안 기본선: 비밀번호 SHA-256 해시, SQL Prepared Statement 강제

**Out-of-Scope**
- 웹 클라이언트, 파일·이미지 전송, 음성·화상 통화
- TLS 암호화 전송 (평문 전송은 로컬 네트워크 한정)
- 분산 서버·샤딩·로드밸런싱, 외부 푸시 알림 연동

---

## 2. 주요 기능

### 2-1. 계정 관리 (FR-A)

| ID | 기능 | 설명 |
|----|------|------|
| FR-A01 | 회원가입 | ID(최대 20자) · Password(최대 30자) · 닉네임(최대 20자) · 상태메시지 등록 |
| FR-A02 | 로그인 | ID + Password 검증, 중복 로그인 차단 |
| FR-A03 | 로그아웃 | 접속 종료, `last_seen` 갱신, 온라인 상태 해제 |
| FR-A04 | 프로필 수정 | 닉네임, 상태메시지 변경 |
| FR-A05 | 비밀번호 변경 | 현재 PW 확인 후 새 PW 설정 |
| FR-A06 | 마지막 접속 시간 | 오프라인 유저 프로필에 `마지막 접속: N분 전` 표시 |

### 2-2. 친구 관리 (FR-F)

| ID | 기능 | 설명 |
|----|------|------|
| FR-F01 | 친구 추가 | 상대방 ID 검색 후 친구 요청 전송 |
| FR-F02 | 친구 요청 수락/거절 | 수신된 요청 목록에서 처리 |
| FR-F03 | 친구 목록 조회 | 온라인/오프라인 상태 표시 포함 |
| FR-F04 | 친구 삭제 | 친구 목록에서 제거 |
| FR-F05 | 친구 차단 | 차단 시 메시지 수신 차단, 목록에서 숨김 |
| FR-F06 | 온라인 상태 표시 | `[ON]` / `[OFF]` / `[바쁨]` 3가지 상태 |
| FR-F07 | 유저 검색 | ID 또는 닉네임으로 전체 유저 검색 |

### 2-3. 1:1 DM (FR-D)

| ID | 기능 | 설명 |
|----|------|------|
| FR-D01 | DM 시작 | 친구 목록에서 선택하거나 ID 직접 입력으로 채팅방 개설 |
| FR-D02 | 메시지 전송 | 텍스트 메시지 전송 (최대 500자) |
| FR-D03 | 읽음 확인 | 상대방이 읽으면 `[읽음]` 표시 (카카오톡 스타일) |
| FR-D04 | 메시지 히스토리 | 입장 시 최근 50개 메시지 출력 (MySQL 조회) |
| FR-D05 | 안읽은 메시지 수 | 채팅 목록에 미읽 메시지 수 배지 표시 |

### 2-4. 그룹 채팅방 (FR-G)

| ID | 기능 | 설명 |
|----|------|------|
| FR-G01 | 채팅방 생성 | 방 이름(최대 30자) · 주제(최대 100자) · 최대 인원(기본 30명) · 비밀번호 선택 설정 |
| FR-G02 | 멤버 초대 | 방장 또는 멤버가 친구를 채팅방에 초대 |
| FR-G03 | 메시지 전송 | 입장한 방의 전체 참여자에게 브로드캐스트 |
| FR-G04 | 멘션(@) | `@닉네임`으로 특정 사용자 언급, 해당 사용자에게 알림 |
| FR-G05 | 채팅방 나가기 | 퇴장 메시지 브로드캐스트 후 방에서 제거 |
| FR-G06 | 방장 권한 | 멤버 강퇴 · 방 삭제 · 공지 등록 · 핀 메시지 설정 |
| FR-G07 | 공동 방장 | 방장이 특정 멤버에게 관리자 권한 부여/해제 |
| FR-G08 | 공지사항 등록/조회 | 공지 등록 시 입장할 때마다 상단 표시 |
| FR-G09 | 메시지 히스토리 | 입장 시 최근 100개 메시지 출력 (MySQL 조회) |
| FR-G10 | 멤버 목록 조회 | `/members`로 현재 방 참여자 및 온라인 상태 확인 |

### 2-5. 오픈채팅방 (FR-O)

| ID | 기능 | 설명 |
|----|------|------|
| FR-O01 | 오픈채팅방 생성 | 방 이름 · 주제 · 최대 인원 · 비밀번호(선택) 설정, 누구나 발견 가능 |
| FR-O02 | 목록 조회 | 전체 오픈채팅방 목록 (방이름, 현재인원/최대인원, 주제) |
| FR-O03 | 방 검색 | 방 이름 또는 주제 키워드로 검색 |
| FR-O04 | 자유 참여 | 목록에서 선택 후 참여, 비밀번호 방은 입력 후 참여 |
| FR-O05 | 익명 닉네임 | 오픈채팅방 전용 별도 닉네임 설정 가능 (미설정 시 기본 닉네임 사용) |

### 2-6. 메시지 부가기능 (FR-M)

| ID | 기능 | 설명 |
|----|------|------|
| FR-M01 | 귓속말 | `/w <닉네임> <내용>` — 특정 사용자에게만 전달 |
| FR-M02 | 메시지 삭제 | `/del <msg_id>` — 내 메시지 삭제, 상대에게 `삭제된 메시지` 표시 |
| FR-M03 | 메시지 수정 | `/edit <msg_id> <새내용>` — 전송 후 5분 이내 수정, `(수정됨)` 표시 |
| FR-M04 | 답장(인용) | `/reply <msg_id> <내용>` — 특정 메시지 인용 후 답장, 인용 원문 축약 표시 |
| FR-M05 | 리액션 | `/react <msg_id> <이모지>` — 메시지에 텍스트 이모지 반응 추가/취소, 집계 표시 |
| FR-M06 | 이모티콘 변환 | `:smile:` → `(^_^)`, `:heart:` → `<3` 등 텍스트 이모지 세트 |
| FR-M07 | 시스템 메시지 | 입/퇴장 · 초대 · 강퇴 · 공지 등 이벤트를 구분된 색상으로 표시 |
| FR-M08 | 메시지 검색 | `/search <키워드>` — 현재 채팅방 내 메시지 전문 검색 |
| FR-M09 | 타임스탬프 | 모든 메시지에 `[HH:MM]` 형식 시간 표시 (설정에서 형식 변경 가능) |
| FR-M10 | 핀 메시지 | `/pin <msg_id>` — 방장이 방 상단에 중요 메시지 고정 |
| FR-M11 | /me 액션 | `/me <동작>` — 이탤릭 스타일의 액션 메시지 (예: `* 홍길동 손을 흔든다`) |

### 2-7. 알림 (FR-N)

| ID | 기능 | 설명 |
|----|------|------|
| FR-N01 | 메시지 알림 | 현재 보고 있지 않은 채팅방/DM 메시지 수신 시 상단 배너 출력 |
| FR-N02 | 친구 요청 알림 | 로그인 시 및 실시간으로 알림 |
| FR-N03 | 멘션 알림 | `@나` 언급 시 강조 알림 (DND 상태에서도 표시) |
| FR-N04 | DND 모드 | `/dnd` 명령으로 알림 무음 설정 (멘션 제외) |
| FR-N05 | 타이핑 표시 | 상대방이 입력 중일 때 `홍길동 님이 입력 중...` 표시 |
| FR-N06 | 방 알림 무음 | `/mute` — 특정 채팅방 알림만 무음 처리 |

### 2-8. 유저 커스터마이징 (FR-C)

| ID | 기능 | 설명 |
|----|------|------|
| FR-C01 | 내 메시지 색상 | 내가 보낸 메시지의 텍스트 색상 선택 (red/green/yellow/blue/magenta/cyan/white) |
| FR-C02 | 닉네임 색상 | 채팅창에 표시되는 내 닉네임 색상 선택 |
| FR-C03 | 테마 | dark / light 터미널 테마 선택 |
| FR-C04 | 타임스탬프 형식 | `HH:MM` / `HH:MM:SS` / `MM-DD HH:MM` 중 선택 |
| FR-C05 | 상태메시지 | 친구 목록에 표시되는 한 줄 상태메시지 수정 |
| FR-C06 | 온라인 상태 | `online` / `busy` / `invisible` 수동 설정 |
| FR-C07 | 오픈채팅 닉네임 | 오픈채팅방별 별도 닉네임 설정 |

### 2-9. 마이페이지 (FR-P)

| ID | 기능 | 설명 |
|----|------|------|
| FR-P01 | 프로필 조회 | ID · 닉네임 · 상태메시지 · 가입일 · 마지막 접속 시간 |
| FR-P02 | 활동 통계 | 총 전송 메시지 수 · 참여 중인 채팅방 수 · 친구 수 |
| FR-P03 | 참여 채팅방 목록 | 현재 참여 중인 그룹/오픈채팅방 목록 |
| FR-P04 | 최근 DM 목록 | 최근 대화한 상대 목록 (마지막 메시지, 안읽은 수 포함) |
| FR-P05 | 프로필 수정 | 닉네임 · 상태메시지 인라인 수정 |
| FR-P06 | 비밀번호 변경 | 현재 PW 확인 후 새 PW 설정 |

---

## 3. 시스템 요구사항

### 3-1. 지원 플랫폼

| 구분 | Linux (glibc) | Windows (MinGW-w64) |
|------|:---:|:---:|
| 서버 | ✅ | ✅ |
| 클라이언트 | ✅ | ✅ |
| 컴파일러 | gcc 9+ / clang 10+ | mingw-w64 gcc 8+ |
| C 표준 | C11 | C11 |
| 스레드 | `pthread` | `pthread-win32` |
| 소켓 | `<sys/socket.h>` | `<winsock2.h>` + `ws2_32` |
| GUI | GTK4 (`libgtk-4-dev`) | GTK4 (MSYS2 `mingw-w64-x86_64-gtk4`) |

### 3-2. 최소 사양 (클라이언트)

| 항목 | 최솟값 |
|------|--------|
| CPU | 듀얼코어 1.6GHz |
| RAM | 2GB (OS 포함) — 클라이언트 자체 ≤20MB 상주 |
| 디스플레이 해상도 | 800×600 이상 |
| 네트워크 | 1Mbps |

> 서버는 MySQL 제외 **100MB 이하** 상주 (NFR-05).

### 3-3. 의존성

| 의존성 | 버전 | 용도 |
|--------|------|------|
| MySQL | 5.7+ / 8.x | 영속 저장 |
| libmysqlclient (C API) | MySQL 번들 | 서버측 DB 접근 |
| GTK4 | 3.22+ | 클라이언트 GUI 프레임워크 |
| GLib / GObject | GTK4 번들 | 이벤트 루프, g_idle_add() |
| pthread | OS 번들 | 멀티스레딩 |
| C11 런타임 | - | 공통 |

---

## 4. 빠른 시작

### 4-1. 의존성 설치

**Ubuntu / Debian**
```bash
sudo apt install build-essential libmysqlclient-dev libgtk-4-dev
```

**Windows (MSYS2)**
```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-mysql-connector-c mingw-w64-x86_64-gtk4
```

### 4-2. 데이터베이스 초기화

```bash
# MySQL 서버에 접속하여 스키마 생성
mysql -u root -p < sql/schema.sql
```

> `sql/schema.sql`에 테이블 DDL과 초기 `admin` 계정 시드가 포함되어 있습니다.  
> **⚠ 배포 전 반드시 `admin` 계정의 기본 비밀번호를 변경하세요.**

### 4-3. 빌드

```bash
# 서버 + 클라이언트 전체 빌드
make

# 개별 빌드
make server    # server/chat_server
make client    # client/chat_client

# 디버그 빌드 (AddressSanitizer 포함, Linux)
make DEBUG=1

# 정리
make clean
```

### 4-4. 실행

```bash
# 서버 실행 (기본 포트 8080)
./server/chat_server

# 포트 지정
./server/chat_server 9090

# 클라이언트 실행 (기본: localhost:8080)
./client/chat_client

# 호스트·포트 지정
./client/chat_client 192.168.1.10 9090
```

### 4-5. 접속 예시

GTK4 로그인 창이 열립니다:
- 상단: 앱 제목 "C Chat v2.0" 레이블
- 중단: ID 입력 필드, 비밀번호 입력 필드 (마스킹)
- 하단: [로그인] 버튼, [회원가입] 버튼

---

## 5. 시스템 아키텍처

### 5-1. 전체 구성도

```
┌──────────────────────────────────────────────────────────┐
│                        SERVER                            │
│  ┌────────────┐  ┌──────────────┐  ┌──────────────────┐ │
│  │  Accept    │  │  Router      │  │  Broadcast       │ │
│  │  Thread    │→ │  (per client │→ │  Manager         │ │
│  │            │  │   thread)    │  │  (session array  │ │
│  └────────────┘  └──────────────┘  │   + mutex)       │ │
│                        │           └──────────────────┘ │
│  ┌─────────────────────▼──────────────────────────────┐ │
│  │           In-Memory Session Store                  │ │
│  │  ClientSession[MAX_CLIENTS] — 접속 중인 유저만     │ │
│  └─────────────────────┬──────────────────────────────┘ │
│                        │ SQL (Prepared Statement)        │
│  ┌─────────────────────▼──────────────────────────────┐ │
│  │                MySQL (chat_db)                     │ │
│  │  users | rooms | room_members | messages           │ │
│  │  friends | room_invites | dm_reads | user_settings │ │
│  └────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────┘
               ▲  TCP Socket  ▼
┌──────────────────────────────────────────────────────────┐
│                        CLIENT                            │
│  ┌──────────────────────┐   ┌──────────────────────────┐ │
│  │  GTK Main Thread     │   │  Receive Thread          │ │
│  │  (GtkApplication +   │   │  (socket → g_idle_add    │ │
│  │   event loop)        │   │   → GTK main thread)     │ │
│  └──────────────────────┘   └──────────────────────────┘ │
│  ┌──────────────────────────────────────────────────┐    │
│  │  GTK4 GUI — GtkWindow, GtkStack, GtkTextView     │    │
│  │  Screen: LOGIN | MAIN | CHAT | MYPAGE | SETTINGS  │    │
│  └──────────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────────┘
```

### 5-2. 서버 스레딩 모델

서버는 **Thread-per-client** 모델을 채택합니다. 최대 100 동시 접속(NFR-01)에서 충분히 가벼운 구조입니다.

```
Main Thread (listen + accept)
    │
    ├──pthread_create──▶ Handler Thread #1  ──▶ MySQL Connection #1
    ├──pthread_create──▶ Handler Thread #2  ──▶ MySQL Connection #2
    └──pthread_create──▶ Handler Thread #N  ──▶ MySQL Connection #N
                              │
                    ┌─────────▼──────────┐
                    │  g_sessions[]      │  (공유 상태)
                    │  g_sessions_mutex  │
                    └────────────────────┘
```

**핵심 규칙**
- 핸들러는 `pthread_detach`로 분리 — join하지 않음
- 각 핸들러는 전용 `MYSQL*`을 시작 시 `mysql_init + mysql_real_connect`로 획득, 종료 시 `mysql_close`
- **mutex 잠금 중 `send()` / DB 호출 금지** — 블로킹 시 전체 정지 위험
- 브로드캐스트: mutex 획득 → fd 목록 스냅샷 복사 → unlock → 스냅샷으로 send

### 5-3. 클라이언트 스레드 구조

| 스레드 | 역할 |
|--------|------|
| Main (UI) | GTK 이벤트 루프 실행 (`g_application_run()`), 위젯 생성·업데이트 |
| Recv | 소켓 수신 → `g_idle_add()`로 UI 업데이트 콜백 예약 |

`send()`는 Recv 스레드에서도 호출될 수 있으므로 `tx_mutex`로 보호합니다. GTK 위젯 업데이트는 반드시 `g_idle_add()` 또는 `g_main_context_invoke()`를 통해 GTK 메인 스레드에서만 수행합니다.

### 5-4. 시그널 처리

| 시그널 | 서버 | 클라이언트 |
|--------|------|-----------|
| `SIGPIPE` | `SIG_IGN` | `SIG_IGN` |
| `SIGINT` / `SIGTERM` | 플래그 set → accept 중단 → 모든 세션 close → exit | 소켓 close → GTK 종료 → exit |

---

## 6. 프로젝트 구조

```
C_ChatProgram/
├── chat_program/
│   └── src/
│       ├── server/
│       │   ├── main.c              # 서버 진입점, accept loop
│       │   ├── config.h            # DB 접속 정보, 포트, MAX_CLIENTS 상수
│       │   ├── globals.c / globals.h  # g_sessions[], g_sessions_mutex 선언·정의
│       │   ├── client_handler.c/h  # 클라이언트별 스레드
│       │   ├── router.c / router.h # 패킷 타입별 핸들러 라우팅
│       │   ├── db.c / db.h         # MySQL 연결 래퍼, 쿼리 헬퍼
│       │   ├── auth.c / auth.h     # 회원가입, 로그인
│       │   ├── user_store.c/h      # 유저·설정 CRUD
│       │   ├── friend.c / friend.h # 친구 요청/수락/차단
│       │   ├── room.c / room.h     # 채팅방 생성/참여/관리
│       │   ├── dm.c / dm.h         # 1:1 DM 처리
│       │   ├── message.c / .h      # 메시지 저장·삭제·수정·검색
│       │   ├── broadcast.c / .h    # 룸 브로드캐스트, 알림 전송
│       │   └── admin.c / admin.h   # 관리자 명령 처리 (Out-of-Scope)
│       │
│       ├── client/
│       │   ├── main.c              # GTK 앱 진입점, CSS 로드, app_window 생성
│       │   ├── state.c / state.h   # 전역 클라이언트 상태 (소켓, 현재 화면, 설정)
│       │   ├── net.c / net.h       # 소켓 연결, recv 스레드, send 함수
│       │   ├── packet.c / packet.h # packet_build / packet_parse 직렬화
│       │   ├── app_window.c/h      # GtkApplicationWindow, GtkStack 화면 전환
│       │   ├── notify.c / notify.h # 알림 GtkRevealer 큐, 표시 처리
│       │   ├── screen_login.c/h    # 로그인·회원가입 화면
│       │   ├── screen_main.c/h     # 메인(친구·채팅 목록) 화면
│       │   ├── screen_chat.c/h     # 채팅방 화면
│       │   ├── screen_mypage.c/h   # 마이페이지 화면
│       │   ├── screen_settings.c/h # 설정 화면
│       │   └── css/
│       │       ├── theme-dark.css
│       │       ├── theme-light.css
│       │       ├── components.css
│       │       ├── chat.css
│       │       └── login.css
│       │
│       └── common/
│           ├── protocol.h          # 패킷 타입 상수, 구분자, 응답 코드
│           ├── types.h             # 공통 구조체 (UserSettings 등)
│           ├── utils.c / utils.h   # 타임스탬프, 이모티콘 변환, 문자열 유틸
│           ├── net_compat.h        # 플랫폼 소켓 호환 인터페이스
│           ├── net_posix.c         # Linux 소켓 구현
│           └── net_win.c           # Windows(Winsock2) 소켓 구현
│
├── sql/
│   └── schema.sql              # DB 생성·테이블 DDL, 시드 데이터
│
├── docs/                       # 설계 문서 (10개 섹션)
├── Makefile                    # Linux · Windows(MinGW-w64) 통합 빌드
├── requirements.md             # 요구사항 명세 v2.0.0
└── README.md                   # 이 문서
```

### 6-1. 레이어 역할

| 레이어 | 디렉터리 | 책임 | 의존 |
|--------|----------|------|------|
| 공통 | `common/` | 패킷 타입 상수 · 공용 구조체 · 유틸리티 · 플랫폼 소켓 호환 | 표준 C, 플랫폼 네트워크 헤더 |
| 서버 | `server/` | TCP listen · 멀티스레드 핸들러 · 기능 처리 · MySQL 접근 | `common/`, pthread, libmysqlclient |
| 클라이언트 | `client/` | GTK4 GUI · send/recv 스레드 · 패킷 처리 | `common/`, pthread, libgtk-4 |
| DB 스키마 | `sql/` | 초기화 SQL | (없음) |

---

## 7. 패킷 프로토콜

### 7-1. 기본 형식

```
<TYPE>|<PAYLOAD>\n
```

| 구성 요소 | 값 | 설명 |
|-----------|-----|------|
| 필드 구분자 | `\|` | TYPE과 PAYLOAD, PAYLOAD 내 필드 간 구분 |
| 다중값 구분자 | `:` | 단일 필드 내 복수 값 |
| 리스트 구분자 | `;` | 목록 항목 간 구분 |
| 종단 문자 | `\n` | 패킷 끝 |
| 최대 길이 | **2048 bytes** | 엄격 준수 |

### 7-2. 인증 패킷

```
# 로그인
C→S  LOGIN_REQ|<id>:<pass>
S→C  LOGIN_RES|<code>:<message>
     # code: 0=OK, 1=WRONG_ID, 2=WRONG_PW, 3=ALREADY_ONLINE

# 회원가입
C→S  REGISTER_REQ|<id>:<pass>:<nick>:<status>
S→C  REGISTER_RES|<code>
     # code: 0=OK, 1=DUPLICATE_ID

# 로그아웃
C→S  LOGOUT_REQ|
S→C  LOGOUT_RES|OK
```

### 7-3. 친구 패킷

```
C→S  FRIEND_ADD_REQ|<target_id>
S→C  FRIEND_ADD_RES|<code>        # 0=SENT, 1=NOT_FOUND, 2=BLOCKED, 3=ALREADY_FRIEND
S→C  FRIEND_REQUEST_NOTIFY|<from_id>:<from_nick>

C→S  FRIEND_ACCEPT|<from_id>
C→S  FRIEND_REJECT|<from_id>
C→S  FRIEND_DELETE|<target_id>
C→S  FRIEND_BLOCK|<target_id>

C→S  FRIEND_LIST_REQ|
S→C  FRIEND_LIST_RES|<id>:<nick>:<status>:<status_msg>;<id>:...

S→C  FRIEND_STATUS_CHANGE|<id>:<nick>:<status>   # 친구 접속/해제 실시간 알림

C→S  USER_SEARCH|<keyword>
S→C  USER_SEARCH_RES|<id>:<nick>:<status_msg>;<id>:...
```

### 7-4. DM 패킷

```
C→S  DM_SEND|<to_id>:<content>
S→C  DM_RECV|<from_id>:<from_nick>:<content>:<timestamp>:<msg_id>
S→C  DM_READ_NOTIFY|<from_id>
C→S  DM_HISTORY_REQ|<with_id>:<count>
S→C  DM_HISTORY_RES|<msg_id>:<from_id>:<content>:<timestamp>:<read>|...
```

### 7-5. 채팅방 패킷

```
C→S  ROOM_CREATE|<name>:<max_users>:<is_open>:<password>:<topic>
S→C  ROOM_CREATE_RES|<code>:<room_id>

C→S  ROOM_LIST_REQ|<type>               # type: group | open
S→C  ROOM_LIST_RES|<id>:<name>:<cur>:<max>:<topic>:<has_pw>;<id>:...

C→S  ROOM_JOIN|<room_id>:<password>
S→C  ROOM_JOIN_RES|<code>:<room_id>:<room_name>

C→S  ROOM_MSG|<room_id>:<content>
S→C  ROOM_MSG_RECV|<room_id>:<from_nick>:<content>:<timestamp>:<msg_id>:<reply_to_id>:<reply_preview>

C→S  ROOM_LEAVE|<room_id>
C→S  ROOM_INVITE|<room_id>:<target_id>
C→S  ROOM_KICK|<room_id>:<target_id>    # 방장/관리자 전용
C→S  ROOM_SET_NOTICE|<room_id>:<notice> # 방장/관리자 전용
C→S  ROOM_GRANT_ADMIN|<room_id>:<target_id>   # 방장 전용
C→S  ROOM_REVOKE_ADMIN|<room_id>:<target_id>  # 방장 전용
C→S  ROOM_MEMBERS_REQ|<room_id>
S→C  ROOM_MEMBERS_RES|<room_id>|<id>:<nick>:<is_admin>:<online>;<id>:...
```

### 7-6. 메시지 부가기능 패킷

```
C→S  WHISPER|<to_nick>:<content>
S→C  WHISPER_RECV|<from_nick>:<content>:<timestamp>

C→S  MSG_DELETE|<room_id>:<msg_id>
S→C  MSG_DELETED_NOTIFY|<room_id>:<msg_id>

C→S  MSG_EDIT|<room_id>:<msg_id>:<new_content>
S→C  MSG_EDITED_NOTIFY|<room_id>:<msg_id>:<new_content>

C→S  MSG_REPLY|<room_id>:<reply_to_id>:<content>

C→S  MSG_REACT|<room_id>:<msg_id>:<emoji>    # 이미 반응했으면 토글(취소)
S→C  MSG_REACT_NOTIFY|<room_id>:<msg_id>:<emoji>:<count>:<user_list>

C→S  MSG_SEARCH|<room_id>:<keyword>
S→C  MSG_SEARCH_RES|<msg_id>:<from_nick>:<content>:<timestamp>|...

C→S  MSG_PIN|<room_id>:<msg_id>              # 방장/관리자 전용
S→C  MSG_PIN_NOTIFY|<room_id>:<msg_id>:<from_nick>:<content_preview>
```

### 7-7. 알림 및 기타 패킷

```
# 알림 (서버 → 클라이언트)
S→C  NOTIFY|<type>:<content>
     # type: MENTION | FRIEND_REQ | SERVER | DM | REACTION

# 타이핑 인디케이터
C→S  TYPING_START|<room_id>
C→S  TYPING_STOP|<room_id>
S→C  TYPING_NOTIFY|<room_id>:<nick>:<is_typing>   # 0 or 1

# 관리자
C→S  ADMIN_CMD|<cmd>:<args>
     # cmd: broadcast, kick_user, server_stat, user_list, delete_room
S→C  ADMIN_RES|<code>:<data>

# 연결 유지
C→S  PING|
S→C  PONG|
```

---

## 8. 데이터베이스

### 8-1. ER 다이어그램

```
users ──────────────────────────── user_settings (1:1)
  │
  ├── friends (user_id / friend_id)
  ├── rooms (owner_id)
  ├── room_members ────── rooms
  ├── room_invites ────── rooms   ← 오프라인 초대 보관
  │    ├── inviter_id → users
  │    └── invitee_id → users
  └── messages (from_id) ─────── rooms (group/open)
       ├── dm_reads ─────── users (reader_id)
       └── messages (reply_to, self-reference)
```

### 8-2. 테이블 스키마

**users** — 유저 기본 정보

```sql
CREATE TABLE users (
    id            VARCHAR(20)  PRIMARY KEY,
    password_hash VARCHAR(64)  NOT NULL,        -- SHA2(pass, 256)
    nickname      VARCHAR(20)  NOT NULL UNIQUE,  -- 닉네임 중복 불허 (귓속말 대상 식별)
    status_msg    VARCHAR(100) DEFAULT '',
    online_status TINYINT      DEFAULT 0,        -- 0=offline, 1=online, 2=busy
    is_admin      TINYINT      DEFAULT 0,
    last_seen     DATETIME     DEFAULT NULL,
    created_at    DATETIME     DEFAULT CURRENT_TIMESTAMP
);
```

**user_settings** — 커스터마이징 설정

```sql
CREATE TABLE user_settings (
    user_id    VARCHAR(20) PRIMARY KEY,
    msg_color  VARCHAR(15) DEFAULT 'cyan',
    nick_color VARCHAR(15) DEFAULT 'yellow',
    theme      VARCHAR(10) DEFAULT 'dark',
    ts_format  TINYINT     DEFAULT 0,           -- 0=HH:MM, 1=HH:MM:SS, 2=MM-DD HH:MM
    dnd        TINYINT     DEFAULT 0,           -- 1=방해금지 모드
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);
```

**friends** — 친구 관계

```sql
CREATE TABLE friends (
    id        INT         AUTO_INCREMENT PRIMARY KEY,
    user_id   VARCHAR(20) NOT NULL,
    friend_id VARCHAR(20) NOT NULL,
    status    TINYINT     DEFAULT 0,           -- 0=pending, 1=accepted, 2=blocked
    created_at DATETIME   DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uniq_pair (user_id, friend_id),
    FOREIGN KEY (user_id)   REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (friend_id) REFERENCES users(id) ON DELETE CASCADE
);
```

> 양방향 친구 관계는 accepted 시 `(A,B)`, `(B,A)` 두 레코드로 표현합니다.

**rooms** — 채팅방

```sql
CREATE TABLE rooms (
    id            INT          AUTO_INCREMENT PRIMARY KEY,
    name          VARCHAR(30)  NOT NULL,
    topic         VARCHAR(100) DEFAULT '',
    password_hash VARCHAR(64)  DEFAULT '',     -- 빈 문자열 = 공개
    max_users     INT          DEFAULT 30,
    owner_id      VARCHAR(20)  NOT NULL,
    notice        VARCHAR(255) DEFAULT '',
    is_open       TINYINT      DEFAULT 0,      -- 0=그룹(초대), 1=오픈채팅
    pinned_msg_id INT          DEFAULT NULL,   -- 앱 레벨 정합성 관리
    created_at    DATETIME     DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (owner_id) REFERENCES users(id)
);
```

**room_members** — 채팅방 멤버십

```sql
CREATE TABLE room_members (
    room_id   INT         NOT NULL,
    user_id   VARCHAR(20) NOT NULL,
    open_nick VARCHAR(20) DEFAULT '',          -- 오픈채팅 전용 닉네임
    is_admin  TINYINT     DEFAULT 0,
    is_muted  TINYINT     DEFAULT 0,           -- 방 알림 무음
    joined_at DATETIME    DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (room_id, user_id),
    FOREIGN KEY (room_id) REFERENCES rooms(id)   ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(id)   ON DELETE CASCADE
);
```

**messages** — 메시지

```sql
CREATE TABLE messages (
    id         INT         AUTO_INCREMENT PRIMARY KEY,
    room_id    INT         DEFAULT NULL,       -- NULL이면 DM
    from_id    VARCHAR(20) NOT NULL,
    to_id      VARCHAR(20) DEFAULT NULL,       -- DM 수신자
    content    VARCHAR(500) NOT NULL,
    reply_to   INT         DEFAULT NULL,       -- 답장 원본 msg_id
    msg_type   TINYINT     DEFAULT 0,          -- 0=normal, 1=system, 2=whisper, 3=me-action
    is_deleted TINYINT     DEFAULT 0,
    created_at DATETIME    DEFAULT CURRENT_TIMESTAMP,
    edited_at  DATETIME    DEFAULT NULL,
    FOREIGN KEY (from_id)  REFERENCES users(id),
    FOREIGN KEY (reply_to) REFERENCES messages(id) ON DELETE SET NULL
);
```

**dm_reads** — DM 읽음 상태

```sql
CREATE TABLE dm_reads (
    msg_id    INT         NOT NULL,
    reader_id VARCHAR(20) NOT NULL,
    read_at   DATETIME    DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (msg_id, reader_id),
    FOREIGN KEY (msg_id)    REFERENCES messages(id) ON DELETE CASCADE,
    FOREIGN KEY (reader_id) REFERENCES users(id)    ON DELETE CASCADE
);
```

**room_invites** — 채팅방 초대 (오프라인 사용자 초대 보관)

```sql
CREATE TABLE room_invites (
    id         INT          AUTO_INCREMENT PRIMARY KEY,
    room_id    INT          NOT NULL,
    inviter_id VARCHAR(20)  NOT NULL,
    invitee_id VARCHAR(20)  NOT NULL,
    status     TINYINT      NOT NULL DEFAULT 0,         -- 0=pending, 1=수락, 2=거절
    created_at DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uq_invite (room_id, invitee_id),
    INDEX      idx_invitee (invitee_id),
    FOREIGN KEY (room_id)    REFERENCES rooms(id)   ON DELETE CASCADE,
    FOREIGN KEY (inviter_id) REFERENCES users(id)   ON DELETE CASCADE,
    FOREIGN KEY (invitee_id) REFERENCES users(id)   ON DELETE CASCADE
);
```

> 로그인 시 `status=0` 인 미처리 초대를 조회하여 `ROOM_INVITE_NOTIFY` 로 재전송.

**reactions** — *(Out-of-Scope: FR-M05)* v2.1 이후 별도 마이그레이션으로 추가 예정.

### 8-3. 서버 인메모리 세션 구조체

```c
/* 접속 중인 클라이언트 세션 (DB에 저장하지 않음) */
typedef struct {
    int       fd;
    char      user_id[21];
    char      nickname[21];
    int       online_status;    /* 0=offline, 1=online, 2=busy */
    int       dnd;
    int       current_room_id;  /* 0 = 채팅방 밖 */
    int       muted_rooms[32];  /* 알림 무음 room_id 목록 */
    int       muted_count;
    int       is_admin;
    int       active;           /* 1 = 유효한 세션 */
    MYSQL    *db;               /* 스레드 전용 MySQL 연결 */
    pthread_t tid;
} ClientSession;

extern ClientSession  g_sessions[MAX_CLIENTS]; /* 전역, mutex 보호 */
extern pthread_mutex_t g_sessions_mutex;
```

---

## 9. GUI 화면 구성

### 9-1. 로그인 화면 (LoginWindow)

GTK4 `GtkWindow` 기반 로그인 창:
- **레이아웃**: `GtkBox` (vertical) — 타이틀 라벨, ID 입력란(`GtkEntry`), 비밀번호 입력란(`GtkEntry`, visibility=FALSE), [로그인] 버튼, [회원가입] 버튼
- **인터랙션**: [로그인] 버튼 클릭 또는 Enter → `LOGIN_REQ` 패킷 전송 → 응답에 따라 메인 창으로 전환 또는 오류 메시지 표시(`GtkRevealer`)
- **회원가입**: 별도 `GtkWindow` (모달) 또는 같은 창 내 `GtkStack` 전환으로 ID·비밀번호·닉네임·상태메시지 입력

### 9-2. 메인 화면 (MainWindow)

`GtkApplicationWindow` 내 `GtkPaned` (좌측 패널 + 우측 콘텐츠):
- **좌측 패널** (`GtkNotebook` 또는 `GtkStackSidebar`):
  - 친구 목록 탭: `GtkListBox` — 각 행에 닉네임, 상태 아이콘(●), 상태메시지 표시
  - 채팅 목록 탭: `GtkListBox` — 방 이름, 마지막 메시지 미리보기, 미읽 배지
  - 오픈채팅 탭: `GtkListBox` — 오픈채팅방 목록 + 검색 `GtkEntry`
  - 마이페이지 탭: 프로필 정보 및 통계
- **우측 상단**: 친구 추가(`GtkButton`), 방 만들기(`GtkButton`), 내 프로필 버튼
- **알림 배너**: `GtkRevealer` 또는 커스텀 오버레이 위젯 (친구 요청, 멘션)

### 9-3. 채팅 화면 (ChatWindow or GtkStack 내 페이지)

- **상단 헤더바** (`GtkHeaderBar`): 방 이름, 인원 수, 공지사항 버튼, 멤버 목록 버튼, 검색 버튼
- **메시지 영역** (`GtkScrolledWindow` + `GtkTextView` 또는 `GtkListBox`):
  - 각 메시지: 닉네임, 내용, 타임스탬프, 읽음 수 표시
  - 내 메시지는 우측 정렬, 상대 메시지는 좌측 정렬
  - 답장 메시지: 인용 블록(`GtkFrame` 또는 `GtkBox` 강조) 포함
  - 시스템 메시지: 중앙 정렬, 회색 텍스트
  - 타이핑 표시: 하단에 "홍길동 님이 입력 중..." 레이블 표시/숨김
- **입력 영역** (`GtkBox` horizontal): `GtkEntry` (메시지 입력) + [전송] `GtkButton`
  - Enter 또는 [전송] 클릭 → `ROOM_MSG` 패킷 전송
- **컨텍스트 메뉴** (우클릭 또는 더보기 버튼): 답장, 수정, 삭제, 귓속말, 검색 메뉴 항목

### 9-4. 마이페이지 화면

`GtkGrid` 기반 프로필 정보 표시:
- 프로필 영역: ID, 닉네임, 상태메시지, 가입일, 마지막 접속 시간
- 통계: 총 메시지 수, 참여 방 수, 친구 수 (레이블로 표시)
- [프로필 수정] 버튼 → `GtkWindow` (모달 팝업, 닉네임·상태메시지 수정)
- [비밀번호 변경] 버튼 → `GtkWindow` (모달 팝업)

### 9-5. 설정 화면

`GtkListBox` 기반 설정 항목:
- 테마: `GtkDropDown` (dark / light)
- 타임스탬프 형식: `GtkDropDown` (HH:MM / HH:MM:SS / MM-DD HH:MM)
- 온라인 상태: `GtkDropDown` (online / busy / invisible)
- 각 설정 변경 시 즉시 `PROFILE_UPDATE` 패킷 전송 또는 로컬 저장

---

## 10. 메뉴 및 버튼 구성

GUI 기반으로 슬래시 커맨드 대신 버튼, 메뉴, 다이얼로그를 통해 모든 기능을 수행합니다.

### 10-1. 채팅방 내 액션

| 기능 | UI 요소 | 설명 |
|------|---------|------|
| 메시지 전송 | `GtkEntry` + [전송] 버튼 | Enter 또는 버튼 클릭 |
| 귓속말 | 메시지 우클릭 → "귓속말" 메뉴 | 대상 선택 후 내용 입력 다이얼로그 |
| 메시지 삭제 | 메시지 우클릭 → "삭제" 메뉴 | 본인 메시지만, 확인 다이얼로그 표시 |
| 메시지 수정 | 메시지 우클릭 → "수정" 메뉴 | 전송 후 5분 이내, 인라인 편집 |
| 답장 | 메시지 우클릭 → "답장" 메뉴 | 인용 표시 후 입력창 포커스 |
| 메시지 검색 | 헤더바 검색 버튼 클릭 | `GtkSearchBar` 슬라이드 다운 |
| 멤버 목록 | 헤더바 멤버 버튼 클릭 | `GtkPopover` 또는 사이드 패널 |
| 공지 등록 | 헤더바 더보기 버튼 → "공지 등록" | 방장만 활성화, 입력 다이얼로그 |
| 방 나가기 | 헤더바 더보기 버튼 → "나가기" | 확인 다이얼로그 표시 |
| 멘션(@) | 입력창에 `@` 입력 → 자동완성 팝업 | `GtkPopover`로 멤버 목록 표시 |
| 타이핑 표시 | 자동 (입력창 포커스/입력 시 전송) | 별도 사용자 조작 불필요 |

### 10-2. 메인 화면 액션

| 기능 | UI 요소 | 설명 |
|------|---------|------|
| 친구 추가 | 친구 탭 상단 [+] 버튼 | ID 입력 다이얼로그 |
| 친구 요청 수락/거절 | 알림 배너 또는 친구 탭 요청 목록 | 수락/거절 버튼 |
| 친구 삭제/차단 | 친구 항목 우클릭 → 컨텍스트 메뉴 | 확인 다이얼로그 표시 |
| DM 시작 | 친구 항목 더블클릭 또는 "메시지" 버튼 | 채팅 화면으로 전환 |
| 그룹 채팅 생성 | 채팅 탭 상단 [+] 버튼 | 방 이름·주제·최대인원 입력 다이얼로그 |
| 오픈채팅 참여 | 오픈채팅 탭 → 방 선택 → [참여] 버튼 | 비밀번호 방이면 입력 다이얼로그 |
| 유저 검색 | 친구 탭 상단 검색 `GtkEntry` | 실시간 필터링 |

### 10-3. 전역 동작

| 기능 | UI 요소 | 설명 |
|------|---------|------|
| 온라인 상태 변경 | 하단 상태바 또는 마이페이지 `GtkDropDown` | online / busy / invisible |
| 마이페이지 | 좌측 패널 프로필 아이콘 클릭 | 마이페이지 탭으로 전환 |
| 설정 | 헤더바 설정(⚙) 버튼 | 설정 탭 또는 다이얼로그 |
| 로그아웃 | 헤더바 메뉴 → "로그아웃" | `LOGOUT_REQ` 전송 후 로그인 창으로 전환 |

---

## 11. 보안

### 11-1. 비밀번호 저장

- 알고리즘: **SHA-256** (`MySQL SHA2(pass, 256)`)
- 저장 컬럼: `users.password_hash VARCHAR(64)`
- 평문은 어디에도 저장되지 않으며 로그에도 기록하지 않음 (`***` 마스킹)
- v2.0 한계: salt 없음 (v2.1에서 per-user salt 도입 예정)

### 11-2. SQL Injection 방지

- **모든** DB 접근은 MySQL Prepared Statement 사용 — 직접 문자열 연결 금지

### 11-3. 권한 검사

- 모든 서버 핸들러는 페이로드의 `from_id`를 신뢰하지 않고 **세션의 `user_id`** 기준으로만 처리
- 방장/관리자 전용 기능은 세션 + DB 기준으로 권한 재검사

### 11-4. 알려진 제한사항 (공식 명시)

| 항목 | 현황 | 개선 계획 |
|------|------|-----------|
| 전송 암호화 | 평문 TCP (로컬/신뢰망 전용) | v2.1에서 TLS 도입 |
| 비밀번호 salt | 없음 (rainbow table 취약) | v2.1에서 per-user salt |
| IP 속도 제한 | 미구현 | P4 후보 |
| 로그인 시도 제한 | 5회 실패 시 소켓 close | - |

### 11-5. 배포 체크리스트

- [ ] 시드 `admin` 계정 비밀번호를 강한 것으로 즉시 변경
- [ ] MySQL root 계정 대신 전용 사용자(`chat_app@localhost`, 필요 권한만) 사용
- [ ] 방화벽에서 서버 포트(기본 8080) 외부 노출 제한
- [ ] `sql/schema.sql` 외 개발 덤프 유출 주의

---

## 12. 비기능 요구사항

| ID | 항목 | 목표 | 구현 전략 |
|----|------|------|-----------|
| NFR-01 | 동시 접속 | ≤100 | `MAX_CLIENTS=128` 정적 세션 배열, accept 초과 시 `SERVER_FULL` 응답 |
| NFR-02 | 응답 지연 | ≤50ms (로컬) | 브로드캐스트 → DB 저장 순서 유지, 논블로킹 `send` |
| NFR-03 | 안정성 | 크래시 없음 | `SIGPIPE` 무시, recv 0/에러 시 세션만 정리, 스레드 detach |
| NFR-04 | 보안 | SHA-256 해시 | MySQL `SHA2()`, 평문 전송은 로컬 환경 한정 |
| NFR-05 | 경량성 | 서버 ≤100MB | 정적 세션 배열, 메시지 히스토리는 DB에 위임 |
| NFR-06 | 이식성 | Linux · Windows | 플랫폼 분기를 `net_compat.h`에 집중, GUI는 GTK4로 통일 |
| NFR-07 | GTK4 GUI | GTK4 GUI | `net` 계층과 GTK4 `ui` 계층 분리, 수신 스레드는 `g_idle_add()`로만 UI 접근 |
| NFR-08 | 영속성 | 재시작 후 데이터 유지 | 세션 외 모든 상태를 MySQL에 영속 저장 |
| NFR-09 | 스레드 안전 | mutex 보호 | 세션 배열 `g_sessions_mutex`, DB 연결 스레드 전용 |

---

## 13. 개발 로드맵

### P0 — 최소 동작 (MVP)

> **완료 기준**: 서버 실행 → 2 클라이언트 동시 접속 → 같은 방에서 메시지 교환

- 빌드 시스템 (Makefile, Linux/Windows), DB 스키마 (`sql/schema.sql`)
- `common/` 레이어: `protocol.h`, `types.h`, `utils.c`, `net_compat.h`
- 서버 accept + thread-per-client 뼈대, router, broadcast
- 클라이언트 GTK4 기본 창 (`GtkApplicationWindow` + `GtkStack`), recv 스레드, `packet.c`
- DB 연결 (`db_connect`), Prepared Statement 헬퍼
- **FR-A01, A02, A03** — 회원가입·로그인·로그아웃
- **FR-G01, G03, G05, G09** — 그룹방 생성·메시지 전송·퇴장·히스토리
- **FR-O01, O02, O04** — 오픈채팅방 생성·목록·참여
- `PING/PONG` 연결 유지

### P1 — 소셜 기반

- **계정**: FR-A04~A06 (프로필 수정·비밀번호 변경·마지막 접속 표시)
- **친구**: FR-F01~F07 (추가·수락/거절·목록·삭제·차단·상태 표시·검색)
- **DM**: FR-D01~D05 (시작·전송·읽음 확인·히스토리·미읽 배지)
- **마이페이지**: FR-P01~P06 (프로필·통계·채팅방 목록·DM 목록·프로필 수정·비밀번호 변경)

### P2 — 완성도

- **메시지 편집**: FR-M01, M02, M03, M07, M09 (귓속말·삭제·수정·시스템 메시지·타임스탬프)
- **방 관리**: FR-G02, G06~G08, G10 (초대·방장권한·공동방장·공지·멤버목록)
- **커스터마이징**: FR-C01~C07 (메시지색·닉네임색·테마·타임스탬프·상태메시지·온라인상태·오픈닉)
- **알림**: FR-N01, N02 (메시지 배너·친구 요청 알림)

### P3 — 고급 기능

- **메시지**: FR-M04, M06, M08, M10, M11 (답장(인용)·이모티콘 변환·검색·핀·me 액션)
- **오픈채팅**: FR-O03, O05 (방 검색·익명 닉네임)
- **알림 고급**: FR-N03~N06 (멘션 강조·DND 모드·타이핑 표시·방 무음)

> **Out-of-Scope**: FR-M05(리액션) · FR-ADM01~ADM05(관리자) — v2.1 이후 별도 구현

### P4 — v2.1 후보 (Out-of-scope)

- BAN 테이블, rate-limit, FULLTEXT 검색
- 세션별 Prepared Statement 캐시
- DM 다중행 메시지
- TLS 암호화 전송
- 자동 재접속 시 인증 복원 (토큰)

---

## 14. 설계 문서

`docs/` 디렉터리에 10개 섹션으로 구성된 상세 설계 문서가 있습니다.

| # | 섹션 | 내용 |
|---|------|------|
| 01 | [overview](./docs/01_overview/) | 프로젝트 목적, 타깃 플랫폼, NFR, 용어집 |
| 02 | [features](./docs/02_features/) | 기능 요구사항(FR) 상세 |
| 03 | [architecture](./docs/03_architecture/) | 시스템 구성, 스레딩, 데이터 흐름, 빌드 이식성 |
| 04 | [file_structure](./docs/04_file_structure/) | 소스 트리, 모듈 책임, 네이밍 규약 |
| 05 | [security](./docs/05_security/) | 인증, 검증, 권한, 위협 모델 (STRIDE) |
| 06 | [ui_ux](./docs/06_ui_ux/) | 화면·명령어·테마·렌더링 전략 |
| 07 | [database](./docs/07_database/) | 스키마, 쿼리 카탈로그, 연결 관리 |
| 08 | [api](./docs/08_api/) | 패킷 프로토콜 전체 명세, 시퀀스 다이어그램 |
| 09 | [exception](./docs/09_exception/) | 예외 분류·처리·엣지 케이스·로깅 |
| 10 | [todo_list](./docs/10_todo_list/) | 단계별 로드맵, 수용 기준, 리스크 |

**읽는 순서 추천**
- 처음 보는 사람: `01 → 02 → 03 → 06 → 07 → 08 → 05 → 09 → 04 → 10`
- 구현자: `04 → 08 → 07 → 03 → 09`
- 리뷰어: `01 → 10 → 02 → 05 → 09`

---

> **요구사항 기준**: [`requirements.md`](./requirements.md) v2.0.0  
> **설계 문서 기준**: [`docs/`](./docs/) v2.0.0-design.1
