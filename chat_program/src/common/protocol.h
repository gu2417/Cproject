#pragma once

/* =========================================================
 * 네트워크 상수
 * ========================================================= */
#define DEFAULT_PORT      55555
#define MAX_PKT_SIZE      10240
#define MAX_BUF_SIZE      10240
#define MAX_CLIENTS       256
#define MAX_ROOMS         100
#define MAX_ROOM_MEMBERS  64
#define MAX_MSG_HISTORY   1000

/* =========================================================
 * 파일 경로 상수
 * ========================================================= */
#define FILE_USERS         "data/users.txt"
#define FILE_ROOMS         "data/rooms.txt"
#define FILE_MESSAGES      "data/messages.txt"
#define FILE_FRIENDS       "data/friends.txt"
#define FILE_ROOM_MEMBERS  "data/room_members.txt"
#define FILE_DM_READS      "data/dm_reads.txt"
#define FILE_ROOM_INVITES  "data/room_invites.txt"
#define FILE_USER_SETTINGS "data/user_settings.txt"
#define FILE_ROOM_READS    "data/room_reads.txt"

/* =========================================================
 * 패킷 구분자
 * ========================================================= */
#define PKT_TYPE_SEP  "|"
#define PKT_FIELD_SEP ":"
#define PKT_LIST_SEP  ";"
#define FILE_FIELD_SEP "//"

/* =========================================================
 * 패킷 타입 — 인증/계정
 * ========================================================= */
#define LOGIN_REQ           "LOGIN_REQ"
#define LOGIN_RES           "LOGIN_RES"
#define REGISTER_REQ        "REGISTER_REQ"
#define REGISTER_RES        "REGISTER_RES"
#define LOGOUT_REQ          "LOGOUT_REQ"
#define LOGOUT_RES          "LOGOUT_RES"
#define PROFILE_UPDATE      "PROFILE_UPDATE"
#define PROFILE_UPDATE_RES  "PROFILE_UPDATE_RES"
#define PASS_CHANGE         "PASS_CHANGE"
#define PASS_CHANGE_RES     "PASS_CHANGE_RES"
#define STATUS_CHANGE       "STATUS_CHANGE"

/* =========================================================
 * 패킷 타입 — 채팅방
 * ========================================================= */
#define ROOM_CREATE         "ROOM_CREATE"
#define ROOM_CREATE_RES     "ROOM_CREATE_RES"
#define ROOM_JOIN           "ROOM_JOIN"
#define ROOM_JOIN_RES       "ROOM_JOIN_RES"
#define ROOM_LEAVE          "ROOM_LEAVE"
#define ROOM_MSG            "ROOM_MSG"
#define ROOM_MSG_RECV       "ROOM_MSG_RECV"
#define ROOM_LIST_REQ       "ROOM_LIST_REQ"
#define ROOM_LIST_RES       "ROOM_LIST_RES"
#define ROOM_HISTORY_REQ    "ROOM_HISTORY_REQ"
#define ROOM_HISTORY_RES    "ROOM_HISTORY_RES"
#define ROOM_INVITE         "ROOM_INVITE"
#define ROOM_INVITE_RES     "ROOM_INVITE_RES"
#define ROOM_INVITE_NOTIFY  "ROOM_INVITE_NOTIFY"
#define ROOM_KICK           "ROOM_KICK"
#define ROOM_KICKED_NOTIFY  "ROOM_KICKED_NOTIFY"
#define ROOM_SET_NOTICE     "ROOM_SET_NOTICE"
#define ROOM_NOTICE         "ROOM_NOTICE"
#define ROOM_GRANT_ADMIN    "ROOM_GRANT_ADMIN"
#define ROOM_REVOKE_ADMIN   "ROOM_REVOKE_ADMIN"
#define ROOM_SET_OPEN_NICK  "ROOM_SET_OPEN_NICK"
#define ROOM_SET_OPEN_NICK_RES "ROOM_SET_OPEN_NICK_RES"
#define ROOM_MUTE           "ROOM_MUTE"
#define ROOM_PIN            "ROOM_PIN"
#define ROOM_SEARCH         "ROOM_SEARCH"

/* =========================================================
 * 패킷 타입 — 메시지
 * ========================================================= */
#define MSG_EDIT            "MSG_EDIT"
#define MSG_DELETE          "MSG_DELETE"
#define MSG_EDITED_NOTIFY   "MSG_EDITED_NOTIFY"
#define MSG_DELETED_NOTIFY  "MSG_DELETED_NOTIFY"
#define MSG_REPLY           "MSG_REPLY"
#define MSG_SEARCH          "MSG_SEARCH"
#define MSG_SEARCH_RES      "MSG_SEARCH_RES"
#define MSG_PIN_NOTIFY      "MSG_PIN_NOTIFY"

