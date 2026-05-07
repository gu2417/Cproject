# Git Commit Message Rules

## 형식 — Conventional Commits (한국어 적용)

```
<type>(<scope>): <subject>

<body — 선택, why 중심>

<footer — 선택>
```

### 1. 헤더 (1줄, 50자 이내, 마침표 없음)

| `<type>` | 의미 | 예시 |
|----------|------|------|
| `feat` | 새 기능 | `feat(auth): SHA-256 회원가입 핸들러 추가` |
| `fix` | 버그 수정 | `fix(file_io): 5슬래시 버그 정정` |
| `refactor` | 구조 변경 | `refactor(router): 디스패치 테이블 도입` |
| `docs` | 문서만 변경 | `docs(packet): MYPAGE_RES 필드 순서 수정` |
| `test` | 테스트 | `test(utils): SHA-256 known vector 검증` |
| `chore` | 빌드·잡무 | `chore(make): room_reads.txt 추가` |
| `build` | 빌드 시스템 | `build(mingw): -ladvapi32 추가` |
| `perf` | 성능 | `perf(broadcast): 멤버 hash set 캐시` |
| `style` | 스타일 | `style: 들여쓰기 4 spaces 통일` |
| `ci` | CI 설정 | `ci: Windows runner 추가` |

### 2. `<scope>` (선택)
모듈명: `auth`, `room`, `dm`, `friend`, `message`, `broadcast`, `router`, `file_io`, `user_store`, `client`, `protocol`, `utils`
카테고리: `docs`, `build`, `make`, `security`, `phase0`, `phase1`, `phase2`, `phase3`

### 3. `<subject>` (한국어, 명사형, 50자 이내, 마침표 없음)

### 4. 본문 — why 중심, 72자 줄바꿈

### 5. 푸터 — 선택
| 키 | 용도 |
|----|------|
| `Refs #<n>` | 관련 이슈 |
| `Closes #<n>` | 이슈 종료 |
| `BREAKING CHANGE: <설명>` | 패킷 형식·DB 스키마 변경 |

### 6. 금지
- ❌ "WIP", "수정", "업데이트" 같은 무의미한 헤더
- ❌ `--no-verify` 우회
- ❌ 한 커밋에 두 가지 이상 type 혼합

### 7. 자동 검증 정규식
`^(feat|fix|refactor|docs|test|chore|build|perf|style|ci)(\([a-z0-9_-]+\))?: .{1,50}$`
