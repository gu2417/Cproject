# user_settings

## DDL

```sql
CREATE TABLE user_settings (
    user_id    VARCHAR(20)  PRIMARY KEY,
    msg_color  VARCHAR(15)  DEFAULT 'cyan',
    nick_color VARCHAR(15)  DEFAULT 'yellow',
    theme      VARCHAR(10)  DEFAULT 'dark',
    ts_format  TINYINT      DEFAULT 0,
    dnd        TINYINT      DEFAULT 0,   -- 0=off 1=방해금지
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);
```

## 컬럼

| 컬럼 | 범위 |
|------|------|
| `msg_color` | red/green/yellow/blue/magenta/cyan/white |
| `nick_color` | 동일 팔레트 |
| `theme` | `dark` \| `light` |
| `ts_format` | 0=`HH:MM`, 1=`HH:MM:SS`, 2=`MM-DD HH:MM` |
| `dnd` | 0=알림 허용, 1=방해금지(MENTION 제외 NOTIFY 억제) |

## 생명주기

- 회원가입 트랜잭션 내에서 `INSERT user_settings(user_id)` 기본값으로 생성.
- 설정 변경: `SETTINGS_UPDATE` 패킷 → `UPDATE` 한 번에 전체 컬럼 갱신.
- DND 토글: `UPDATE user_settings SET dnd=? WHERE user_id=?`
- 사용자 삭제 시 CASCADE.

## 검증

- 값은 화이트리스트 비교(`05_security/input_validation.md`).
- 미승인 값 입력 시 서버에서 reject → `SETTINGS_UPDATE_RES|1`.