/* =========================================================
 * 패킷 타입 — DM
 * ========================================================= */
#define DM_SEND             "DM_SEND"
#define DM_RECV             "DM_RECV"
#define DM_LIST_REQ         "DM_LIST_REQ"
#define DM_LIST_RES         "DM_LIST_RES"
#define DM_HISTORY_REQ      "DM_HISTORY_REQ"
#define DM_HISTORY_RES      "DM_HISTORY_RES"
#define DM_READ_NOTIFY      "DM_READ_NOTIFY"

/* =========================================================
 * 패킷 타입 — 친구
 * ========================================================= */
#define FRIEND_ADD_REQ          "FRIEND_ADD_REQ"
#define FRIEND_ADD_RES          "FRIEND_ADD_RES"
#define FRIEND_ACCEPT           "FRIEND_ACCEPT"
#define FRIEND_REJECT           "FRIEND_REJECT"
#define FRIEND_DELETE           "FRIEND_DELETE"
#define FRIEND_BLOCK            "FRIEND_BLOCK"
#define FRIEND_LIST_REQ         "FRIEND_LIST_REQ"
#define FRIEND_LIST_RES         "FRIEND_LIST_RES"
#define FRIEND_REQUEST_NOTIFY   "FRIEND_REQUEST_NOTIFY"
#define FRIEND_ACCEPT_NOTIFY    "FRIEND_ACCEPT_NOTIFY"
#define FRIEND_STATUS_CHANGE    "FRIEND_STATUS_CHANGE"
#define USER_SEARCH_REQ         "USER_SEARCH_REQ"
#define USER_SEARCH_RES         "USER_SEARCH_RES"

/* =========================================================
 * 패킷 타입 — 마이페이지
 * ========================================================= */
#define MYPAGE_REQ          "MYPAGE_REQ"
#define MYPAGE_RES          "MYPAGE_RES"
#define MY_ROOMS_REQ        "MY_ROOMS_REQ"
#define MY_ROOMS_RES        "MY_ROOMS_RES"

/* =========================================================
 * 패킷 타입 — 설정
 * ========================================================= */
#define SETTINGS_REQ        "SETTINGS_REQ"
#define SETTINGS_RES        "SETTINGS_RES"
#define SETTINGS_UPDATE     "SETTINGS_UPDATE"
#define SETTINGS_UPDATE_RES "SETTINGS_UPDATE_RES"

/* =========================================================
 * 패킷 타입 — 알림/시스템
 * ========================================================= */
#define NOTIFY              "NOTIFY"
#define TYPING_START        "TYPING_START"
#define TYPING_NOTIFY       "TYPING_NOTIFY"
#define PING                "PING"
#define PONG                "PONG"
#define ERROR_PKT           "ERROR"

/* =========================================================
 * 응답 코드 (패킷마다 다름!)
 * ========================================================= */
/* LOGIN_RES */
#define LOGIN_OK            0
#define LOGIN_WRONG_ID      1
#define LOGIN_WRONG_PW      2
#define LOGIN_ALREADY_ONLINE 3

/* REGISTER_RES — 성공=1 (LOGIN과 다름!) */
#define REGISTER_OK         1
#define REGISTER_DUPLICATE  2
#define REGISTER_ERROR      3

/* ROOM_CREATE_RES — 성공=1, 실패=0 */
#define ROOM_CREATE_OK      1
#define ROOM_CREATE_FAIL    0

/* ROOM_JOIN_RES */
#define ROOM_JOIN_OK        0
#define ROOM_JOIN_NOT_FOUND 1
#define ROOM_JOIN_WRONG_PW  2
#define ROOM_JOIN_FULL      3

/* FRIEND_ADD_RES */
#define FRIEND_SENT         0
#define FRIEND_NOT_FOUND    1
#define FRIEND_BLOCKED      2
#define FRIEND_ALREADY      3

/* =========================================================
 * 메시지 타입
 * ========================================================= */
#define MSG_TYPE_NORMAL   0
#define MSG_TYPE_SYSTEM   1
#define MSG_TYPE_WHISPER  2
#define MSG_TYPE_ME       3

/* =========================================================
 * 온라인 상태
 * ========================================================= */
#define STATUS_OFFLINE    0
#define STATUS_ONLINE     1
#define STATUS_BUSY       2
#define STATUS_INVISIBLE  3

/* =========================================================
 * 친구 상태
 * ========================================================= */
#define FRIEND_PENDING    0
#define FRIEND_ACCEPTED   1
#define FRIEND_BLOCKED_S  2
