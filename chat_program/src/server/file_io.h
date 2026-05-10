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
 * id//user_id//friend_id//status//status_before_block//created_at
 * ========================================================= */
int  load_friends(const char *path);
void save_friends(const char *path);
int  append_friend(const char *path, const FriendRecord *f);

/* =========================================================
 * dm_reads.txt  포맷:
 * msg_id//reader_id//read_at
 * ========================================================= */
int  load_dm_reads(const char *path);
void save_dm_reads(const char *path);
int  append_dm_read(const char *path, const DmReadRecord *r);

/* =========================================================
 * room_invites.txt  포맷:
 * id//room_id//inviter_id//invitee_id//status//created_at
 * ========================================================= */
int  load_room_invites(const char *path);
void save_room_invites(const char *path);
int  append_room_invite(const char *path, const RoomInviteRecord *r);

/* =========================================================
 * user_settings.txt  포맷:
 * user_id//msg_color//nick_color//theme//ts_format//dnd//welcome_shown
 * ========================================================= */
int  load_user_settings(const char *path);
void save_user_settings(const char *path);
int  upsert_user_settings(const char *path, const UserSettingsRecord *s);

/* =========================================================
 * room_reads.txt  포맷:
 * room_id//user_id//last_read_msg_id//read_at
 * ========================================================= */
int  load_room_reads(const char *path);
void save_room_reads(const char *path);
int  update_room_read(const char *path, int room_id,
                      const char *user_id, int msg_id,
                      const char *read_at);
