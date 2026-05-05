-- C Chat Application Schema
-- Version: 2.0.0
-- MySQL 5.7+

DROP DATABASE IF EXISTS chat_db;
CREATE DATABASE chat_db CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE chat_db;

-- ============================================================
-- users: 사용자 정보
-- ============================================================
CREATE TABLE users (
    user_id VARCHAR(20) PRIMARY KEY,
    password_hash VARCHAR(65) NOT NULL COMMENT 'SHA256 hex',
    nickname VARCHAR(20) UNIQUE NOT NULL,
    profile_msg VARCHAR(100) DEFAULT '',
    online_status TINYINT DEFAULT 0 COMMENT '0=offline, 1=online, 2=away',
    last_seen DATETIME DEFAULT CURRENT_TIMESTAMP,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_nickname (nickname),
    INDEX idx_online (online_status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- user_settings: 사용자 설정
-- ============================================================
CREATE TABLE user_settings (
    user_id VARCHAR(20) PRIMARY KEY,
    msg_color VARCHAR(15) DEFAULT 'default',
    nick_color VARCHAR(15) DEFAULT 'default',
    theme VARCHAR(10) DEFAULT 'light' COMMENT 'light, dark, auto',
    timestamp_format TINYINT DEFAULT 0 COMMENT '0=12h, 1=24h',
    dnd_mode TINYINT DEFAULT 0 COMMENT 'do-not-disturb',
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- friends: 친구 관계
-- ============================================================
CREATE TABLE friends (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id VARCHAR(20) NOT NULL,
    friend_id VARCHAR(20) NOT NULL,
    status TINYINT DEFAULT 0 COMMENT '0=pending, 1=accepted, 2=blocked',
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_friendship (user_id, friend_id),
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    FOREIGN KEY (friend_id) REFERENCES users(user_id) ON DELETE CASCADE,
    INDEX idx_status (status),
    INDEX idx_user (user_id, status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- rooms: 채팅방
-- ============================================================
CREATE TABLE rooms (
    room_id INT AUTO_INCREMENT PRIMARY KEY,
    room_name VARCHAR(50) NOT NULL,
    topic VARCHAR(100) DEFAULT '',
    password_hash VARCHAR(65) COMMENT 'NULL=open, non-NULL=private',
    max_users INT DEFAULT 100,
    owner_id VARCHAR(20) NOT NULL,
    notice VARCHAR(255) DEFAULT '',
    is_open TINYINT DEFAULT 1 COMMENT '1=open, 0=private',
    pinned_msg_id INT COMMENT 'FK to messages.id, no constraint',
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (owner_id) REFERENCES users(user_id) ON DELETE CASCADE,
    INDEX idx_open (is_open),
    INDEX idx_owner (owner_id),
    INDEX idx_created (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- room_members: 방 멤버
-- ============================================================
CREATE TABLE room_members (
    room_id INT NOT NULL,
    user_id VARCHAR(20) NOT NULL,
    open_nick VARCHAR(30) COMMENT 'Open chat nickname',
    is_admin TINYINT DEFAULT 0,
    is_muted TINYINT DEFAULT 0,
    joined_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (room_id, user_id),
    FOREIGN KEY (room_id) REFERENCES rooms(room_id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    INDEX idx_user (user_id),
    INDEX idx_joined (joined_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- messages: 메시지
-- ============================================================
CREATE TABLE messages (
    msg_id INT AUTO_INCREMENT PRIMARY KEY,
    room_id INT COMMENT 'NULL=DM',
    from_id VARCHAR(20) NOT NULL,
    to_id VARCHAR(20) COMMENT 'DM only',
    content VARCHAR(500) NOT NULL,
    reply_to INT COMMENT 'FK to messages.id',
    msg_type TINYINT DEFAULT 0 COMMENT '0=normal, 1=system, 2=whisper, 3=me',
    is_deleted TINYINT DEFAULT 0,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    edited_at DATETIME,
    FOREIGN KEY (room_id) REFERENCES rooms(room_id) ON DELETE CASCADE,
    FOREIGN KEY (from_id) REFERENCES users(user_id) ON DELETE CASCADE,
    FOREIGN KEY (to_id) REFERENCES users(user_id) ON DELETE CASCADE,
    FOREIGN KEY (reply_to) REFERENCES messages(msg_id) ON DELETE SET NULL,
    INDEX idx_room (room_id, created_at),
    INDEX idx_from (from_id),
    INDEX idx_to (to_id),
    INDEX idx_created (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- dm_reads: DM 읽음 상태
-- ============================================================
CREATE TABLE dm_reads (
    msg_id INT NOT NULL,
    reader_id VARCHAR(20) NOT NULL,
    read_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (msg_id, reader_id),
    FOREIGN KEY (msg_id) REFERENCES messages(msg_id) ON DELETE CASCADE,
    FOREIGN KEY (reader_id) REFERENCES users(user_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- notifications: 알림
-- ============================================================
CREATE TABLE notifications (
    notif_id INT AUTO_INCREMENT PRIMARY KEY,
    user_id VARCHAR(20) NOT NULL,
    notif_type TINYINT COMMENT '0=friend_req, 1=room_invite, 2=mentioned, 3=msg_reply, 4=msg_edited',
    from_id VARCHAR(20),
    room_id INT,
    content VARCHAR(100),
    is_read TINYINT DEFAULT 0,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    FOREIGN KEY (from_id) REFERENCES users(user_id) ON DELETE CASCADE,
    FOREIGN KEY (room_id) REFERENCES rooms(room_id) ON DELETE CASCADE,
    INDEX idx_user_read (user_id, is_read),
    INDEX idx_created (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- room_invites: 초대 현황
-- ============================================================
CREATE TABLE room_invites (
    invite_id INT AUTO_INCREMENT PRIMARY KEY,
    room_id INT NOT NULL,
    inviter_id VARCHAR(20) NOT NULL,
    invitee_id VARCHAR(20) NOT NULL,
    status TINYINT DEFAULT 0 COMMENT '0=pending, 1=accepted, 2=declined',
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (room_id) REFERENCES rooms(room_id) ON DELETE CASCADE,
    FOREIGN KEY (inviter_id) REFERENCES users(user_id) ON DELETE CASCADE,
    FOREIGN KEY (invitee_id) REFERENCES users(user_id) ON DELETE CASCADE,
    INDEX idx_invitee (invitee_id, status),
    INDEX idx_status (status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- Seed: system 계정 및 테스트 데이터
-- ============================================================
INSERT INTO users (user_id, password_hash, nickname, profile_msg, online_status)
VALUES ('system', SHA2('', 256), 'System', 'System announcements', 1);

INSERT INTO users (user_id, password_hash, nickname, profile_msg, online_status)
VALUES
    ('alice', SHA2('alice123', 256), 'Alice', 'Hello, I''m Alice', 0),
    ('bob', SHA2('bob123', 256), 'Bob', 'Hi, I''m Bob', 0),
    ('charlie', SHA2('charlie123', 256), 'Charlie', 'Hey there!', 0);

INSERT INTO user_settings (user_id)
VALUES ('system'), ('alice'), ('bob'), ('charlie');
