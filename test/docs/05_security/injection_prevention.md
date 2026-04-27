# SQL Injection 방지

## 1. 기본 원칙

> **모든 SQL 은 prepared statement 를 사용한다. 예외 없음.**

문자열 연결/`sprintf` 로 SQL 을 만들면 코드 리뷰에서 **즉시 리젝트**.

## 2. 권장 래퍼 API

`chat_program/src/server/db.h`:
```c
typedef struct DbStmt DbStmt;                 /* 내부: MYSQL_STMT */

DbStmt *db_prepare(MYSQL *c, const char *sql);
int db_bind_str(DbStmt *s, int idx, const char *v);
int db_bind_int(DbStmt *s, int idx, long v);
int db_execute(DbStmt *s);
int db_fetch_row(DbStmt *s);                  /* 0 = 더 없음, 1 = row */
const char *db_col_str(DbStmt *s, int idx);
long        db_col_int(DbStmt *s, int idx);
void        db_stmt_close(DbStmt *s);
```

사용 예:
```c
DbStmt *st = db_prepare(db, "SELECT nickname FROM users WHERE id=?");
db_bind_str(st, 0, user_id);
db_execute(st);
if (db_fetch_row(st)) {
    strlcpy_safe(nick, db_col_str(st, 0), sizeof(nick));
}
db_stmt_close(st);
```

## 3. LIKE 검색

- 와일드카드 `%`, `_` 는 유저 입력을 **이스케이프** 한 후 래핑:
  ```c
  /* in → in_escaped with `\%` and `\_` */
  snprintf(pattern, sizeof pattern, "%%%s%%", in_escaped);
  db_bind_str(st, 0, pattern);
  ```
- 쿼리: `... WHERE name LIKE ? ESCAPE '\\'`.

## 4. 동적 `IN (...)`

- 리스트 길이 상한(예: 32)을 두고 **플레이스홀더 배열** 로 구성:
  ```c
  build_placeholders(buf, n);     /* "?,?,?" */
  snprintf(sql, ..., "... WHERE id IN (%s)", buf);
  /* 이후 각 요소를 bind */
  ```
- 문자열 concat 으로 값을 직접 끼워넣지 않는다.

## 5. ORDER BY / 컬럼명

- `ORDER BY ?` 는 불가. 허용 컬럼을 **화이트리스트** 로 매핑:
  ```c
  static const char *SORTABLE[] = { "id", "created_at", "name" };
  ```

## 6. 에러 처리

- `db_execute` 실패 시 `mysql_stmt_errno` 로 분류(9장 참조). deadlock(1213)·lock timeout(1205) 은 1회 재시도.

## 7. 감사 체크리스트(PR 리뷰 시)

- [ ] `sprintf("... %s ...", user_input)` 없음
- [ ] `mysql_query(c, ...)` 로 raw 호출 없음(모든 호출은 `db_*` 경유)
- [ ] LIKE 에 유저 입력이 직접 들어가지 않음
- [ ] `IN (...)` 는 placeholder 배열로만 구성
