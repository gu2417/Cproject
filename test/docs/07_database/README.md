# 07 · Database

MySQL 5.7+ 기반. 스키마는 `sql/schema.sql` 이 원본이고, 본 섹션은 설계 근거·쿼리·운영을 다룬다.

| 문서 | 내용 |
|------|------|
| [er_diagram.md](./er_diagram.md) | 전체 ER 다이어그램 |
| [tables/](./tables/) | 테이블별 상세 (users, user_settings, friends, rooms, room_members, messages, dm_reads, room_invites) · reactions는 out-of-scope |
| [indexes_and_performance.md](./indexes_and_performance.md) | 인덱스 설계·쿼리 성능 |
| [query_catalog.md](./query_catalog.md) | 기능별 대표 쿼리 |
| [connection_pooling.md](./connection_pooling.md) | 스레드 전용 연결 수명 |
| [migration_and_seed.md](./migration_and_seed.md) | 초기화·시드·마이그레이션 |
