# 인증 · 비밀번호

## 1. 저장 형식

- 컬럼: `users.password_hash VARCHAR(64)`.
- 알고리즘: **SHA-256** (MySQL `SHA2(plain, 256)` 사용). salt 없음(v2.0 한계).
- 가입 시:
  ```sql
  INSERT INTO users (id, password_hash, ...) VALUES (?, SHA2(?,256), ...);
  ```
- 로그인 시:
  ```sql
  SELECT id, password_hash FROM users WHERE id=?;
  ```
  서버가 `SHA2(input,256)` 를 MySQL 에 다시 요청하거나, 서버 측 SHA-256 구현으로 해시해 비교.
  → **권장**: 서버 측 직접 해시(불필요한 왕복 제거). libcrypto 혹은 mini 구현.

## 2. 전송

- 본 버전은 **평문 전송**(NFR-04). 사용은 로컬/폐쇄망으로 제한.
- 로그인 패킷에서 비밀번호 필드는 로그에 남기지 않는다 — 로그 출력 시 `***` 마스킹.

## 3. 중복 로그인

- 로그인 시 `g_sessions[]` 스캔하여 동일 `user_id` 가 `active=1` 이면 `LOGIN_RES|3`(ALREADY_ONLINE).
- 정책 대안(향후): 기존 세션 강제 종료 후 신규 허용. 현재는 **거부** 방식.

## 4. 로그인 실패 대응

- 실패 5회 시 해당 소켓 close.
- IP 기반 속도 제한은 out-of-scope(P4).
- 로그에는 `WRONG_ID` vs `WRONG_PW` 구분을 남기지만, 클라이언트에는 `로그인 실패` 단일 메시지로 응답하는 옵션(enumeration 공격 완화 — P3 적용 검토).

## 5. 비밀번호 변경

- 현재 pw 해시 비교 후 새 pw 길이/문자셋 검증 → `UPDATE users SET password_hash=SHA2(?,256)`.
- 성공 시 모든 기존 세션 유지(로그아웃 강제 안 함). 향후 "다른 기기 로그아웃" 옵션 추가 가능.

## 6. 로그아웃

- `online_status=0`, `last_seen=NOW()` UPDATE 후 세션 제거. 친구에게 상태 변경 전파.

## 7. 관리자 계정

- `sql/schema.sql` 에 `admin` 시드. 배포 시 **즉시 비밀번호 변경** 을 문서에 명시.
