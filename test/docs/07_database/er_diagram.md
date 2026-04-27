# ER 다이어그램

```mermaid
erDiagram
    users ||--|| user_settings : "1:1"
    users ||--o{ friends : "user_id"
    users ||--o{ friends : "friend_id"
    users ||--o{ rooms : "owner"
    users ||--o{ room_members : ""
    rooms ||--o{ room_members : ""
    rooms ||--o{ messages : "group/open"
    users ||--o{ messages : "from"
    messages ||--o{ dm_reads : ""
    users ||--o{ dm_reads : ""
    messages ||--o{ messages : "reply_to"
    rooms ||--o{ room_invites : "초대"
    users ||--o{ room_invites : "초대자/피초대자"

    users {
        VARCHAR20 id PK
        VARCHAR64 password_hash
        VARCHAR20 nickname "UNIQUE"
        VARCHAR100 status_msg
        TINYINT online_status
        DATETIME last_seen
        DATETIME created_at
    }
    user_settings {
        VARCHAR20 user_id PK_FK
        VARCHAR15 msg_color
        VARCHAR15 nick_color
        VARCHAR10 theme
        TINYINT ts_format
        TINYINT dnd
    }
    friends {
        INT id PK
        VARCHAR20 user_id FK
        VARCHAR20 friend_id FK
        TINYINT status "0=pending 1=accepted 2=blocked"
        DATETIME created_at
    }
    rooms {
        INT id PK
        VARCHAR30 name
        VARCHAR100 topic
        VARCHAR64 password_hash
        INT max_users
        VARCHAR20 owner_id FK
        VARCHAR255 notice
        TINYINT is_open
        INT pinned_msg_id
        DATETIME created_at
    }
    room_members {
        INT room_id PK_FK
        VARCHAR20 user_id PK_FK
        VARCHAR20 open_nick
        TINYINT is_admin
        TINYINT is_muted
        DATETIME joined_at
    }
    messages {
        INT id PK
        INT room_id "NULL=DM"
        VARCHAR20 from_id FK
        VARCHAR20 to_id "DM only"
        VARCHAR500 content
        INT reply_to FK
        TINYINT msg_type "0=normal 1=system 2=whisper 3=me"
        TINYINT is_deleted
        DATETIME created_at
        DATETIME edited_at
    }
    dm_reads {
        INT msg_id PK_FK
        VARCHAR20 reader_id PK_FK
        DATETIME read_at
    }
    room_invites {
        INT id PK
        INT room_id FK
        VARCHAR20 inviter_id FK
        VARCHAR20 invitee_id FK
        TINYINT status
        DATETIME created_at
    }
```

## 주요 관계 설명

- `users 1:1 user_settings`: 가입 시 자동 생성(트리거 또는 앱 레벨 `INSERT`). DND 설정은 `user_settings.dnd` 에 저장.
- `friends`: 양방향 표현을 위해 accepted 시 `(A,B)`, `(B,A)` 두 레코드 사용.
- `rooms.pinned_msg_id`: `messages.id` 를 참조하지만 **FK 미설정**(순환 의존 방지). 앱 레벨에서 정합성 유지.
- `messages.reply_to`: self-reference FK. 원본 삭제 시 `SET NULL`.
- `messages.to_id`: DM 일 때만 의미. 그룹 메시지에서는 NULL.
- `dm_reads`: DM 전용 읽음 상태. 그룹 읽음 표시는 v2.0 범위 아님.
- `room_invites`: 방장이 특정 사용자를 초대할 때 생성. `status` 0=pending, 1=accepted, 2=declined.
- `users.nickname`: UNIQUE 제약으로 닉네임 중복 방지.
