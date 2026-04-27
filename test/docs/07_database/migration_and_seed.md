# 초기화 · 시드 · 마이그레이션

## 1. 파일

- `sql/schema.sql` — DB 생성, 테이블 DDL, 시드. 재실행 가능하도록 `DROP` 없이 `CREATE TABLE IF NOT EXISTS` 사용(초기 배포 외에는 부분 마이그레이션 스크립트 별도).

## 2. 초기 설치 절차

```bash
mysql -u root -p < sql/schema.sql
```

`schema.sql` 구성(예시 구조):

```sql
-- 1. 데이터베이스
CREATE DATABASE IF NOT EXISTS chat_db
  DEFAULT CHARACTER SET utf8mb4 DEFAULT COLLATE utf8mb4_unicode_ci;
USE chat_db;

-- 2. 애플리케이션 유저(권한 최소화)
CREATE USER IF NOT EXISTS 'chat_app'@'localhost' IDENTIFIED BY 'CHANGE_ME';
GRANT SELECT, INSERT, UPDATE, DELETE ON chat_db.* TO 'chat_app'@'localhost';
FLUSH PRIVILEGES;

-- 3. 테이블 (07_database/tables/*.md 의 DDL)
-- users, user_settings, friends, rooms, room_members,
-- messages, dm_reads, room_invites

-- 4. 시스템 유저 (system 메시지용 예약)
INSERT IGNORE INTO users (id, password_hash, nickname)
VALUES ('system', SHA2('**disabled**', 256), '시스템');

-- 5. 관리자 계정
INSERT IGNORE INTO users (id, password_hash, nickname)
VALUES ('admin', SHA2('admin', 256), '관리자');
INSERT IGNORE INTO user_settings (user_id, dnd) VALUES ('admin', 0), ('system', 0);
```

## 3. 재설치

개발 환경에서 전체 초기화:
```sql
DROP DATABASE IF EXISTS chat_db;
-- 이후 schema.sql 재실행
```

운영에서는 **절대 사용 금지**.

## 4. 마이그레이션 정책

- v2.x 내 마이너 변경: `sql/migrations/v2_<n>_<desc>.sql` 형식으로 ALTER 스크립트 누적.
- 실행 추적은 앱 외부(운영자 체크리스트). 자동 마이그레이션 런너는 out-of-scope.

## 5. 백업 권장

- `mysqldump chat_db > backup.sql` 정기 실행(운영자 책임).
- 무정지 백업은 out-of-scope.

## 6. 배포 시 체크

- [ ] `chat_app` 비밀번호 변경
- [ ] `admin` 계정 비밀번호 변경(또는 비활성화 후 새 admin 생성)
- [ ] `config.h` 의 `DB_HOST/DB_USER/DB_PASS/DB_NAME` 이 배포 환경과 일치
- [ ] `sql_mode` 에 `STRICT_TRANS_TABLES` 포함 확인
