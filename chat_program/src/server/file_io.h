#pragma once

#include "../common/types.h"

/* =========================================================
 * users.txt  포맷:
 * id//pw_hash//nickname//status_msg//online_status//is_admin//last_seen//created_at
 * ========================================================= */
int  load_users(const char *path);
void save_users(const char *path);
int  append_user(const char *path, const UserRecord *u);

/* =========================================================
 * rooms.txt  포맷:
 * id//name//topic//pw_hash//max_users//owner_id//notice//is_open//pinned_msg_id//created_at
 * ========================================================= */
int  load_rooms(const char *path);
void save_rooms(const char *path);
int  append_room(const char *path, const RoomRecord *r);

/* =========================================================
 * room_members.txt  포맷:
 * room_id//user_id//open_nick//is_admin//is_muted//joined_at
 * ========================================================= */
int  load_room_members(const char *path);
void save_room_members(const char *path);
int  append_room_member(const char *path, const RoomMemberRecord *m);

/* =========================================================
 * messages.txt  포맷 (content-last):
 * id//room_id//from_id//to_id//reply_to//msg_type//is_deleted//created_at//edited_at//content
 * ========================================================= */
int  load_messages(const char *path);
int  append_message(const char *path, const MessageRecord *m);

/* =========================================================
 * friends.txt  포맷:
 * id//user_id//friend_id//status//created_at
 * ========================================================= */
int  load_friends(const char *path);
void save_friends(const char *path);
int  append_friend(const char *path, const FriendRecord *f);
