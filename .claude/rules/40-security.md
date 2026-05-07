# Security Rules

## 금지문자 검사
사용자 입력(ID, 닉네임, 비번, 방 이름, 메시지 제외)에서 다음 문자 금지:
`: ; | \n`
- `has_forbidden_char()` 함수로 검사
- 등록/로그인/방 생성 시 반드시 적용

## 패스워드 해싱
- SHA-256 hex-lowercase 64자
- WinCrypt: `CryptAcquireContext` → `CryptCreateHash` → `CryptHashData` → `CryptGetHashParam` → `CryptDestroyHash` → `CryptReleaseContext`
- 평문 비번 로그 금지 — `printf` 시 `***` 마스크
- 길이 제한: 유저 비번 1-19자, 방 비번 1-10자

## 금지 함수
- `gets()` — 절대 사용 금지. `fgets()` 또는 `scanf("%Ns", ...)` 사용

## 차단 검사 방향
`is_blocked_by(receiver_id, sender_id)` — receiver가 sender를 차단했는지.
호출 순서를 반드시 준수.
