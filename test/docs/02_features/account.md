# 계정 관리 (FR-A01 ~ A06)

## FR-A01 회원가입 · P0

| 항목 | 내용 |
|------|------|
| 입력 | id(≤20), password(≤30), nickname(≤20), status_msg(≤100) |
| 패킷 | `REGISTER_REQ` / `REGISTER_RES` |
| 테이블 | `users`, `user_settings`(기본값 자동 생성) |
| 로직 | ① 입력 길이·문자셋 검증 → ② id 중복 검사(`SELECT 1 FROM users WHERE id=?`) → ③ `INSERT users` + `SHA2(?,256)` → ④ `INSERT user_settings (user_id)` |
| 응답 코드 | 0=OK, 1=DUPLICATE_ID, 기타=검증 실패 |
| 예외 | 동시 가입 race → UNIQUE 제약으로 DB 가 거부, `DUPLICATE_ID` 로 매핑 |

## FR-A02 로그인 · P0

| 항목 | 내용 |
|------|------|
| 입력 | id, password |
| 패킷 | `LOGIN_REQ` / `LOGIN_RES` |
| 로직 | ① `SELECT password_hash FROM users WHERE id=?` → ② `hash == SHA2(input,256)` 비교 → ③ 세션 배열에서 동일 id 가 이미 `active` 이면 `ALREADY_ONLINE` 반환 → ④ 세션 등록, `online_status=1`, `last_seen=NOW()` 업데이트 |
| 응답 코드 | 0=OK, 1=WRONG_ID, 2=WRONG_PW, 3=ALREADY_ONLINE |
| 예외 | 세션 배열 full → `SERVER_FULL`(09장 참조) |

## FR-A03 로그아웃 · P0

| 로직 | ① `online_status=0`, `last_seen=NOW()` UPDATE → ② 세션 배열에서 제거(mutex) → ③ 친구들에게 `FRIEND_STATUS_CHANGE` 브로드캐스트 → ④ 소켓 close |
| 패킷 | `LOGOUT_REQ` / `LOGOUT_RES` |

## FR-A04 프로필 수정 · P1

| 패킷 | `PROFILE_UPDATE` / `PROFILE_UPDATE_RES` |
| 로직 | `UPDATE users SET nickname=?, status_msg=? WHERE id=?` |
| 예외 | 닉네임 길이 초과 → `code=1`(INVALID) |

## FR-A05 비밀번호 변경 · P1

| 패킷 | `PASS_CHANGE` / `PASS_CHANGE_RES` |
| 로직 | 현재 pw 해시 비교 → 일치 시 `UPDATE users SET password_hash=SHA2(?,256)` |
| 예외 | 현재 pw 불일치 → `code=1`(WRONG_OLD_PW) |

## FR-A06 마지막 접속 시간 · P1

| 출력 | 오프라인 친구 프로필에 `마지막 접속: N분 전` 표시 |
| 계산 | 클라이언트가 `last_seen` 과 현재 시각 차이를 표시. `<1분`→"방금 전", `<60분`→"N분 전", `<24시간`→"N시간 전", 그 외→`YYYY-MM-DD` |

## 관련 문서

- 패킷: [`08_api/packets/auth.md`](../08_api/packets/auth.md)
- 테이블: [`07_database/tables/users.md`](../07_database/tables/users.md), [`user_settings.md`](../07_database/tables/user_settings.md)
- 보안: [`05_security/auth_and_password.md`](../05_security/auth_and_password.md)
