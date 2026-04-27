# 에러 분류 체계

## 1. 계층

| 계층 | 예 | 처리 주체 |
|------|-----|-----------|
| 네트워크 | socket read=0, ECONNRESET, EPIPE | 서버 핸들러 / 클라 recv thread |
| 프로토콜 | 길이 초과, TYPE 미지원, 필드 부족 | 수신 측(파서) |
| 인증 | 비밀번호 오류, 세션 만료, 중복 로그인 | 서버 auth handler |
| 권한 | 방장/관리자 전용, 차단 관계 | 서버 authz |
| 검증 | 길이, 문자셋, 화이트리스트 | 수신 측 validator |
| 상태 | 이미 삭제/만료, 정원 초과 | 서버 business layer |
| DB | connect fail, deadlock, lost, constraint | 서버 db layer |
| 시스템 | OOM, thread create 실패 | 서버 main |
| 렌더 | GTK4 렌더 실패, 소켓 write 실패 | 클라 UI |

## 2. 매핑 → error_codes

| 계층 | 주 코드 |
|------|---------|
| 프로토콜 | 10 |
| 인증 | 2 |
| 권한 | 5 |
| 검증 | 1 |
| 상태 | 6 |
| 차단 | 7 |
| DB/시스템 | 9 |
| 중복 | 3 |
| 없음 | 4 |

## 3. 전파

- 서버: 각 핸들러는 `int` 반환으로 코드를 돌려주고 라우터가 `*_RES` 생성.
- 클라: 에러는 UI 레이어까지 전달돼 상태 바(status bar) 또는 모달로 표시.
